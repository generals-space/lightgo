// Inferno libmach/executable.c
// http://code.google.com/p/inferno-os/source/browse/utils/libmach/executable.c
//
//	Copyright © 1994-1999 Lucent Technologies Inc.
//	Power PC support Copyright © 1995-2004 C H Forsyth (forsyth@terzarima.net).
//	Portions Copyright © 1997-1999 Vita Nuova Limited.
//	Portions Copyright © 2000-2007 Vita Nuova Holdings Limited (www.vitanuova.com).
//	Revisions Copyright © 2000-2004 Lucent Technologies Inc. and others.
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

#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	<bootexec.h>
#include	<mach.h>
#include	"elf.h"

/*
 *	All a.out header types.  The dummy entry allows canonical
 *	processing of the union as a sequence of int32s
 */

typedef struct {
	union{
		/*struct { */
			Exec exechdr;		/* a.out.h */
		/*	uvlong hdr[1];*/
		/*};*/
		Ehdr32 elfhdr32;			/* elf.h */
		Ehdr64 elfhdr64;			/* elf.h */
	} e;
	int32 dummy;			/* padding to ensure extra int32 */
} ExecHdr;

static	int	commonllp64(int, Fhdr*, ExecHdr*);
static	int	elfdotout(int, Fhdr*, ExecHdr*);
static	void	setsym(Fhdr*, vlong, int32, vlong, int32, vlong, int32);
static	void	setdata(Fhdr*, uvlong, int32, vlong, int32);
static	void	settext(Fhdr*, uvlong, uvlong, int32, vlong);
static	void	hswal(void*, int, uint32(*)(uint32));
static	uvlong	_round(uvlong, uint32);

/*
 *	definition of per-executable file type structures
 */

typedef struct Exectable{
	int32	magic;			/* big-endian magic number of file */
	char	*name;			/* executable identifier */
	char	*dlmname;		/* dynamically loadable module identifier */
	uchar	type;			/* Internal code */
	uchar	_magic;			/* _MAGIC() magic */
	Mach	*mach;			/* Per-machine data */
	int32	hsize;			/* header size */
	uint32	(*swal)(uint32);		/* beswal or leswal */
	int	(*hparse)(int, Fhdr*, ExecHdr*);
} ExecTable;

extern	Mach	mi386;
extern	Mach	mamd64;

/* BUG: FIX THESE WHEN NEEDED */
Mach	malpha;

ExecTable exectab[] =
{
	{ S_MAGIC,			/* amd64 6.out & boot image */
		"amd64 plan 9 executable",
		"amd64 plan 9 dlm",
		FAMD64,
		1,
		&mamd64,
		sizeof(Exec)+8,
		nil,
		commonllp64 },
	{ ELF_MAG,			/* any elf32 or elf64 */
		"elf executable",
		nil,
		FNONE,
		0,
		&mi386,
		sizeof(Ehdr64),
		nil,
		elfdotout },
	{ 0 },
};

Mach	*mach = &mi386;			/* Global current machine table */

static ExecTable*
couldbe4k(ExecTable *mp)
{
	Dir *d;
	ExecTable *f;

	if((d=dirstat("/proc/1/regs")) == nil)
		return mp;
	if(d->length < 32*8){		/* R3000 */
		free(d);
		return mp;
	}
	free(d);
	for (f = exectab; f->magic; f++)
		if(f->magic == M_MAGIC) {
			f->name = "mips plan 9 executable on mips2 kernel";
			return f;
		}
	return mp;
}

int
crackhdr(int fd, Fhdr *fp)
{
	ExecTable *mp;
	ExecHdr d;
	int nb, ret;
	uint32 magic;

	fp->type = FNONE;
	nb = read(fd, (char *)&d.e, sizeof(d.e));
	if (nb <= 0)
		return 0;

	ret = 0;
	magic = beswal(d.e.exechdr.magic);		/* big-endian */
	for (mp = exectab; mp->magic; mp++) {
		if (nb < mp->hsize)
			continue;

		/*
		 * The magic number has morphed into something
		 * with fields (the straw was DYN_MAGIC) so now
		 * a flag is needed in Fhdr to distinguish _MAGIC()
		 * magic numbers from foreign magic numbers.
		 *
		 * This code is creaking a bit and if it has to
		 * be modified/extended much more it's probably
		 * time to step back and redo it all.
		 */
		if(mp->_magic){
			if(mp->magic != (magic & ~DYN_MAGIC))
				continue;

			if(mp->magic == V_MAGIC)
				mp = couldbe4k(mp);

			if ((magic & DYN_MAGIC) && mp->dlmname != nil)
				fp->name = mp->dlmname;
			else
				fp->name = mp->name;
		}
		else{
			if(mp->magic != magic)
				continue;
			fp->name = mp->name;
		}
		fp->type = mp->type;
		fp->hdrsz = mp->hsize;		/* will be zero on bootables */
		fp->_magic = mp->_magic;
		fp->magic = magic;

		mach = mp->mach;
		if(mp->swal != nil)
			hswal(&d, sizeof(d.e)/sizeof(uint32), mp->swal);
		ret = mp->hparse(fd, fp, &d);
		seek(fd, mp->hsize, 0);		/* seek to end of header */
		break;
	}
	if(mp->magic == 0)
		werrstr("unknown header type");
	return ret;
}

/*
 * Convert header to canonical form
 */
static void
hswal(void *v, int n, uint32 (*swap)(uint32))
{
	uint32 *ulp;

	for(ulp = v; n--; ulp++)
		*ulp = (*swap)(*ulp);
}

static void
commonboot(Fhdr *fp)
{
	if (!(fp->entry & mach->ktmask))
		return;

	switch(fp->type) {				/* boot image */
	case FAMD64:
		fp->type = FAMD64B;
		fp->txtaddr = fp->entry;
		fp->name = "amd64 plan 9 boot image";
		fp->dataddr = _round(fp->txtaddr+fp->txtsz, mach->pgsize);
		break;
	default:
		return;
	}
	fp->hdrsz = 0;			/* header stripped */
}

static int
commonllp64(int unused, Fhdr *fp, ExecHdr *hp)
{
	int32 pgsize;
	uvlong entry;

	USED(unused);

	hswal(&hp->e, sizeof(Exec)/sizeof(int32), beswal);
	if(!(hp->e.exechdr.magic & HDR_MAGIC))
		return 0;

	/*
	 * There can be more magic here if the
	 * header ever needs more expansion.
	 * For now just catch use of any of the
	 * unused bits.
	 */
	if((hp->e.exechdr.magic & ~DYN_MAGIC)>>16)
		return 0;
	union {
		char *p;
		uvlong *v;
	} u;
	u.p = (char*)&hp->e.exechdr;
	entry = beswav(*u.v);

	pgsize = mach->pgsize;
	settext(fp, entry, pgsize+fp->hdrsz, hp->e.exechdr.text, fp->hdrsz);
	setdata(fp, _round(pgsize+fp->txtsz+fp->hdrsz, pgsize),
		hp->e.exechdr.data, fp->txtsz+fp->hdrsz, hp->e.exechdr.bss);
	setsym(fp, fp->datoff+fp->datsz, hp->e.exechdr.syms, 0, hp->e.exechdr.spsz, 0, hp->e.exechdr.pcsz);

	if(hp->e.exechdr.magic & DYN_MAGIC) {
		fp->txtaddr = 0;
		fp->dataddr = fp->txtsz;
		return 1;
	}
	commonboot(fp);
	return 1;
}

/*
 * Elf32 and Elf64 binaries.
 */
static int
elf64dotout(int fd, Fhdr *fp, ExecHdr *hp)
{
	uvlong (*swav)(uvlong);
	uint32 (*swal)(uint32);
	ushort (*swab)(ushort);
	Ehdr64 *ep;
	Phdr64 *ph, *pph;
	Shdr64 *sh;
	int i, it, id, is, phsz, shsz;

	/* bitswap the header according to the DATA format */
	ep = &hp->e.elfhdr64;
	if(ep->ident[CLASS] != ELFCLASS64) {
		werrstr("bad ELF class - not 32 bit or 64 bit");
		return 0;
	}
	if(ep->ident[DATA] == ELFDATA2LSB) {
		swab = leswab;
		swal = leswal;
		swav = leswav;
	} else if(ep->ident[DATA] == ELFDATA2MSB) {
		swab = beswab;
		swal = beswal;
		swav = beswav;
	} else {
		werrstr("bad ELF encoding - not big or little endian");
		return 0;
	}

	ep->type = swab(ep->type);
	ep->machine = swab(ep->machine);
	ep->version = swal(ep->version);
	ep->elfentry = swal(ep->elfentry);
	ep->phoff = swav(ep->phoff);
	ep->shoff = swav(ep->shoff);
	ep->flags = swav(ep->flags);
	ep->ehsize = swab(ep->ehsize);
	ep->phentsize = swab(ep->phentsize);
	ep->phnum = swab(ep->phnum);
	ep->shentsize = swab(ep->shentsize);
	ep->shnum = swab(ep->shnum);
	ep->shstrndx = swab(ep->shstrndx);
	if(ep->type != EXEC || ep->version != CURRENT)
		return 0;

	/* we could definitely support a lot more machines here */
	fp->magic = ELF_MAG;
	fp->hdrsz = (ep->ehsize+ep->phnum*ep->phentsize+16)&~15;
	switch(ep->machine) {
	case AMD64:
		mach = &mamd64;
		fp->type = FAMD64;
		break;
	default:
		return 0;
	}

	if(ep->phentsize != sizeof(Phdr64)) {
		werrstr("bad ELF header size");
		return 0;
	}
	phsz = sizeof(Phdr64)*ep->phnum;
	ph = malloc(phsz);
	if(!ph)
		return 0;
	seek(fd, ep->phoff, 0);
	if(read(fd, ph, phsz) < 0) {
		free(ph);
		return 0;
	}
	hswal(ph, phsz/sizeof(uint32), swal);

	shsz = sizeof(Shdr64)*ep->shnum;
	sh = malloc(shsz);
	if(sh) {
		seek(fd, ep->shoff, 0);
		if(read(fd, sh, shsz) < 0) {
			free(sh);
			sh = 0;
		} else
			hswal(sh, shsz/sizeof(uint32), swal);
	}

	/* find text, data and symbols and install them */
	it = id = is = -1;
	for(i = 0; i < ep->phnum; i++) {
		if(ph[i].type == LOAD
		&& (ph[i].flags & (R|X)) == (R|X) && it == -1)
			it = i;
		else if(ph[i].type == LOAD
		&& (ph[i].flags & (R|W)) == (R|W) && id == -1)
			id = i;
		else if(ph[i].type == NOPTYPE && is == -1)
			is = i;
	}
	if(it == -1 || id == -1) {
		/*
		 * The SPARC64 boot image is something of an ELF hack.
		 * Text+Data+BSS are represented by ph[0].  Symbols
		 * are represented by ph[1]:
		 *
		 *		filesz, memsz, vaddr, paddr, off
		 * ph[0] : txtsz+datsz, txtsz+datsz+bsssz, txtaddr-KZERO, datasize, txtoff
		 * ph[1] : symsz, lcsz, 0, 0, symoff
		 */
		if(ep->machine == SPARC64 && ep->phnum == 2) {
			uint32 txtaddr, txtsz, dataddr, bsssz;

			txtaddr = ph[0].vaddr | 0x80000000;
			txtsz = ph[0].filesz - ph[0].paddr;
			dataddr = txtaddr + txtsz;
			bsssz = ph[0].memsz - ph[0].filesz;
			settext(fp, ep->elfentry | 0x80000000, txtaddr, txtsz, ph[0].offset);
			setdata(fp, dataddr, ph[0].paddr, ph[0].offset + txtsz, bsssz);
			setsym(fp, ph[1].offset, ph[1].filesz, 0, 0, 0, ph[1].memsz);
			free(ph);
			return 1;
		}

		werrstr("No TEXT or DATA sections");
		free(ph);
		free(sh);
		return 0;
	}

	settext(fp, ep->elfentry, ph[it].vaddr, ph[it].memsz, ph[it].offset);
	pph = ph + id;
	setdata(fp, pph->vaddr, pph->filesz, pph->offset, pph->memsz - pph->filesz);
	if(is != -1)
		setsym(fp, ph[is].offset, ph[is].filesz, 0, 0, 0, ph[is].memsz);
	else if(sh != 0){
		char *buf;
		uvlong symsize = 0;
		uvlong symoff = 0;
		uvlong pclnsz = 0;
		uvlong pclnoff = 0;

		/* load shstrtab names */
		buf = malloc(sh[ep->shstrndx].size);
		if (buf == 0)
			goto done;
		memset(buf, 0, sh[ep->shstrndx].size);
		seek(fd, sh[ep->shstrndx].offset, 0);
		i = read(fd, buf, sh[ep->shstrndx].size);
		USED(i);	// shut up ubuntu gcc

		for(i = 0; i < ep->shnum; i++) {
			if (strcmp(&buf[sh[i].name], ".gosymtab") == 0) {
				symsize = sh[i].size;
				symoff = sh[i].offset;
			}
			if (strcmp(&buf[sh[i].name], ".gopclntab") == 0) {
				pclnsz = sh[i].size;
				pclnoff = sh[i].offset;
			}
		}
		setsym(fp, symoff, symsize, 0, 0, pclnoff, pclnsz);
		free(buf);
	}
done:
	free(ph);
	free(sh);
	return 1;
}

static int
elfdotout(int fd, Fhdr *fp, ExecHdr *hp)
{

	uint32 (*swal)(uint32);
	ushort (*swab)(ushort);
	Ehdr32 *ep;
	Phdr32 *ph;
	int i, it, id, is, phsz, shsz;
	Shdr32 *sh;

	/* bitswap the header according to the DATA format */
	ep = &hp->e.elfhdr32;
	if(ep->ident[CLASS] != ELFCLASS32) {
		return elf64dotout(fd, fp, hp);
	}
	if(ep->ident[DATA] == ELFDATA2LSB) {
		swab = leswab;
		swal = leswal;
	} else if(ep->ident[DATA] == ELFDATA2MSB) {
		swab = beswab;
		swal = beswal;
	} else {
		werrstr("bad ELF encoding - not big or little endian");
		return 0;
	}

	ep->type = swab(ep->type);
	ep->machine = swab(ep->machine);
	ep->version = swal(ep->version);
	ep->elfentry = swal(ep->elfentry);
	ep->phoff = swal(ep->phoff);
	ep->shoff = swal(ep->shoff);
	ep->flags = swal(ep->flags);
	ep->ehsize = swab(ep->ehsize);
	ep->phentsize = swab(ep->phentsize);
	ep->phnum = swab(ep->phnum);
	ep->shentsize = swab(ep->shentsize);
	ep->shnum = swab(ep->shnum);
	ep->shstrndx = swab(ep->shstrndx);
	if(ep->type != EXEC || ep->version != CURRENT)
		return 0;

	/* we could definitely support a lot more machines here */
	fp->magic = ELF_MAG;
	fp->hdrsz = (ep->ehsize+ep->phnum*ep->phentsize+16)&~15;
	switch(ep->machine) {
	case I386:
		mach = &mi386;
		fp->type = FI386;
		break;
	default:
		return 0;
	}

	if(ep->phentsize != sizeof(Phdr32)) {
		werrstr("bad ELF header size");
		return 0;
	}
	phsz = sizeof(Phdr32)*ep->phnum;
	ph = malloc(phsz);
	if(!ph)
		return 0;
	seek(fd, ep->phoff, 0);
	if(read(fd, ph, phsz) < 0) {
		free(ph);
		return 0;
	}
	hswal(ph, phsz/sizeof(uint32), swal);

	shsz = sizeof(Shdr32)*ep->shnum;
	sh = malloc(shsz);
	if(sh) {
		seek(fd, ep->shoff, 0);
		if(read(fd, sh, shsz) < 0) {
			free(sh);
			sh = 0;
		} else
			hswal(sh, shsz/sizeof(uint32), swal);
	}

	/* find text, data and symbols and install them */
	it = id = is = -1;
	for(i = 0; i < ep->phnum; i++) {
		if(ph[i].type == LOAD
		&& (ph[i].flags & (R|X)) == (R|X) && it == -1)
			it = i;
		else if(ph[i].type == LOAD
		&& (ph[i].flags & (R|W)) == (R|W) && id == -1)
			id = i;
		else if(ph[i].type == NOPTYPE && is == -1)
			is = i;
	}
	if(it == -1 || id == -1) {
		/*
		 * The SPARC64 boot image is something of an ELF hack.
		 * Text+Data+BSS are represented by ph[0].  Symbols
		 * are represented by ph[1]:
		 *
		 *		filesz, memsz, vaddr, paddr, off
		 * ph[0] : txtsz+datsz, txtsz+datsz+bsssz, txtaddr-KZERO, datasize, txtoff
		 * ph[1] : symsz, lcsz, 0, 0, symoff
		 */
		if(ep->machine == SPARC64 && ep->phnum == 2) {
			uint32 txtaddr, txtsz, dataddr, bsssz;

			txtaddr = ph[0].vaddr | 0x80000000;
			txtsz = ph[0].filesz - ph[0].paddr;
			dataddr = txtaddr + txtsz;
			bsssz = ph[0].memsz - ph[0].filesz;
			settext(fp, ep->elfentry | 0x80000000, txtaddr, txtsz, ph[0].offset);
			setdata(fp, dataddr, ph[0].paddr, ph[0].offset + txtsz, bsssz);
			setsym(fp, ph[1].offset, ph[1].filesz, 0, 0, 0, ph[1].memsz);
			free(ph);
			return 1;
		}

		werrstr("No TEXT or DATA sections");
		free(sh);
		free(ph);
		return 0;
	}

	settext(fp, ep->elfentry, ph[it].vaddr, ph[it].memsz, ph[it].offset);
	setdata(fp, ph[id].vaddr, ph[id].filesz, ph[id].offset, ph[id].memsz - ph[id].filesz);
	if(is != -1)
		setsym(fp, ph[is].offset, ph[is].filesz, 0, 0, 0, ph[is].memsz);
	else if(sh != 0){
		char *buf;
		uvlong symsize = 0;
		uvlong symoff = 0;
		uvlong pclnsize = 0;
		uvlong pclnoff = 0;

		/* load shstrtab names */
		buf = malloc(sh[ep->shstrndx].size);
		if (buf == 0)
			goto done;
		memset(buf, 0, sh[ep->shstrndx].size);
		seek(fd, sh[ep->shstrndx].offset, 0);
		i = read(fd, buf, sh[ep->shstrndx].size);
		USED(i);	// shut up ubuntu gcc

		for(i = 0; i < ep->shnum; i++) {
			if (strcmp(&buf[sh[i].name], ".gosymtab") == 0) {
				symsize = sh[i].size;
				symoff = sh[i].offset;
			}
			if (strcmp(&buf[sh[i].name], ".gopclntab") == 0) {
				pclnsize = sh[i].size;
				pclnoff = sh[i].offset;
			}
		}
		setsym(fp, symoff, symsize, 0, 0, pclnoff, pclnsize);
		free(buf);
	}
done:
	free(sh);
	free(ph);
	return 1;
}

/*
 * Structures needed to parse PE image.
 */
typedef struct {
	uint16 Machine;
	uint16 NumberOfSections;
	uint32 TimeDateStamp;
	uint32 PointerToSymbolTable;
	uint32 NumberOfSymbols;
	uint16 SizeOfOptionalHeader;
	uint16 Characteristics;
} IMAGE_FILE_HEADER;

typedef struct {
	uint8  Name[8];
	uint32 VirtualSize;
	uint32 VirtualAddress;
	uint32 SizeOfRawData;
	uint32 PointerToRawData;
	uint32 PointerToRelocations;
	uint32 PointerToLineNumbers;
	uint16 NumberOfRelocations;
	uint16 NumberOfLineNumbers;
	uint32 Characteristics;
} IMAGE_SECTION_HEADER;

typedef struct {
	uint32 VirtualAddress;
	uint32 Size;
} IMAGE_DATA_DIRECTORY;

typedef struct {
	uint16 Magic;
	uint8  MajorLinkerVersion;
	uint8  MinorLinkerVersion;
	uint32 SizeOfCode;
	uint32 SizeOfInitializedData;
	uint32 SizeOfUninitializedData;
	uint32 AddressOfEntryPoint;
	uint32 BaseOfCode;
	uint32 BaseOfData;
	uint32 ImageBase;
	uint32 SectionAlignment;
	uint32 FileAlignment;
	uint16 MajorOperatingSystemVersion;
	uint16 MinorOperatingSystemVersion;
	uint16 MajorImageVersion;
	uint16 MinorImageVersion;
	uint16 MajorSubsystemVersion;
	uint16 MinorSubsystemVersion;
	uint32 Win32VersionValue;
	uint32 SizeOfImage;
	uint32 SizeOfHeaders;
	uint32 CheckSum;
	uint16 Subsystem;
	uint16 DllCharacteristics;
	uint32 SizeOfStackReserve;
	uint32 SizeOfStackCommit;
	uint32 SizeOfHeapReserve;
	uint32 SizeOfHeapCommit;
	uint32 LoaderFlags;
	uint32 NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[16];
} IMAGE_OPTIONAL_HEADER;

typedef struct {
	uint16 Magic;
	uint8  MajorLinkerVersion;
	uint8  MinorLinkerVersion;
	uint32 SizeOfCode;
	uint32 SizeOfInitializedData;
	uint32 SizeOfUninitializedData;
	uint32 AddressOfEntryPoint;
	uint32 BaseOfCode;
	uint64 ImageBase;
	uint32 SectionAlignment;
	uint32 FileAlignment;
	uint16 MajorOperatingSystemVersion;
	uint16 MinorOperatingSystemVersion;
	uint16 MajorImageVersion;
	uint16 MinorImageVersion;
	uint16 MajorSubsystemVersion;
	uint16 MinorSubsystemVersion;
	uint32 Win32VersionValue;
	uint32 SizeOfImage;
	uint32 SizeOfHeaders;
	uint32 CheckSum;
	uint16 Subsystem;
	uint16 DllCharacteristics;
	uint64 SizeOfStackReserve;
	uint64 SizeOfStackCommit;
	uint64 SizeOfHeapReserve;
	uint64 SizeOfHeapCommit;
	uint32 LoaderFlags;
	uint32 NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[16];
} PE64_IMAGE_OPTIONAL_HEADER;

static void
settext(Fhdr *fp, uvlong e, uvlong a, int32 s, vlong off)
{
	fp->txtaddr = a;
	fp->entry = e;
	fp->txtsz = s;
	fp->txtoff = off;
}

static void
setdata(Fhdr *fp, uvlong a, int32 s, vlong off, int32 bss)
{
	fp->dataddr = a;
	fp->datsz = s;
	fp->datoff = off;
	fp->bsssz = bss;
}

static void
setsym(Fhdr *fp, vlong symoff, int32 symsz, vlong sppcoff, int32 sppcsz, vlong lnpcoff, int32 lnpcsz)
{
	fp->symoff = symoff;
	fp->symsz = symsz;
	
	if(sppcoff == 0)
		sppcoff = symoff+symsz;
	fp->sppcoff = symoff;
	fp->sppcsz = sppcsz;

	if(lnpcoff == 0)
		lnpcoff = sppcoff + sppcsz;
	fp->lnpcoff = lnpcoff;
	fp->lnpcsz = lnpcsz;
}

static uvlong
_round(uvlong a, uint32 b)
{
	uvlong w;

	w = (a/b)*b;
	if (a!=w)
		w += b;
	return(w);
}
