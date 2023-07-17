#include	<u.h>

#include	<libc.h>

#include	"go.h"

// pkgnotused import 导入了包但未被使用则报错.
//
// 	@param lineno: 导入但未被使用的 package 所在行号
// 	@param path: 导入但未被使用的 package 路径, 如"github.com/golang/sys/unix"
// 	@param name: package 被导入时可能会指定的别名, 如 import(gounix "github.com/golang/sys/unix")
//
// caller:
// 	1. mkpackage() 只有这一处
static void pkgnotused(int lineno, Strlit *path, char *name)
{
	char *elem;

	// If the package was imported with a name other than the final
	// import path element, show it explicitly in the error message.
	// Note that this handles both renamed imports and imports of
	// packages containing unconventional package declarations.
	// Note that this uses / always, even on Windows, because Go import
	// paths always use forward slashes.
	elem = strrchr(path->s, '/');
	if(elem != nil) {
		elem++;
	}
	else {
		elem = path->s;
	}
	if(name == nil || strcmp(elem, name) == 0) {
		if (!nostrictmode) {
			yyerrorl(lineno, "imported and not used: \"%Z\"", path);
		} else {
			mywarn("imported and not used: \"%Z\"\n", path);
		}
	}
	else {
		if (!nostrictmode) {
			yyerrorl(lineno, "imported and not used: \"%Z\" as %s", path, name);
		} else {
			mywarn("imported and not used: \"%Z\" as %s\n", path, name);
		}
	}
}

// 	@param pkgname: 一般来说, 该参数为"main", 表示待编译程序的入口函数所在的 package.
//
// caller:
// 	1. main()
void mkpackage(char* pkgname)
{
	Sym *s;
	int32 h;
	char *p;

	if(localpkg->name == nil) {
		if(strcmp(pkgname, "_") == 0) {
			yyerror("invalid package name _");
		}
		localpkg->name = pkgname;
	} else {
		if(strcmp(pkgname, localpkg->name) != 0) {
			yyerror("package %s; expected %s", pkgname, localpkg->name);
		}
		for(h=0; h<NHASH; h++) {
			for(s = hash[h]; s != S; s = s->link) {
				if(s->def == N || s->pkg != localpkg) {
					continue;
				}
				if(s->def->op == OPACK) {
					// throw away top-level package name leftover
					// from previous file.
					// leave s->block set to cause redeclaration
					// errors if a conflicting top-level name is
					// introduced by a different file.
					if(!s->def->used && !nsyntaxerrors) {
						pkgnotused(s->def->lineno, s->def->pkg->path, s->name);
					}
					s->def = N;
					continue;
				}
				if(s->def->sym != s) {
					// throw away top-level name left over
					// from previous import . "x"
					if(s->def->pack != N && !s->def->pack->used && !nsyntaxerrors) {
						pkgnotused(s->def->pack->lineno, s->def->pack->pkg->path, nil);
						s->def->pack->used = 1;
					}
					s->def = N;
					continue;
				}
			}
		}
	}

	if(outfile == nil) {
		p = strrchr(infile, '/');

		if(p == nil) {
			p = infile;
		}
		else {
			p = p+1;
		}
		snprint(namebuf, sizeof(namebuf), "%s", p);
		p = strrchr(namebuf, '.');
		if(p != nil) {
			*p = 0;
		}
		outfile = smprint("%s.%c", namebuf, thechar);
	}
}
