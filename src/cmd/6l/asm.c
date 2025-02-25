// Inferno utils/6l/asm.c
// http://code.google.com/p/inferno-os/source/browse/utils/6l/asm.c
//
//	Copyright © 1994-1999 Lucent Technologies Inc.  All rights reserved.
//	Portions Copyright © 1995-1997 C H Forsyth (forsyth@terzarima.net)
//	Portions Copyright © 1997-1999 Vita Nuova Limited
//	Portions Copyright © 2000-2007 Vita Nuova Holdings Limited (www.vitanuova.com)
//	Portions Copyright © 2004,2006 Bruce Ellis
//	Portions Copyright © 2005-2007 C H Forsyth (forsyth@terzarima.net)
//	Revisions Copyright © 2000-2007 Lucent Technologies Inc. and others
//	Portions Copyright © 2009 The Go Authors.  All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

// Writing object files.

#include	"l.h"
#include	"../ld/lib.h"
#include	"../ld/elf.h"
#include	"../ld/dwarf.h"

#define PADDR(a)	((uint32)(a) & ~0x80000000)

char linuxdynld[] = "/lib64/ld-linux-x86-64.so.2";
char freebsddynld[] = "/libexec/ld-elf.so.1";
char openbsddynld[] = "/usr/libexec/ld.so";
char netbsddynld[] = "/libexec/ld.elf_so";
char dragonflydynld[] = "/usr/libexec/ld-elf.so.2";

char	zeroes[32];

vlong
entryvalue(void)
{
	char *a;
	Sym *s;

	a = INITENTRY;
	if(*a >= '0' && *a <= '9')
		return atolwhex(a);
	s = lookup(a, 0);
	if(s->type == 0)
		return INITTEXT;
	if(s->type != STEXT)
		diag("entry not text: %s", s->name);
	return s->value;
}

vlong
datoff(vlong addr)
{
	if(addr >= segdata.vaddr)
		return addr - segdata.vaddr + segdata.fileoff;
	if(addr >= segtext.vaddr)
		return addr - segtext.vaddr + segtext.fileoff;
	diag("datoff %#llx", addr);
	return 0;
}

static int
needlib(char *name)
{
	char *p;
	Sym *s;

	if(*name == '\0')
		return 0;

	/* reuse hash code in symbol table */
	p = smprint(".elfload.%s", name);
	s = lookup(p, 0);
	free(p);
	if(s->type == 0) {
		s->type = 100;	// avoid SDATA, etc.
		return 1;
	}
	return 0;
}

int nelfsym = 1;

static void addpltsym(Sym*);
static void addgotsym(Sym*);

void
adddynrela(Sym *rela, Sym *s, Reloc *r)
{
	addaddrplus(rela, s, r->off);
	adduint64(rela, R_X86_64_RELATIVE);
	addaddrplus(rela, r->sym, r->add); // Addend
}

void
adddynrel(Sym *s, Reloc *r)
{
	Sym *targ, *rela, *got;
	
	targ = r->sym;
	cursym = s;

	switch(r->type) {
	default:
		if(r->type >= 256) {
			diag("unexpected relocation type %d", r->type);
			return;
		}
		break;

	// Handle relocations found in ELF object files.
	case 256 + R_X86_64_PC32:
		if(targ->type == SDYNIMPORT)
			diag("unexpected R_X86_64_PC32 relocation for dynamic symbol %s", targ->name);
		if(targ->type == 0 || targ->type == SXREF)
			diag("unknown symbol %s in pcrel", targ->name);
		r->type = D_PCREL;
		r->add += 4;
		return;
	
	case 256 + R_X86_64_PLT32:
		r->type = D_PCREL;
		r->add += 4;
		if(targ->type == SDYNIMPORT) {
			addpltsym(targ);
			r->sym = lookup(".plt", 0);
			r->add += targ->plt;
		}
		return;
	
	case 256 + R_X86_64_GOTPCREL:
		if(targ->type != SDYNIMPORT) {
			// have symbol
			if(r->off >= 2 && s->p[r->off-2] == 0x8b) {
				// turn MOVQ of GOT entry into LEAQ of symbol itself
				s->p[r->off-2] = 0x8d;
				r->type = D_PCREL;
				r->add += 4;
				return;
			}
			// fall back to using GOT and hope for the best (CMOV*)
			// TODO: just needs relocation, no need to put in .dynsym
		}
		addgotsym(targ);
		r->type = D_PCREL;
		r->sym = lookup(".got", 0);
		r->add += 4;
		r->add += targ->got;
		return;
	
	case 256 + R_X86_64_64:
		if(targ->type == SDYNIMPORT)
			diag("unexpected R_X86_64_64 relocation for dynamic symbol %s", targ->name);
		r->type = D_ADDR;
		return;
	}
	
	// Handle references to ELF symbols from our own object files.
	if(targ->type != SDYNIMPORT)
		return;

	switch(r->type) {
	case D_PCREL:
		addpltsym(targ);
		r->sym = lookup(".plt", 0);
		r->add = targ->plt;
		return;
	
	case D_ADDR:
		if(s->type != SDATA)
			break;
		if(iself) {
			adddynsym(targ);
			rela = lookup(".rela", 0);
			addaddrplus(rela, s, r->off);
			if(r->siz == 8)
				adduint64(rela, ELF64_R_INFO(targ->dynid, R_X86_64_64));
			else
				adduint64(rela, ELF64_R_INFO(targ->dynid, R_X86_64_32));
			adduint64(rela, r->add);
			r->type = 256;	// ignore during relocsym
			return;
		}
		if(HEADTYPE == Hdarwin && s->size == PtrSize && r->off == 0) {
			// Mach-O relocations are a royal pain to lay out.
			// They use a compact stateful bytecode representation
			// that is too much bother to deal with.
			// Instead, interpret the C declaration
			//	void *_Cvar_stderr = &stderr;
			// as making _Cvar_stderr the name of a GOT entry
			// for stderr.  This is separate from the usual GOT entry,
			// just in case the C code assigns to the variable,
			// and of course it only works for single pointers,
			// but we only need to support cgo and that's all it needs.
			adddynsym(targ);
			got = lookup(".got", 0);
			s->type = got->type | SSUB;
			s->outer = got;
			s->sub = got->sub;
			got->sub = s;
			s->value = got->size;
			adduint64(got, 0);
			adduint32(lookup(".linkedit.got", 0), targ->dynid);
			r->type = 256;	// ignore during relocsym
			return;
		}
		break;
	}
	
	cursym = s;
	diag("unsupported relocation for dynamic symbol %s (type=%d stype=%d)", targ->name, r->type, targ->type);
}

int
elfreloc1(Reloc *r, vlong sectoff)
{
	int32 elfsym;

	VPUT(sectoff);

	elfsym = r->xsym->elfsym;
	switch(r->type) {
	default:
		return -1;

	case D_ADDR:
		if(r->siz == 4)
			VPUT(R_X86_64_32 | (uint64)elfsym<<32);
		else if(r->siz == 8)
			VPUT(R_X86_64_64 | (uint64)elfsym<<32);
		else
			return -1;
		break;

	case D_PCREL:
		if(r->siz == 4)
			VPUT(R_X86_64_PC32 | (uint64)elfsym<<32);
		else
			return -1;
		break;
	
	case D_TLS:
		if(r->siz == 4) {
			if(flag_shared)
				VPUT(R_X86_64_GOTTPOFF | (uint64)elfsym<<32);
			else
				VPUT(R_X86_64_TPOFF32 | (uint64)elfsym<<32);
		} else
			return -1;
		break;		
	}

	VPUT(r->xadd);
	return 0;
}

int
archreloc(Reloc *r, Sym *s, vlong *val)
{
	USED(r);
	USED(s);
	USED(val);
	return -1;
}

void
elfsetupplt(void)
{
	Sym *plt, *got;

	plt = lookup(".plt", 0);
	got = lookup(".got.plt", 0);
	if(plt->size == 0) {
		// pushq got+8(IP)
		adduint8(plt, 0xff);
		adduint8(plt, 0x35);
		addpcrelplus(plt, got, 8);
		
		// jmpq got+16(IP)
		adduint8(plt, 0xff);
		adduint8(plt, 0x25);
		addpcrelplus(plt, got, 16);
		
		// nopl 0(AX)
		adduint32(plt, 0x00401f0f);
		
		// assume got->size == 0 too
		addaddrplus(got, lookup(".dynamic", 0), 0);
		adduint64(got, 0);
		adduint64(got, 0);
	}
}

static void
addpltsym(Sym *s)
{
	if(s->plt >= 0)
		return;
	
	adddynsym(s);
	
	if(iself) {
		Sym *plt, *got, *rela;

		plt = lookup(".plt", 0);
		got = lookup(".got.plt", 0);
		rela = lookup(".rela.plt", 0);
		if(plt->size == 0)
			elfsetupplt();
		
		// jmpq *got+size(IP)
		adduint8(plt, 0xff);
		adduint8(plt, 0x25);
		addpcrelplus(plt, got, got->size);
	
		// add to got: pointer to current pos in plt
		addaddrplus(got, plt, plt->size);
		
		// pushq $x
		adduint8(plt, 0x68);
		adduint32(plt, (got->size-24-8)/8);
		
		// jmpq .plt
		adduint8(plt, 0xe9);
		adduint32(plt, -(plt->size+4));
		
		// rela
		addaddrplus(rela, got, got->size-8);
		adduint64(rela, ELF64_R_INFO(s->dynid, R_X86_64_JMP_SLOT));
		adduint64(rela, 0);
		
		s->plt = plt->size - 16;
	} else if(HEADTYPE == Hdarwin) {
		// To do lazy symbol lookup right, we're supposed
		// to tell the dynamic loader which library each 
		// symbol comes from and format the link info
		// section just so.  I'm too lazy (ha!) to do that
		// so for now we'll just use non-lazy pointers,
		// which don't need to be told which library to use.
		//
		// http://networkpx.blogspot.com/2009/09/about-lcdyldinfoonly-command.html
		// has details about what we're avoiding.

		Sym *plt;
		
		addgotsym(s);
		plt = lookup(".plt", 0);

		adduint32(lookup(".linkedit.plt", 0), s->dynid);

		// jmpq *got+size(IP)
		s->plt = plt->size;

		adduint8(plt, 0xff);
		adduint8(plt, 0x25);
		addpcrelplus(plt, lookup(".got", 0), s->got);
	} else {
		diag("addpltsym: unsupported binary format");
	}
}

static void
addgotsym(Sym *s)
{
	Sym *got, *rela;

	if(s->got >= 0)
		return;

	adddynsym(s);
	got = lookup(".got", 0);
	s->got = got->size;
	adduint64(got, 0);

	if(iself) {
		rela = lookup(".rela", 0);
		addaddrplus(rela, got, s->got);
		adduint64(rela, ELF64_R_INFO(s->dynid, R_X86_64_GLOB_DAT));
		adduint64(rela, 0);
	} else if(HEADTYPE == Hdarwin) {
		adduint32(lookup(".linkedit.got", 0), s->dynid);
	} else {
		diag("addgotsym: unsupported binary format");
	}
}

void
adddynsym(Sym *s)
{
	Sym *d;
	int t;
	char *name;

	if(s->dynid >= 0)
		return;

	if(iself) {
		s->dynid = nelfsym++;

		d = lookup(".dynsym", 0);

		name = s->extname;
		adduint32(d, addstring(lookup(".dynstr", 0), name));
		/* type */
		t = STB_GLOBAL << 4;
		if(s->cgoexport && (s->type&SMASK) == STEXT)
			t |= STT_FUNC;
		else
			t |= STT_OBJECT;
		adduint8(d, t);
	
		/* reserved */
		adduint8(d, 0);
	
		/* section where symbol is defined */
		if(s->type == SDYNIMPORT)
			adduint16(d, SHN_UNDEF);
		else {
			switch(s->type) {
			default:
			case STEXT:
				t = 11;
				break;
			case SRODATA:
				t = 12;
				break;
			case SDATA:
				t = 13;
				break;
			case SBSS:
				t = 14;
				break;
			}
			adduint16(d, t);
		}
	
		/* value */
		if(s->type == SDYNIMPORT)
			adduint64(d, 0);
		else
			addaddr(d, s);
	
		/* size of object */
		adduint64(d, s->size);
	
		if(!(s->cgoexport & CgoExportDynamic) && s->dynimplib && needlib(s->dynimplib)) {
			elfwritedynent(lookup(".dynamic", 0), DT_NEEDED,
				addstring(lookup(".dynstr", 0), s->dynimplib));
		}
	} else {
		diag("adddynsym: unsupported binary format");
	}
}

void
adddynlib(char *lib)
{
	Sym *s;
	
	if(!needlib(lib))
		return;
	
	if(iself) {
		s = lookup(".dynstr", 0);
		if(s->size == 0)
			addstring(s, "");
		elfwritedynent(lookup(".dynamic", 0), DT_NEEDED, addstring(s, lib));
	} else {
		diag("adddynlib: unsupported binary format");
	}
}

void
asmb(void)
{
	vlong symo;
	Section *sect;

	if(debug['v'])
		Bprint(&bso, "%5.2f asmb\n", cputime());
	Bflush(&bso);

	if(debug['v'])
		Bprint(&bso, "%5.2f codeblk\n", cputime());
	Bflush(&bso);

	if(iself)
		asmbelfsetup();

	sect = segtext.sect;
	cseek(sect->vaddr - segtext.vaddr + segtext.fileoff);
	codeblk(sect->vaddr, sect->len);
	for(sect = sect->next; sect != nil; sect = sect->next) {
		cseek(sect->vaddr - segtext.vaddr + segtext.fileoff);
		datblk(sect->vaddr, sect->len);
	}

	if(segrodata.filelen > 0) {
		if(debug['v'])
			Bprint(&bso, "%5.2f rodatblk\n", cputime());
		Bflush(&bso);

		cseek(segrodata.fileoff);
		datblk(segrodata.vaddr, segrodata.filelen);
	}

	if(debug['v'])
		Bprint(&bso, "%5.2f datblk\n", cputime());
	Bflush(&bso);

	cseek(segdata.fileoff);
	datblk(segdata.vaddr, segdata.filelen);

	switch(HEADTYPE) {
	default:
		diag("unknown header type %d", HEADTYPE);
	case Helf:
		break;
	case Hlinux:
		debug['8'] = 1;	/* 64-bit addresses */
		break;
	}

	symsize = 0;
	spsize = 0;
	lcsize = 0;
	symo = 0;
	if(!debug['s']) {
		if(debug['v'])
			Bprint(&bso, "%5.2f sym\n", cputime());
		Bflush(&bso);
		switch(HEADTYPE) {
		default:
		case Helf:
			debug['s'] = 1;
			symo = HEADR+segtext.len+segdata.filelen;
			break;
		case Hlinux:
			symo = rnd(HEADR+segtext.len, INITRND)+rnd(segrodata.len, INITRND)+segdata.filelen;
			symo = rnd(symo, INITRND);
			break;
		}
		cseek(symo);
		switch(HEADTYPE) {
		default:
			if(iself) {
				cseek(symo);
				asmelfsym();
				cflush();
				cwrite(elfstrdat, elfstrsize);

				if(debug['v'])
				       Bprint(&bso, "%5.2f dwarf\n", cputime());

				dwarfemitdebugsections();
				
				if(linkmode == LinkExternal)
					elfemitreloc();
			}
			break;
		}
	}

	if(debug['v'])
		Bprint(&bso, "%5.2f headr\n", cputime());
	Bflush(&bso);
	cseek(0L);
	switch(HEADTYPE) {
	default:
	case Hlinux:
		asmbelf(symo);
		break;
	}
	cflush();
}

vlong
rnd(vlong v, vlong r)
{
	vlong c;

	if(r <= 0)
		return v;
	v += r - 1;
	c = v % r;
	if(c < 0)
		c += r;
	v -= c;
	return v;
}
