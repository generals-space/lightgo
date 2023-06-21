/* A Bison parser, made by GNU Bison 2.5.  */

// 貌似本文件就是 go.y 文件编译而来的, go.y 用特殊格式声明了语法规则, y.tab.c 则是可编译的 c 文件.

/* Bison implementation for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.5"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 268 of yacc.c  */
#line 24 "go.y"

#include <u.h>
#include <stdio.h>	/* if we don't, bison will, and go.h re-#defines getc */
#include <libc.h>
#include "go.h"

static void fixlbrace(int);


/* Line 268 of yacc.c  */
#line 81 "y.tab.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     LLITERAL = 258,
     LASOP = 259,
     LCOLAS = 260,
     LBREAK = 261,
     LCASE = 262,
     LCHAN = 263,
     LCONST = 264,
     LCONTINUE = 265,
     LDDD = 266,
     LDEFAULT = 267,
     LDEFER = 268,
     LELSE = 269,
     LFALL = 270,
     LFOR = 271,
     LFUNC = 272,
     LGO = 273,
     LGOTO = 274,
     LIF = 275,
     LIMPORT = 276,
     LINTERFACE = 277,
     LMAP = 278,
     LNAME = 279,
     LPACKAGE = 280,
     LRANGE = 281,
     LRETURN = 282,
     LSELECT = 283,
     LSTRUCT = 284,
     LSWITCH = 285,
     LTYPE = 286,
     LVAR = 287,
     LANDAND = 288,
     LANDNOT = 289,
     LBODY = 290,
     LCOMM = 291,
     LDEC = 292,
     LEQ = 293,
     LGE = 294,
     LGT = 295,
     LIGNORE = 296,
     LINC = 297,
     LLE = 298,
     LLSH = 299,
     LLT = 300,
     LNE = 301,
     LOROR = 302,
     LRSH = 303,
     NotPackage = 304,
     NotParen = 305,
     PreferToRightParen = 306
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 293 of yacc.c  */
#line 32 "go.y"

	Node*		node;
	NodeList*		list;
	Type*		type;
	Sym*		sym;
	struct	Val	val;
	int		i;



/* Line 293 of yacc.c  */
#line 179 "y.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 343 of yacc.c  */
#line 191 "y.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  4
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2305

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  76
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  142
/* YYNRULES -- Number of rules.  */
#define YYNRULES  352
/* YYNRULES -- Number of states.  */
#define YYNSTATES  669

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   306

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    69,     2,     2,    64,    55,    56,     2,
      59,    60,    53,    49,    75,    50,    63,    54,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    66,    62,
       2,    65,     2,    73,    74,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    71,     2,    72,    52,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    67,    51,    68,    70,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    57,    58,    61
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     8,     9,    13,    14,    18,    19,    23,
      26,    32,    36,    40,    43,    45,    49,    51,    54,    57,
      62,    63,    65,    66,    71,    72,    74,    76,    78,    80,
      83,    89,    93,    96,   102,   110,   114,   117,   123,   127,
     129,   132,   137,   141,   146,   150,   152,   155,   157,   159,
     162,   166,   168,   172,   176,   180,   183,   186,   190,   196,
     202,   205,   206,   211,   212,   216,   217,   220,   221,   226,
     231,   236,   242,   244,   246,   249,   250,   254,   256,   260,
     261,   262,   263,   272,   273,   279,   280,   283,   284,   287,
     288,   289,   297,   298,   304,   306,   310,   314,   318,   322,
     326,   330,   334,   338,   342,   346,   350,   354,   358,   362,
     366,   370,   374,   378,   382,   386,   388,   391,   394,   397,
     400,   403,   406,   409,   412,   416,   422,   429,   431,   433,
     437,   443,   449,   454,   461,   470,   472,   478,   484,   490,
     498,   500,   501,   505,   507,   512,   514,   519,   521,   525,
     527,   529,   531,   533,   535,   537,   539,   540,   542,   544,
     546,   548,   553,   558,   560,   562,   564,   567,   569,   571,
     573,   575,   577,   581,   583,   585,   587,   590,   592,   594,
     596,   598,   602,   604,   606,   608,   610,   612,   614,   616,
     618,   620,   624,   629,   634,   637,   641,   647,   649,   651,
     654,   658,   664,   668,   674,   678,   682,   688,   697,   703,
     712,   718,   719,   723,   724,   726,   730,   732,   737,   740,
     741,   745,   747,   751,   753,   757,   759,   763,   765,   769,
     771,   775,   779,   782,   787,   791,   797,   803,   805,   809,
     811,   814,   816,   820,   825,   827,   830,   833,   835,   837,
     841,   842,   845,   846,   848,   850,   852,   854,   856,   858,
     860,   862,   864,   865,   870,   872,   875,   878,   881,   884,
     887,   890,   892,   896,   898,   902,   904,   908,   910,   914,
     916,   920,   922,   924,   928,   932,   933,   936,   937,   939,
     940,   942,   943,   945,   946,   948,   949,   951,   952,   954,
     955,   957,   958,   960,   961,   963,   968,   973,   979,   986,
     991,   996,   998,  1000,  1002,  1004,  1006,  1008,  1010,  1012,
    1014,  1018,  1023,  1029,  1034,  1039,  1042,  1045,  1050,  1054,
    1058,  1064,  1068,  1073,  1077,  1083,  1085,  1086,  1088,  1092,
    1094,  1096,  1099,  1101,  1103,  1109,  1110,  1113,  1115,  1119,
    1121,  1125,  1127
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      77,     0,    -1,    79,    78,    81,   166,    -1,    -1,    25,
     141,    62,    -1,    -1,    80,    86,    88,    -1,    -1,    81,
      82,    62,    -1,    21,    83,    -1,    21,    59,    84,   190,
      60,    -1,    21,    59,    60,    -1,    85,    86,    88,    -1,
      85,    88,    -1,    83,    -1,    84,    62,    83,    -1,     3,
      -1,   141,     3,    -1,    63,     3,    -1,    25,    24,    87,
      62,    -1,    -1,    24,    -1,    -1,    89,   214,    64,    64,
      -1,    -1,    91,    -1,   158,    -1,   181,    -1,     1,    -1,
      32,    93,    -1,    32,    59,   167,   190,    60,    -1,    32,
      59,    60,    -1,    92,    94,    -1,    92,    59,    94,   190,
      60,    -1,    92,    59,    94,    62,   168,   190,    60,    -1,
      92,    59,    60,    -1,    31,    97,    -1,    31,    59,   169,
     190,    60,    -1,    31,    59,    60,    -1,     9,    -1,   185,
     146,    -1,   185,   146,    65,   186,    -1,   185,    65,   186,
      -1,   185,   146,    65,   186,    -1,   185,    65,   186,    -1,
      94,    -1,   185,   146,    -1,   185,    -1,   141,    -1,    96,
     146,    -1,    96,    65,   146,    -1,   126,    -1,   126,     4,
     126,    -1,   186,    65,   186,    -1,   186,     5,   186,    -1,
     126,    42,    -1,   126,    37,    -1,     7,   187,    66,    -1,
       7,   187,    65,   126,    66,    -1,     7,   187,     5,   126,
      66,    -1,    12,    66,    -1,    -1,    67,   101,   183,    68,
      -1,    -1,    99,   103,   183,    -1,    -1,   104,   102,    -1,
      -1,    35,   106,   183,    68,    -1,   186,    65,    26,   126,
      -1,   186,     5,    26,   126,    -1,   194,    62,   194,    62,
     194,    -1,   194,    -1,   107,    -1,   108,   105,    -1,    -1,
      16,   111,   109,    -1,   194,    -1,   194,    62,   194,    -1,
      -1,    -1,    -1,    20,   114,   112,   115,   105,   116,   119,
     120,    -1,    -1,    14,    20,   118,   112,   105,    -1,    -1,
     119,   117,    -1,    -1,    14,   100,    -1,    -1,    -1,    30,
     122,   112,   123,    35,   104,    68,    -1,    -1,    28,   125,
      35,   104,    68,    -1,   127,    -1,   126,    47,   126,    -1,
     126,    33,   126,    -1,   126,    38,   126,    -1,   126,    46,
     126,    -1,   126,    45,   126,    -1,   126,    43,   126,    -1,
     126,    39,   126,    -1,   126,    40,   126,    -1,   126,    49,
     126,    -1,   126,    50,   126,    -1,   126,    51,   126,    -1,
     126,    52,   126,    -1,   126,    53,   126,    -1,   126,    54,
     126,    -1,   126,    55,   126,    -1,   126,    56,   126,    -1,
     126,    34,   126,    -1,   126,    44,   126,    -1,   126,    48,
     126,    -1,   126,    36,   126,    -1,   134,    -1,    53,   127,
      -1,    56,   127,    -1,    49,   127,    -1,    50,   127,    -1,
      69,   127,    -1,    70,   127,    -1,    52,   127,    -1,    36,
     127,    -1,   134,    59,    60,    -1,   134,    59,   187,   191,
      60,    -1,   134,    59,   187,    11,   191,    60,    -1,     3,
      -1,   143,    -1,   134,    63,   141,    -1,   134,    63,    59,
     135,    60,    -1,   134,    63,    59,    31,    60,    -1,   134,
      71,   126,    72,    -1,   134,    71,   192,    66,   192,    72,
      -1,   134,    71,   192,    66,   192,    66,   192,    72,    -1,
     128,    -1,   149,    59,   126,   191,    60,    -1,   150,   137,
     130,   189,    68,    -1,   129,    67,   130,   189,    68,    -1,
      59,   135,    60,    67,   130,   189,    68,    -1,   165,    -1,
      -1,   126,    66,   133,    -1,   126,    -1,    67,   130,   189,
      68,    -1,   126,    -1,    67,   130,   189,    68,    -1,   129,
      -1,    59,   135,    60,    -1,   126,    -1,   147,    -1,   146,
      -1,    35,    -1,    67,    -1,   141,    -1,   141,    -1,    -1,
     138,    -1,    24,    -1,   142,    -1,    73,    -1,    74,     3,
      63,    24,    -1,    74,     3,    63,    73,    -1,   141,    -1,
     138,    -1,    11,    -1,    11,   146,    -1,   155,    -1,   161,
      -1,   153,    -1,   154,    -1,   152,    -1,    59,   146,    60,
      -1,   155,    -1,   161,    -1,   153,    -1,    53,   147,    -1,
     161,    -1,   153,    -1,   154,    -1,   152,    -1,    59,   146,
      60,    -1,   161,    -1,   153,    -1,   153,    -1,   155,    -1,
     161,    -1,   153,    -1,   154,    -1,   152,    -1,   143,    -1,
     143,    63,   141,    -1,    71,   192,    72,   146,    -1,    71,
      11,    72,   146,    -1,     8,   148,    -1,     8,    36,   146,
      -1,    23,    71,   146,    72,   146,    -1,   156,    -1,   157,
      -1,    53,   146,    -1,    36,     8,   146,    -1,    29,   137,
     170,   190,    68,    -1,    29,   137,    68,    -1,    22,   137,
     171,   190,    68,    -1,    22,   137,    68,    -1,    17,   159,
     162,    -1,   141,    59,   179,    60,   163,    -1,    59,   179,
      60,   141,    59,   179,    60,   163,    -1,   200,    59,   195,
      60,   210,    -1,    59,   215,    60,   141,    59,   195,    60,
     210,    -1,    17,    59,   179,    60,   163,    -1,    -1,    67,
     183,    68,    -1,    -1,   151,    -1,    59,   179,    60,    -1,
     161,    -1,   164,   137,   183,    68,    -1,   164,     1,    -1,
      -1,   166,    90,    62,    -1,    93,    -1,   167,    62,    93,
      -1,    95,    -1,   168,    62,    95,    -1,    97,    -1,   169,
      62,    97,    -1,   172,    -1,   170,    62,   172,    -1,   175,
      -1,   171,    62,   175,    -1,   184,   146,   198,    -1,   174,
     198,    -1,    59,   174,    60,   198,    -1,    53,   174,   198,
      -1,    59,    53,   174,    60,   198,    -1,    53,    59,   174,
      60,   198,    -1,    24,    -1,    24,    63,   141,    -1,   173,
      -1,   138,   176,    -1,   173,    -1,    59,   173,    60,    -1,
      59,   179,    60,   163,    -1,   136,    -1,   141,   136,    -1,
     141,   145,    -1,   145,    -1,   177,    -1,   178,    75,   177,
      -1,    -1,   178,   191,    -1,    -1,   100,    -1,    91,    -1,
     181,    -1,     1,    -1,    98,    -1,   110,    -1,   121,    -1,
     124,    -1,   113,    -1,    -1,   144,    66,   182,   180,    -1,
      15,    -1,     6,   140,    -1,    10,   140,    -1,    18,   128,
      -1,    13,   128,    -1,    19,   138,    -1,    27,   193,    -1,
     180,    -1,   183,    62,   180,    -1,   138,    -1,   184,    75,
     138,    -1,   139,    -1,   185,    75,   139,    -1,   126,    -1,
     186,    75,   126,    -1,   135,    -1,   187,    75,   135,    -1,
     131,    -1,   132,    -1,   188,    75,   131,    -1,   188,    75,
     132,    -1,    -1,   188,   191,    -1,    -1,    62,    -1,    -1,
      75,    -1,    -1,   126,    -1,    -1,   186,    -1,    -1,    98,
      -1,    -1,   215,    -1,    -1,   216,    -1,    -1,   217,    -1,
      -1,     3,    -1,    21,    24,     3,    62,    -1,    32,   200,
     202,    62,    -1,     9,   200,    65,   213,    62,    -1,     9,
     200,   202,    65,   213,    62,    -1,    31,   201,   202,    62,
      -1,    17,   160,   162,    62,    -1,   142,    -1,   200,    -1,
     204,    -1,   205,    -1,   206,    -1,   204,    -1,   206,    -1,
     142,    -1,    24,    -1,    71,    72,   202,    -1,    71,     3,
      72,   202,    -1,    23,    71,   202,    72,   202,    -1,    29,
      67,   196,    68,    -1,    22,    67,   197,    68,    -1,    53,
     202,    -1,     8,   203,    -1,     8,    59,   205,    60,    -1,
       8,    36,   202,    -1,    36,     8,   202,    -1,    17,    59,
     195,    60,   210,    -1,   141,   202,   198,    -1,   141,    11,
     202,   198,    -1,   141,   202,   198,    -1,   141,    59,   195,
      60,   210,    -1,   202,    -1,    -1,   211,    -1,    59,   195,
      60,    -1,   202,    -1,     3,    -1,    50,     3,    -1,   141,
      -1,   212,    -1,    59,   212,    49,   212,    60,    -1,    -1,
     214,   199,    -1,   207,    -1,   215,    75,   207,    -1,   208,
      -1,   216,    62,   208,    -1,   209,    -1,   217,    62,   209,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   128,   128,   137,   143,   154,   154,   169,   170,   173,
     174,   175,   178,   215,   226,   227,   230,   237,   244,   253,
     267,   268,   275,   275,   288,   292,   293,   297,   302,   313,
     321,   327,   331,   337,   343,   349,   355,   363,   367,   373,
     382,   387,   392,   398,   402,   408,   409,   413,   419,   431,
     436,   442,   460,   465,   477,   493,   498,   505,   525,   543,
     552,   571,   570,   585,   584,   615,   618,   625,   624,   635,
     641,   650,   661,   667,   670,   678,   677,   688,   694,   706,
     710,   715,   705,   736,   735,   748,   751,   757,   760,   772,
     776,   771,   794,   793,   809,   810,   814,   818,   822,   826,
     830,   834,   838,   842,   846,   850,   854,   858,   862,   866,
     870,   874,   878,   882,   887,   893,   894,   898,   909,   913,
     917,   921,   926,   930,   940,   944,   949,   957,   961,   962,
     973,   977,   981,   985,   989,   997,   998,  1004,  1011,  1017,
    1024,  1027,  1034,  1040,  1057,  1064,  1065,  1072,  1073,  1092,
    1093,  1096,  1099,  1103,  1114,  1123,  1129,  1132,  1135,  1142,
    1143,  1149,  1162,  1177,  1185,  1197,  1202,  1208,  1209,  1210,
    1211,  1212,  1213,  1219,  1220,  1221,  1222,  1228,  1229,  1230,
    1231,  1232,  1238,  1239,  1242,  1245,  1246,  1247,  1248,  1249,
    1252,  1253,  1266,  1270,  1275,  1280,  1285,  1289,  1290,  1293,
    1299,  1306,  1312,  1319,  1325,  1336,  1350,  1379,  1419,  1444,
    1462,  1471,  1474,  1482,  1486,  1490,  1497,  1503,  1508,  1520,
    1523,  1539,  1541,  1547,  1548,  1554,  1558,  1564,  1565,  1571,
    1575,  1581,  1604,  1609,  1615,  1621,  1628,  1637,  1646,  1661,
    1667,  1672,  1676,  1683,  1696,  1697,  1703,  1709,  1712,  1716,
    1722,  1725,  1734,  1737,  1738,  1742,  1743,  1749,  1750,  1751,
    1752,  1753,  1755,  1754,  1769,  1774,  1778,  1782,  1786,  1790,
    1795,  1814,  1820,  1828,  1832,  1838,  1842,  1848,  1852,  1858,
    1862,  1871,  1875,  1879,  1883,  1889,  1892,  1900,  1901,  1903,
    1904,  1907,  1910,  1913,  1916,  1919,  1922,  1925,  1928,  1931,
    1934,  1937,  1940,  1943,  1946,  1952,  1956,  1960,  1964,  1968,
    1972,  1992,  1999,  2010,  2011,  2012,  2015,  2016,  2019,  2023,
    2033,  2037,  2041,  2045,  2049,  2053,  2057,  2063,  2069,  2077,
    2085,  2091,  2098,  2114,  2136,  2140,  2146,  2149,  2152,  2156,
    2166,  2170,  2185,  2193,  2194,  2206,  2207,  2210,  2214,  2220,
    2224,  2230,  2234
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
const char *yytname[] =
{
  "$end", "error", "$undefined", "LLITERAL", "LASOP", "LCOLAS", "LBREAK",
  "LCASE", "LCHAN", "LCONST", "LCONTINUE", "LDDD", "LDEFAULT", "LDEFER",
  "LELSE", "LFALL", "LFOR", "LFUNC", "LGO", "LGOTO", "LIF", "LIMPORT",
  "LINTERFACE", "LMAP", "LNAME", "LPACKAGE", "LRANGE", "LRETURN",
  "LSELECT", "LSTRUCT", "LSWITCH", "LTYPE", "LVAR", "LANDAND", "LANDNOT",
  "LBODY", "LCOMM", "LDEC", "LEQ", "LGE", "LGT", "LIGNORE", "LINC", "LLE",
  "LLSH", "LLT", "LNE", "LOROR", "LRSH", "'+'", "'-'", "'|'", "'^'", "'*'",
  "'/'", "'%'", "'&'", "NotPackage", "NotParen", "'('", "')'",
  "PreferToRightParen", "';'", "'.'", "'$'", "'='", "':'", "'{'", "'}'",
  "'!'", "'~'", "'['", "']'", "'?'", "'@'", "','", "$accept", "file",
  "package", "loadsys", "$@1", "imports", "import", "import_stmt",
  "import_stmt_list", "import_here", "import_package", "import_safety",
  "import_there", "$@2", "xdcl", "common_dcl", "lconst", "vardcl",
  "constdcl", "constdcl1", "typedclname", "typedcl", "simple_stmt", "case",
  "compound_stmt", "$@3", "caseblock", "$@4", "caseblock_list",
  "loop_body", "$@5", "range_stmt", "for_header", "for_body", "for_stmt",
  "$@6", "if_header", "if_stmt", "$@7", "$@8", "$@9", "elseif", "$@10",
  "elseif_list", "else", "switch_stmt", "$@11", "$@12", "select_stmt",
  "$@13", "expr", "uexpr", "pseudocall", "pexpr_no_paren", "start_complit",
  "keyval", "bare_complitexpr", "complitexpr", "pexpr", "expr_or_type",
  "name_or_type", "lbrace", "new_name", "dcl_name", "onew_name", "sym",
  "hidden_importsym", "name", "labelname", "dotdotdot", "ntype",
  "non_expr_type", "non_recvchantype", "convtype", "comptype",
  "fnret_type", "dotname", "othertype", "ptrtype", "recvchantype",
  "structtype", "interfacetype", "xfndcl", "fndcl", "hidden_fndcl",
  "fntype", "fnbody", "fnres", "fnlitdcl", "fnliteral", "xdcl_list",
  "vardcl_list", "constdcl_list", "typedcl_list", "structdcl_list",
  "interfacedcl_list", "structdcl", "packname", "embed", "interfacedcl",
  "indcl", "arg_type", "arg_type_list", "oarg_type_list_ocomma", "stmt",
  "non_dcl_stmt", "$@14", "stmt_list", "new_name_list", "dcl_name_list",
  "expr_list", "expr_or_type_list", "keyval_list", "braced_keyval_list",
  "osemi", "ocomma", "oexpr", "oexpr_list", "osimple_stmt",
  "ohidden_funarg_list", "ohidden_structdcl_list",
  "ohidden_interfacedcl_list", "oliteral", "hidden_import",
  "hidden_pkg_importsym", "hidden_pkgtype", "hidden_type",
  "hidden_type_non_recv_chan", "hidden_type_misc", "hidden_type_recv_chan",
  "hidden_type_func", "hidden_funarg", "hidden_structdcl",
  "hidden_interfacedcl", "ohidden_funres", "hidden_funres",
  "hidden_literal", "hidden_constant", "hidden_import_list",
  "hidden_funarg_list", "hidden_structdcl_list",
  "hidden_interfacedcl_list", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,    43,
      45,   124,    94,    42,    47,    37,    38,   304,   305,    40,
      41,   306,    59,    46,    36,    61,    58,   123,   125,    33,
     126,    91,    93,    63,    64,    44
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    76,    77,    78,    78,    80,    79,    81,    81,    82,
      82,    82,    83,    83,    84,    84,    85,    85,    85,    86,
      87,    87,    89,    88,    90,    90,    90,    90,    90,    91,
      91,    91,    91,    91,    91,    91,    91,    91,    91,    92,
      93,    93,    93,    94,    94,    95,    95,    95,    96,    97,
      97,    98,    98,    98,    98,    98,    98,    99,    99,    99,
      99,   101,   100,   103,   102,   104,   104,   106,   105,   107,
     107,   108,   108,   108,   109,   111,   110,   112,   112,   114,
     115,   116,   113,   118,   117,   119,   119,   120,   120,   122,
     123,   121,   125,   124,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   127,   127,   127,   127,   127,
     127,   127,   127,   127,   128,   128,   128,   129,   129,   129,
     129,   129,   129,   129,   129,   129,   129,   129,   129,   129,
     129,   130,   131,   132,   132,   133,   133,   134,   134,   135,
     135,   136,   137,   137,   138,   139,   140,   140,   141,   141,
     141,   142,   142,   143,   144,   145,   145,   146,   146,   146,
     146,   146,   146,   147,   147,   147,   147,   148,   148,   148,
     148,   148,   149,   149,   150,   151,   151,   151,   151,   151,
     152,   152,   153,   153,   153,   153,   153,   153,   153,   154,
     155,   156,   156,   157,   157,   158,   159,   159,   160,   160,
     161,   162,   162,   163,   163,   163,   164,   165,   165,   166,
     166,   167,   167,   168,   168,   169,   169,   170,   170,   171,
     171,   172,   172,   172,   172,   172,   172,   173,   173,   174,
     175,   175,   175,   176,   177,   177,   177,   177,   178,   178,
     179,   179,   180,   180,   180,   180,   180,   181,   181,   181,
     181,   181,   182,   181,   181,   181,   181,   181,   181,   181,
     181,   183,   183,   184,   184,   185,   185,   186,   186,   187,
     187,   188,   188,   188,   188,   189,   189,   190,   190,   191,
     191,   192,   192,   193,   193,   194,   194,   195,   195,   196,
     196,   197,   197,   198,   198,   199,   199,   199,   199,   199,
     199,   200,   201,   202,   202,   202,   203,   203,   204,   204,
     204,   204,   204,   204,   204,   204,   204,   204,   204,   205,
     206,   207,   207,   208,   209,   209,   210,   210,   211,   211,
     212,   212,   212,   213,   213,   214,   214,   215,   215,   216,
     216,   217,   217
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     4,     0,     3,     0,     3,     0,     3,     2,
       5,     3,     3,     2,     1,     3,     1,     2,     2,     4,
       0,     1,     0,     4,     0,     1,     1,     1,     1,     2,
       5,     3,     2,     5,     7,     3,     2,     5,     3,     1,
       2,     4,     3,     4,     3,     1,     2,     1,     1,     2,
       3,     1,     3,     3,     3,     2,     2,     3,     5,     5,
       2,     0,     4,     0,     3,     0,     2,     0,     4,     4,
       4,     5,     1,     1,     2,     0,     3,     1,     3,     0,
       0,     0,     8,     0,     5,     0,     2,     0,     2,     0,
       0,     7,     0,     5,     1,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     1,     2,     2,     2,     2,
       2,     2,     2,     2,     3,     5,     6,     1,     1,     3,
       5,     5,     4,     6,     8,     1,     5,     5,     5,     7,
       1,     0,     3,     1,     4,     1,     4,     1,     3,     1,
       1,     1,     1,     1,     1,     1,     0,     1,     1,     1,
       1,     4,     4,     1,     1,     1,     2,     1,     1,     1,
       1,     1,     3,     1,     1,     1,     2,     1,     1,     1,
       1,     3,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     3,     4,     4,     2,     3,     5,     1,     1,     2,
       3,     5,     3,     5,     3,     3,     5,     8,     5,     8,
       5,     0,     3,     0,     1,     3,     1,     4,     2,     0,
       3,     1,     3,     1,     3,     1,     3,     1,     3,     1,
       3,     3,     2,     4,     3,     5,     5,     1,     3,     1,
       2,     1,     3,     4,     1,     2,     2,     1,     1,     3,
       0,     2,     0,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     0,     4,     1,     2,     2,     2,     2,     2,
       2,     1,     3,     1,     3,     1,     3,     1,     3,     1,
       3,     1,     1,     3,     3,     0,     2,     0,     1,     0,
       1,     0,     1,     0,     1,     0,     1,     0,     1,     0,
       1,     0,     1,     0,     1,     4,     4,     5,     6,     4,
       4,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     4,     5,     4,     4,     2,     2,     4,     3,     3,
       5,     3,     4,     3,     5,     1,     0,     1,     3,     1,
       1,     2,     1,     1,     5,     0,     2,     1,     3,     1,
       3,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       5,     0,     3,     0,     1,     0,     7,     0,    22,   158,
     160,     0,     0,   159,   219,    20,     6,   345,     0,     4,
       0,     0,     0,    21,     0,     0,     0,    16,     0,     0,
       9,    22,     0,     8,    28,   127,   156,     0,    39,   156,
       0,   264,    75,     0,     0,     0,    79,     0,     0,   293,
      92,     0,    89,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   291,     0,    25,     0,   257,   258,
     261,   259,   260,    51,    94,   135,   147,   115,   164,   163,
     128,     0,     0,     0,   184,   197,   198,    26,   216,     0,
     140,    27,     0,    19,     0,     0,     0,     0,     0,     0,
     346,   161,   162,    11,    14,   287,    18,    22,    13,    17,
     157,   265,   154,     0,     0,     0,     0,   163,   190,   194,
     180,   178,   179,   177,   266,   135,     0,   295,   250,     0,
     211,   135,   269,   295,   152,   153,     0,     0,   277,   294,
     270,     0,     0,   295,     0,     0,    36,    48,     0,    29,
     275,   155,     0,   123,   118,   119,   122,   116,   117,     0,
       0,   149,     0,   150,   175,   173,   174,   120,   121,     0,
     292,     0,   220,     0,    32,     0,     0,     0,     0,     0,
      56,     0,     0,     0,    55,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   141,
       0,     0,   291,   262,     0,   141,   218,     0,     0,     0,
       0,   311,     0,     0,   211,     0,     0,   312,     0,     0,
      23,   288,     0,    12,   250,     0,     0,   195,   171,   169,
     170,   167,   168,   199,     0,     0,   296,    73,     0,    76,
       0,    72,   165,   244,   163,   247,   151,   248,   289,     0,
     250,     0,   205,    80,    77,   158,     0,   204,     0,   287,
     241,   229,     0,    65,     0,     0,   202,   273,   287,   227,
     239,   303,     0,    90,    38,   225,   287,     0,    49,    31,
     221,   287,     0,     0,    40,     0,   176,   148,     0,     0,
      35,   287,     0,     0,    52,    96,   111,   114,    97,   101,
     102,   100,   112,    99,    98,    95,   113,   103,   104,   105,
     106,   107,   108,   109,   110,   285,   124,   279,   289,     0,
     129,   292,     0,     0,   289,   285,   256,    61,   254,   253,
     271,   255,     0,    54,    53,   278,     0,     0,     0,     0,
     319,     0,     0,     0,     0,     0,   318,     0,   313,   314,
     315,     0,   347,     0,     0,   297,     0,     0,     0,    15,
      10,     0,     0,     0,   181,   191,    67,    74,     0,     0,
     295,   166,   245,   246,   290,   251,   213,     0,     0,     0,
     295,     0,   237,     0,   250,   240,   288,     0,     0,     0,
       0,   303,     0,     0,   288,     0,   304,   232,     0,   303,
       0,   288,     0,    50,   288,     0,    42,   276,     0,     0,
       0,   200,   171,   169,   170,   168,   141,   193,   192,   288,
       0,    44,     0,   141,   143,   281,   282,   289,     0,   289,
     290,     0,     0,     0,   132,   291,   263,   290,     0,     0,
       0,     0,   217,     0,     0,   326,   316,   317,   297,   301,
       0,   299,     0,   325,   340,     0,     0,   342,   343,     0,
       0,     0,     0,     0,   303,     0,     0,   310,     0,   298,
     305,   309,   306,   213,   172,     0,     0,     0,     0,   249,
     250,   163,   214,   189,   187,   188,   185,   186,   210,   213,
     212,    81,    78,   238,   242,     0,   230,   203,   196,     0,
       0,    93,    63,    66,     0,   234,     0,   303,   228,   201,
     274,   231,    65,   226,    37,   222,    30,    41,     0,   285,
      45,   223,   287,    47,    33,    43,   285,     0,   290,   286,
     138,     0,   280,   125,   131,   130,     0,   136,   137,     0,
     272,   328,     0,     0,   319,     0,   318,     0,   335,   351,
     302,     0,     0,     0,   349,   300,   329,   341,     0,   307,
       0,   320,     0,   303,   331,     0,   348,   336,     0,    70,
      69,   295,     0,   250,   206,    85,   213,     0,    60,     0,
     303,   303,   233,     0,   172,     0,   288,     0,    46,     0,
     141,   145,   142,   283,   284,   126,   291,   133,    62,   327,
     336,   297,   324,     0,     0,   303,   323,     0,     0,   321,
     308,   332,   297,   297,   339,   208,   337,    68,    71,   215,
       0,    87,   243,     0,     0,    57,     0,    64,   236,   235,
      91,   139,   224,    34,   144,   285,     0,   330,     0,   352,
     322,   333,   350,     0,     0,     0,   213,     0,    86,    82,
       0,     0,     0,   134,   336,   344,   336,   338,   207,    83,
      88,    59,    58,   146,   334,   209,   295,     0,    84
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     6,     2,     3,    14,    21,    30,   105,    31,
       8,    24,    16,    17,    65,   328,    67,   149,   520,   521,
     145,   146,    68,   502,   329,   440,   503,   579,   389,   367,
     475,   237,   238,   239,    69,   127,   253,    70,   133,   379,
     575,   648,   666,   621,   649,    71,   143,   400,    72,   141,
      73,    74,    75,    76,   315,   425,   426,   592,    77,   317,
     243,   136,    78,   150,   111,   117,    13,    80,    81,   245,
     246,   163,   119,    82,    83,   482,   228,    84,   230,   231,
      85,    86,    87,   130,   214,    88,   252,   488,    89,    90,
      22,   281,   522,   276,   268,   259,   269,   270,   271,   261,
     385,   247,   248,   249,   330,   331,   323,   332,   272,   152,
      92,   318,   427,   428,   222,   375,   171,   140,   254,   468,
     553,   547,   397,   100,   212,   218,   614,   445,   348,   349,
     350,   352,   554,   549,   615,   616,   458,   459,    25,   469,
     555,   550
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -521
static const yytype_int16 yypact[] =
{
    -521,    61,    48,    62,  -521,   328,  -521,    71,  -521,  -521,
    -521,    82,    44,  -521,    99,   104,  -521,  -521,    76,  -521,
      59,    80,  1130,  -521,    85,   439,    26,  -521,   121,   146,
    -521,    62,   156,  -521,  -521,  -521,   328,   615,  -521,   328,
     294,  -521,  -521,   322,   294,   328,  -521,    25,    95,  1665,
    -521,    25,  -521,   335,   346,  1665,  1665,  1665,  1665,  1665,
    1665,  1708,  1665,  1665,   623,   142,  -521,   392,  -521,  -521,
    -521,  -521,  -521,   931,  -521,  -521,   166,   153,  -521,   143,
    -521,   171,   196,    25,   208,  -521,  -521,  -521,   216,    54,
    -521,  -521,    39,  -521,   204,     7,   221,   204,   204,   229,
    -521,  -521,  -521,  -521,  -521,   232,  -521,  -521,  -521,  -521,
    -521,  -521,  -521,   237,  1828,  1828,  1828,  -521,   235,  -521,
    -521,  -521,  -521,  -521,  -521,   185,   153,  1665,  1820,   242,
     243,   320,  -521,  1665,  -521,  -521,   212,  1828,  2178,   238,
    -521,   280,   434,  1665,   418,  1045,  -521,  -521,   450,  -521,
    -521,  -521,   791,  -521,  -521,  -521,  -521,  -521,  -521,  1751,
    1708,  2178,   262,  -521,   134,  -521,   139,  -521,  -521,   252,
    2178,   257,  -521,   459,  -521,   986,  1665,  1665,  1665,  1665,
    -521,  1665,  1665,  1665,  -521,  1665,  1665,  1665,  1665,  1665,
    1665,  1665,  1665,  1665,  1665,  1665,  1665,  1665,  1665,  -521,
    1347,   476,  1665,  -521,  1665,  -521,  -521,  1292,  1665,  1665,
    1665,  -521,   687,   328,   243,   272,   330,  -521,  2004,  2004,
    -521,    81,   295,  -521,  1820,   348,  1828,  -521,  -521,  -521,
    -521,  -521,  -521,  -521,   300,   328,  -521,  -521,   329,  -521,
      52,   307,  1828,  -521,  1820,  -521,  -521,  -521,   301,   323,
    1820,  1292,  -521,  -521,   336,   286,   360,  -521,   326,   338,
    -521,  -521,   318,  -521,    34,    35,  -521,  -521,   344,  -521,
    -521,   394,  1794,  -521,  -521,  -521,   349,  1828,  -521,  -521,
    -521,   350,  1665,   328,   361,  1853,  -521,   347,  1828,  1828,
    -521,   367,  1665,   379,  2178,  2249,  -521,  2202,   681,   681,
     681,   681,  -521,   681,   681,  2226,  -521,   503,   503,   503,
     503,  -521,  -521,  -521,  -521,  1402,  -521,  -521,    32,  1457,
    -521,  2076,   366,  1218,  2043,  1402,  -521,  -521,  -521,  -521,
    -521,  -521,   111,   238,   238,  2178,  1920,   387,   383,   384,
    -521,   400,   461,  2004,   158,    37,  -521,   408,  -521,  -521,
    -521,   876,  -521,    22,   423,   328,   428,   432,   436,  -521,
    -521,   424,  1828,   446,  -521,  -521,  -521,  -521,  1512,  1567,
    1665,  -521,  -521,  -521,  1820,  -521,  1887,   452,   192,   329,
    1665,   328,   433,   468,  1820,  -521,   487,   453,  1828,    89,
     360,   394,   360,   469,   354,   466,  -521,  -521,   328,   394,
     508,   328,   485,  -521,   328,   492,   238,  -521,  1665,  1912,
    1828,  -521,   175,   351,   405,   422,  -521,  -521,  -521,   328,
     494,   238,  1665,  -521,  2106,  -521,  -521,   473,   495,   480,
    1708,   502,   504,   506,  -521,  1665,  -521,  -521,   509,   500,
    1292,  1218,  -521,  2004,   535,  -521,  -521,  -521,   328,  1945,
    2004,   328,  2004,  -521,  -521,   569,   288,  -521,  -521,   511,
     505,  2004,   158,  2004,   394,   328,   328,  -521,   514,   512,
    -521,  -521,  -521,  1887,  -521,  1292,  1665,  1665,   516,  -521,
    1820,   521,  -521,  -521,  -521,  -521,  -521,  -521,  -521,  1887,
    -521,  -521,  -521,  -521,  -521,   524,  -521,  -521,  -521,  1708,
     515,  -521,  -521,  -521,   530,  -521,   532,   394,  -521,  -521,
    -521,  -521,  -521,  -521,  -521,  -521,  -521,   238,   533,  1402,
    -521,  -521,   536,   986,  -521,   238,  1402,  1610,  1402,  -521,
    -521,   534,  -521,  -521,  -521,  -521,   200,  -521,  -521,   246,
    -521,  -521,   537,   539,   543,   544,   545,   527,  -521,  -521,
     546,   540,  2004,   538,  -521,   552,  -521,  -521,   558,  -521,
    2004,  -521,   553,   394,  -521,   557,  -521,  1979,   282,  2178,
    2178,  1665,   560,  1820,  -521,  -521,  1887,    60,  -521,  1218,
     394,   394,  -521,   215,   442,   549,   328,   562,   379,   550,
    -521,  2178,  -521,  -521,  -521,  -521,  1665,  -521,  -521,  -521,
    1979,   328,  -521,  1945,  2004,   394,  -521,   328,   288,  -521,
    -521,  -521,   328,   328,  -521,  -521,  -521,  -521,  -521,  -521,
     564,   613,  -521,  1665,  1665,  -521,  1708,   566,  -521,  -521,
    -521,  -521,  -521,  -521,  -521,  1402,   561,  -521,   570,  -521,
    -521,  -521,  -521,   575,   576,   582,  1887,    31,  -521,  -521,
    2130,  2154,   580,  -521,  1979,  -521,  1979,  -521,  -521,  -521,
    -521,  -521,  -521,  -521,  -521,  -521,  1665,   329,  -521
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -521,  -521,  -521,  -521,  -521,  -521,  -521,   -10,  -521,  -521,
     598,  -521,     4,  -521,  -521,   627,  -521,  -120,   -25,    67,
    -521,  -127,  -124,  -521,     9,  -521,  -521,  -521,   145,  -358,
    -521,  -521,  -521,  -521,  -521,  -521,  -139,  -521,  -521,  -521,
    -521,  -521,  -521,  -521,  -521,  -521,  -521,  -521,  -521,  -521,
     594,    12,    50,  -521,  -198,   126,   132,  -521,   186,   -56,
     419,   140,   -23,   382,   628,   477,   488,   191,  -521,   425,
     -89,   510,  -521,  -521,  -521,  -521,   -36,   -37,   -35,   -50,
    -521,  -521,  -521,  -521,  -521,    15,   448,  -460,  -521,  -521,
    -521,  -521,  -521,  -521,  -521,  -521,   283,  -105,  -228,   297,
    -521,   292,  -521,  -216,  -300,   659,  -521,  -237,  -521,   -61,
      53,   188,  -521,  -305,  -227,  -271,  -192,  -521,  -112,  -436,
    -521,  -521,  -361,  -521,   420,  -521,  -173,  -521,   355,   241,
     363,   236,    96,   102,  -520,  -521,  -423,   251,  -521,   507,
    -521,  -521
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -278
static const yytype_int16 yytable[] =
{
     121,   120,   122,   236,   273,   162,   175,   325,   361,   236,
     322,   165,   543,   110,   378,   241,   110,   275,   104,   236,
     439,   491,   132,   436,   164,   227,   233,   234,   280,   574,
     505,   260,   387,   558,   377,   108,   391,   393,   511,   347,
     460,   395,   174,   429,   208,   357,   358,   431,   262,   402,
     101,   659,   123,   438,   405,   206,   278,   368,   382,   382,
     134,     4,    27,   284,   420,   623,   213,   153,   154,   155,
     156,   157,   158,     5,   167,   168,   166,   229,   229,   229,
     637,    11,   465,     9,    27,    18,   293,     7,   392,   134,
     125,   229,   135,   390,   131,    15,   499,   466,   327,   102,
     229,   500,   139,   564,   209,     9,    19,   430,   229,   461,
     165,   223,   175,   258,   210,   229,   622,   369,    28,   267,
      20,   135,    29,   164,    27,   624,   625,   210,    23,   232,
     232,   232,    10,    11,   664,   626,   665,   363,   229,    26,
    -216,   540,    33,   232,    29,     9,   582,    93,   291,   106,
     165,   383,   232,   371,    10,    11,   529,   501,   531,   109,
     232,   454,   504,   164,   506,   638,   137,   232,   495,  -184,
     453,   153,   157,   441,  -216,   166,   644,   645,   464,   442,
     240,   103,     9,   399,    29,   643,   658,   229,   403,   229,
     232,   142,  -268,  -183,    10,    11,   411,  -268,  -182,   417,
     418,  -184,   611,   539,   172,   229,  -216,   229,   455,  -154,
    -180,   359,   200,   229,   585,   166,   201,   456,   519,   628,
     629,   589,   499,   205,   202,   526,   126,   500,   118,   207,
     126,    10,    11,   199,  -180,   229,   255,   203,   568,   232,
     229,   232,  -180,   536,   641,   216,   236,  -268,   413,   412,
     414,   229,   229,  -268,   441,   204,   236,   232,   478,   232,
     490,   333,   334,   433,   572,   232,   596,  -183,   492,   165,
     541,   256,   597,   411,   513,  -182,   548,   551,    11,   556,
     257,   260,   164,   630,   515,    10,    11,   232,   561,  -237,
     563,   454,   232,   220,   221,   587,   224,    35,   235,   498,
     415,   250,    37,   232,   232,   118,   118,   118,   441,   668,
     251,   113,     9,   210,   598,   263,    47,    48,     9,   118,
     227,   518,   287,    51,   288,   229,   486,  -267,   118,   289,
     652,   355,  -267,   356,   166,   406,   118,   229,   455,   484,
     483,   485,   627,   118,   441,   421,     9,   229,  -237,   381,
     617,   229,     9,    61,  -237,   360,   362,   620,   523,     9,
     364,    10,    11,   258,   366,    64,   118,    10,    11,   370,
       9,   267,   229,   229,   532,   510,   374,   232,   255,   605,
     165,   128,  -267,   376,   382,   384,  -178,   609,  -267,   232,
     388,   487,   635,   164,   144,    10,    11,   396,   380,   232,
     386,    10,    11,   232,   636,   148,   394,   264,    10,    11,
    -178,   401,   404,   265,   416,   118,     9,   118,  -178,    10,
      11,   333,   334,   486,   232,   232,   408,    10,    11,   419,
     548,   640,   435,   118,   588,   118,   484,   483,   485,   486,
    -179,   118,     9,   229,   422,   166,   448,   236,    94,   165,
     449,   173,   484,   483,   485,   450,    95,  -177,   255,   618,
      96,   517,   164,   118,  -179,    10,    11,   451,   118,   452,
      97,    98,  -179,   462,     9,   525,   118,  -181,   274,   118,
     118,  -177,    12,     9,   473,   467,   229,   264,   487,  -177,
     470,    10,    11,   265,   471,   232,   381,    32,   472,    79,
       9,  -181,   266,    99,   487,    32,   474,    10,    11,  -181,
     279,   255,   489,   112,   166,   215,   112,   217,   219,   290,
     129,   497,   112,    10,    11,   523,   486,   667,   494,   507,
     147,   151,    10,    11,   509,   319,   229,   178,   232,   484,
     483,   485,   236,   512,   151,   514,   256,   186,   528,    10,
      11,   190,   516,   118,   524,   437,   195,   196,   197,   198,
      10,    11,   533,   530,   534,   118,   535,   118,   538,   537,
     532,   342,   557,   559,   567,   118,   165,   560,   571,   118,
     573,   578,   211,   211,   576,   211,   211,   466,   232,   164,
     580,   487,   581,   584,   595,   602,   486,   599,   586,   600,
     118,   118,  -158,   601,  -159,   244,   606,   608,   603,   484,
     483,   485,   604,   112,   607,   610,   612,   631,   634,   112,
     619,   147,   633,    37,   646,   151,    35,   647,   441,   107,
     654,    37,   113,   653,   169,   655,   656,    47,    48,     9,
     113,   166,   657,   138,    51,    47,    48,     9,   663,    66,
     151,   114,    51,   632,   593,   161,   660,   583,   170,    55,
     594,   487,   354,   372,   118,   407,   479,   124,   115,   373,
     286,   118,    56,    57,   116,    58,    59,   508,   320,    60,
     118,    91,    61,   496,    79,   542,    64,   577,    10,    11,
     351,   446,    62,    63,    64,   336,    10,    11,    32,   447,
     346,   244,   566,   642,   337,   639,   346,   346,     0,   338,
     339,   340,   365,   562,   118,   178,   341,     0,     0,     0,
     353,     0,     0,   342,     0,   186,     0,   244,    79,   190,
     191,   192,   193,   194,   195,   196,   197,   198,     0,     0,
     343,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   344,     0,     0,     0,     0,     0,   345,     0,
     151,    11,     0,     0,   118,     0,     0,   118,     0,     0,
     294,   295,   296,   297,     0,   298,   299,   300,     0,   301,
     302,   303,   304,   305,   306,   307,   308,   309,   310,   311,
     312,   313,   314,     0,   161,     0,   321,     0,   324,    37,
      79,     0,   138,   138,   335,     0,     0,     0,   113,     0,
       0,     0,     0,    47,    48,     9,     0,     0,     0,     0,
      51,   457,     0,     0,   346,     0,     0,   225,     0,     0,
       0,   346,   351,     0,     0,     0,     0,   118,     0,   346,
       0,     0,     0,     0,   115,     0,     0,     0,     0,     0,
     226,   244,     0,   481,     0,     0,   282,     0,   493,     0,
       0,   244,    64,   112,    10,    11,   283,     0,     0,     0,
       0,   112,     0,     0,     0,   112,   138,     0,   147,     0,
       0,   151,     0,     0,   336,     0,   138,   463,     0,     0,
       0,     0,     0,   337,     0,     0,   151,     0,   338,   339,
     340,     0,     0,     0,     0,   341,     0,     0,     0,   424,
       0,     0,   342,   161,     0,     0,     0,    79,    79,   424,
       0,     0,     0,     0,     0,   351,   545,     0,   552,   343,
       0,   346,     0,   457,     0,   176,  -277,   546,   346,   457,
     346,     0,   565,   351,     0,     0,     0,   345,     0,   346,
      11,   346,    79,     0,     0,     0,     0,   244,     0,     0,
       0,     0,   138,   138,   177,   178,     0,   179,   180,   181,
     182,   183,     0,   184,   185,   186,   187,   188,   189,   190,
     191,   192,   193,   194,   195,   196,   197,   198,     0,     0,
       0,     0,     0,     0,    37,     0,  -277,     0,     0,     0,
       0,     0,   138,   113,     0,     0,  -277,     0,    47,    48,
       9,     0,     0,     0,     0,    51,   138,     0,     0,     0,
       0,     0,   225,     0,   161,     0,     0,     0,     0,   170,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   115,
     346,     0,     0,     0,     0,   226,     0,     0,   346,     0,
     244,   292,     0,    37,     0,   346,    79,    64,     0,    10,
      11,   283,   113,   151,     0,     0,     0,    47,    48,     9,
     569,   570,     0,     0,    51,     0,     0,     0,   351,     0,
     545,   225,     0,     0,   552,   457,     0,     0,   346,   351,
     351,   546,   346,   161,     0,     0,     0,     0,   115,     0,
       0,     0,     0,     0,   226,     0,     0,     0,     0,     0,
     277,     0,     0,   424,     0,     0,    64,     0,    10,    11,
     424,   591,   424,     0,     0,     0,     0,     0,     0,     0,
      -2,    34,     0,    35,     0,     0,    36,     0,    37,    38,
      39,     0,   346,    40,   346,    41,    42,    43,    44,    45,
      46,     0,    47,    48,     9,     0,     0,    49,    50,    51,
      52,    53,    54,     0,     0,     0,    55,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    56,
      57,     0,    58,    59,     0,     0,    60,     0,     0,    61,
     170,     0,   -24,     0,     0,     0,     0,     0,     0,    62,
      63,    64,     0,    10,    11,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   650,   651,   326,
     161,    35,     0,     0,    36,  -252,    37,    38,    39,   424,
    -252,    40,     0,    41,    42,   113,    44,    45,    46,     0,
      47,    48,     9,     0,     0,    49,    50,    51,    52,    53,
      54,     0,     0,     0,    55,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    56,    57,     0,
      58,    59,     0,     0,    60,     0,     0,    61,     0,     0,
    -252,     0,     0,     0,     0,   327,  -252,    62,    63,    64,
       0,    10,    11,   326,     0,    35,     0,     0,    36,     0,
      37,    38,    39,     0,     0,    40,     0,    41,    42,   113,
      44,    45,    46,     0,    47,    48,     9,     0,     0,    49,
      50,    51,    52,    53,    54,     0,     0,     0,    55,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    56,    57,     0,    58,    59,     0,     0,    60,     0,
      35,    61,     0,     0,  -252,    37,     0,     0,     0,   327,
    -252,    62,    63,    64,   113,    10,    11,     0,     0,    47,
      48,     9,     0,     0,     0,     0,    51,     0,     0,     0,
       0,     0,     0,   159,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    56,    57,     0,    58,
     160,     0,     0,    60,     0,    35,    61,   316,     0,     0,
      37,     0,     0,     0,     0,     0,    62,    63,    64,   113,
      10,    11,     0,     0,    47,    48,     9,     0,     0,     0,
       0,    51,     0,     0,     0,     0,     0,     0,    55,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    56,    57,     0,    58,    59,     0,     0,    60,     0,
      35,    61,     0,     0,     0,    37,     0,     0,     0,   423,
       0,    62,    63,    64,   113,    10,    11,     0,     0,    47,
      48,     9,     0,     0,     0,     0,    51,     0,   432,     0,
       0,     0,     0,   159,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    56,    57,     0,    58,
     160,     0,     0,    60,     0,    35,    61,     0,     0,     0,
      37,     0,     0,     0,     0,     0,    62,    63,    64,   113,
      10,    11,     0,     0,    47,    48,     9,     0,   476,     0,
       0,    51,     0,     0,     0,     0,     0,     0,    55,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    56,    57,     0,    58,    59,     0,     0,    60,     0,
      35,    61,     0,     0,     0,    37,     0,     0,     0,     0,
       0,    62,    63,    64,   113,    10,    11,     0,     0,    47,
      48,     9,     0,   477,     0,     0,    51,     0,     0,     0,
       0,     0,     0,    55,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    35,     0,     0,    56,    57,    37,    58,
      59,     0,     0,    60,     0,     0,    61,   113,     0,     0,
       0,     0,    47,    48,     9,     0,    62,    63,    64,    51,
      10,    11,     0,     0,     0,     0,    55,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    56,
      57,     0,    58,    59,     0,     0,    60,     0,    35,    61,
       0,     0,     0,    37,     0,     0,     0,   590,     0,    62,
      63,    64,   113,    10,    11,     0,     0,    47,    48,     9,
       0,     0,     0,     0,    51,     0,     0,     0,     0,     0,
       0,    55,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    35,     0,     0,    56,    57,    37,    58,    59,     0,
       0,    60,     0,     0,    61,   113,     0,     0,     0,     0,
      47,    48,     9,     0,    62,    63,    64,    51,    10,    11,
       0,     0,     0,     0,   159,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    35,     0,     0,    56,    57,   285,
      58,   160,     0,     0,    60,     0,     0,    61,   113,     0,
       0,     0,     0,    47,    48,     9,     0,    62,    63,    64,
      51,    10,    11,     0,     0,     0,     0,    55,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      56,    57,    37,    58,    59,     0,     0,    60,     0,     0,
      61,   113,     0,     0,     0,     0,    47,    48,     9,     0,
      62,    63,    64,    51,    10,    11,     0,     0,    37,     0,
     225,   242,     0,     0,     0,     0,    37,   113,     0,     0,
       0,     0,    47,    48,     9,   113,     0,   115,     0,    51,
      47,    48,     9,   226,     0,     0,   225,    51,     0,     0,
       0,    37,     0,     0,   225,    64,     0,    10,    11,   398,
     113,     0,     0,   115,     0,    47,    48,     9,     0,   226,
       0,   115,    51,     0,     0,     0,     0,   226,     0,   409,
       0,    64,     0,    10,    11,    37,     0,     0,     0,    64,
       0,    10,    11,     0,   113,     0,   115,     0,     0,    47,
      48,     9,   410,     0,     0,     0,    51,     0,     0,     0,
     285,     0,     0,   225,    64,     0,    10,    11,   336,   113,
       0,     0,     0,     0,    47,    48,     9,   337,     0,     0,
     115,    51,   338,   339,   340,     0,   480,     0,   225,   341,
       0,     0,     0,   336,     0,     0,   443,     0,    64,     0,
      10,    11,   337,     0,     0,   115,     0,   338,   339,   544,
       0,   226,     0,   343,   341,     0,     0,     0,     0,   444,
       0,   342,     0,    64,     0,    10,    11,   336,     0,     0,
       0,   345,     0,     0,    11,     0,   337,     0,   343,     0,
       0,   338,   339,   340,     0,     0,     0,     0,   341,     0,
       0,     0,   336,     0,     0,   342,   345,     0,    10,    11,
       0,   337,     0,     0,     0,     0,   338,   339,   340,     0,
       0,     0,   343,   341,     0,     0,     0,     0,   613,     0,
     342,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     345,     0,     0,    11,     0,     0,     0,   343,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   345,   177,   178,    11,   179,
       0,   181,   182,   183,     0,     0,   185,   186,   187,   188,
     189,   190,   191,   192,   193,   194,   195,   196,   197,   198,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   177,
     178,     0,   179,     0,   181,   182,   183,     0,   437,   185,
     186,   187,   188,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,     0,     0,     0,     0,     0,     0,   177,
     178,     0,   179,     0,   181,   182,   183,     0,   434,   185,
     186,   187,   188,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   177,   178,     0,   179,     0,   181,   182,
     183,     0,   527,   185,   186,   187,   188,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   177,   178,     0,
     179,     0,   181,   182,   183,     0,   661,   185,   186,   187,
     188,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   177,   178,     0,   179,     0,   181,   182,   183,     0,
     662,   185,   186,   187,   188,   189,   190,   191,   192,   193,
     194,   195,   196,   197,   198,   177,   178,     0,     0,     0,
     181,   182,   183,     0,     0,   185,   186,   187,   188,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   177,
     178,     0,     0,     0,   181,   182,   183,     0,     0,   185,
     186,   187,   188,     0,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   178,     0,     0,     0,   181,   182,   183,
       0,     0,   185,   186,   187,   188,     0,   190,   191,   192,
     193,   194,   195,   196,   197,   198
};

#define yypact_value_is_default(yystate) \
  ((yystate) == (-521))

#define yytable_value_is_error(yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
      37,    37,    37,   127,   143,    61,    67,   205,   224,   133,
     202,    61,   448,    36,   251,   127,    39,   144,    28,   143,
     325,   379,    45,   323,    61,   114,   115,   116,   148,   489,
     391,   136,   259,   456,   250,    31,   264,   265,   399,   212,
       3,   268,    67,    11,     5,   218,   219,   318,   137,   276,
      24,    20,    37,   324,   281,     1,   145,     5,    24,    24,
      35,     0,     3,   152,   291,     5,    59,    55,    56,    57,
      58,    59,    60,    25,    62,    63,    61,   114,   115,   116,
     600,    74,    60,    24,     3,     3,   175,    25,    53,    35,
      40,   128,    67,    59,    44,    24,     7,    75,    67,    73,
     137,    12,    49,   464,    65,    24,    62,    75,   145,    72,
     160,   107,   173,   136,    75,   152,   576,    65,    59,   142,
      21,    67,    63,   160,     3,    65,    66,    75,    24,   114,
     115,   116,    73,    74,   654,    75,   656,   226,   175,    63,
       1,   441,    62,   128,    63,    24,   507,    62,   173,     3,
     200,   256,   137,   242,    73,    74,   427,    68,   429,     3,
     145,     3,   390,   200,   392,   601,    71,   152,   384,    35,
     343,   159,   160,    62,    35,   160,   612,   613,   351,    68,
     127,    60,    24,   272,    63,   608,   646,   224,   277,   226,
     175,    51,     7,    59,    73,    74,   285,    12,    59,   288,
     289,    67,   563,   440,    62,   242,    67,   244,    50,    66,
      35,   221,    59,   250,   519,   200,    63,    59,   416,   580,
     581,   526,     7,    83,    71,   423,    40,    12,    37,    89,
      44,    73,    74,    67,    59,   272,    24,    66,   475,   224,
     277,   226,    67,   435,   605,    24,   370,    62,   285,   285,
     285,   288,   289,    68,    62,    59,   380,   242,   370,   244,
      68,   208,   209,   319,   480,   250,    66,    59,   380,   319,
     443,    59,    72,   362,   401,    59,   449,   450,    74,   452,
      68,   386,   319,    68,   404,    73,    74,   272,   461,     3,
     463,     3,   277,    64,    62,   522,    59,     3,    63,   388,
     285,    59,     8,   288,   289,   114,   115,   116,    62,   667,
      67,    17,    24,    75,    68,    35,    22,    23,    24,   128,
     409,   410,    60,    29,    72,   362,   376,     7,   137,    72,
     635,    59,    12,     3,   319,   282,   145,   374,    50,   376,
     376,   376,   579,   152,    62,   292,    24,   384,    62,    63,
      68,   388,    24,    59,    68,    60,     8,   573,   419,    24,
      60,    73,    74,   386,    35,    71,   175,    73,    74,    62,
      24,   394,   409,   410,   430,   398,    75,   362,    24,   552,
     430,    59,    62,    60,    24,    59,    35,   560,    68,   374,
      72,   376,   590,   430,    59,    73,    74,     3,    62,   384,
      62,    73,    74,   388,   596,    59,    62,    53,    73,    74,
      59,    62,    62,    59,    67,   224,    24,   226,    67,    73,
      74,   368,   369,   473,   409,   410,    65,    73,    74,    62,
     603,   604,    66,   242,   523,   244,   473,   473,   473,   489,
      35,   250,    24,   480,    65,   430,    59,   571,     9,   499,
      67,    59,   489,   489,   489,    71,    17,    35,    24,   571,
      21,   408,   499,   272,    59,    73,    74,    67,   277,     8,
      31,    32,    67,    65,    24,   422,   285,    35,    60,   288,
     289,    59,     5,    24,    60,    62,   523,    53,   473,    67,
      62,    73,    74,    59,    62,   480,    63,    20,    62,    22,
      24,    59,    68,    64,   489,    28,    60,    73,    74,    67,
      60,    24,    60,    36,   499,    95,    39,    97,    98,    60,
      43,    68,    45,    73,    74,   586,   576,   666,    60,    60,
      53,    54,    73,    74,    68,    59,   573,    34,   523,   576,
     576,   576,   666,    35,    67,    60,    59,    44,    75,    73,
      74,    48,    60,   362,    60,    75,    53,    54,    55,    56,
      73,    74,    60,    68,    60,   374,    60,   376,    68,    60,
     626,    36,     3,    62,    60,   384,   626,    72,    62,   388,
      59,    66,    94,    95,    60,    97,    98,    75,   573,   626,
      60,   576,    60,    60,    60,    68,   646,    60,    62,    60,
     409,   410,    59,    59,    59,   128,    68,    49,    62,   646,
     646,   646,    72,   136,    62,    62,    59,    68,    68,   142,
      60,   144,    60,     8,    60,   148,     3,    14,    62,    31,
      60,     8,    17,    72,    11,    60,    60,    22,    23,    24,
      17,   626,    60,    49,    29,    22,    23,    24,    68,    22,
     173,    36,    29,   586,   528,    61,   647,   512,    64,    36,
     528,   646,   214,   244,   473,   283,   374,    39,    53,   244,
     160,   480,    49,    50,    59,    52,    53,   394,   201,    56,
     489,    22,    59,   386,   207,   444,    71,   499,    73,    74,
     213,   336,    69,    70,    71,     8,    73,    74,   221,   336,
     212,   224,   466,   607,    17,   603,   218,   219,    -1,    22,
      23,    24,   235,   462,   523,    34,    29,    -1,    -1,    -1,
     213,    -1,    -1,    36,    -1,    44,    -1,   250,   251,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    -1,    -1,
      53,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    65,    -1,    -1,    -1,    -1,    -1,    71,    -1,
     283,    74,    -1,    -1,   573,    -1,    -1,   576,    -1,    -1,
     176,   177,   178,   179,    -1,   181,   182,   183,    -1,   185,
     186,   187,   188,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,    -1,   200,    -1,   202,    -1,   204,     8,
     323,    -1,   208,   209,   210,    -1,    -1,    -1,    17,    -1,
      -1,    -1,    -1,    22,    23,    24,    -1,    -1,    -1,    -1,
      29,   344,    -1,    -1,   336,    -1,    -1,    36,    -1,    -1,
      -1,   343,   355,    -1,    -1,    -1,    -1,   646,    -1,   351,
      -1,    -1,    -1,    -1,    53,    -1,    -1,    -1,    -1,    -1,
      59,   374,    -1,   376,    -1,    -1,    65,    -1,   381,    -1,
      -1,   384,    71,   386,    73,    74,    75,    -1,    -1,    -1,
      -1,   394,    -1,    -1,    -1,   398,   282,    -1,   401,    -1,
      -1,   404,    -1,    -1,     8,    -1,   292,    11,    -1,    -1,
      -1,    -1,    -1,    17,    -1,    -1,   419,    -1,    22,    23,
      24,    -1,    -1,    -1,    -1,    29,    -1,    -1,    -1,   315,
      -1,    -1,    36,   319,    -1,    -1,    -1,   440,   441,   325,
      -1,    -1,    -1,    -1,    -1,   448,   449,    -1,   451,    53,
      -1,   443,    -1,   456,    -1,     4,     5,   449,   450,   462,
     452,    -1,   465,   466,    -1,    -1,    -1,    71,    -1,   461,
      74,   463,   475,    -1,    -1,    -1,    -1,   480,    -1,    -1,
      -1,    -1,   368,   369,    33,    34,    -1,    36,    37,    38,
      39,    40,    -1,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    -1,    -1,
      -1,    -1,    -1,    -1,     8,    -1,    65,    -1,    -1,    -1,
      -1,    -1,   408,    17,    -1,    -1,    75,    -1,    22,    23,
      24,    -1,    -1,    -1,    -1,    29,   422,    -1,    -1,    -1,
      -1,    -1,    36,    -1,   430,    -1,    -1,    -1,    -1,   435,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    53,
     552,    -1,    -1,    -1,    -1,    59,    -1,    -1,   560,    -1,
     573,    65,    -1,     8,    -1,   567,   579,    71,    -1,    73,
      74,    75,    17,   586,    -1,    -1,    -1,    22,    23,    24,
     476,   477,    -1,    -1,    29,    -1,    -1,    -1,   601,    -1,
     603,    36,    -1,    -1,   607,   608,    -1,    -1,   600,   612,
     613,   603,   604,   499,    -1,    -1,    -1,    -1,    53,    -1,
      -1,    -1,    -1,    -1,    59,    -1,    -1,    -1,    -1,    -1,
      65,    -1,    -1,   519,    -1,    -1,    71,    -1,    73,    74,
     526,   527,   528,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       0,     1,    -1,     3,    -1,    -1,     6,    -1,     8,     9,
      10,    -1,   654,    13,   656,    15,    16,    17,    18,    19,
      20,    -1,    22,    23,    24,    -1,    -1,    27,    28,    29,
      30,    31,    32,    -1,    -1,    -1,    36,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,
      50,    -1,    52,    53,    -1,    -1,    56,    -1,    -1,    59,
     596,    -1,    62,    -1,    -1,    -1,    -1,    -1,    -1,    69,
      70,    71,    -1,    73,    74,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   623,   624,     1,
     626,     3,    -1,    -1,     6,     7,     8,     9,    10,   635,
      12,    13,    -1,    15,    16,    17,    18,    19,    20,    -1,
      22,    23,    24,    -1,    -1,    27,    28,    29,    30,    31,
      32,    -1,    -1,    -1,    36,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,    50,    -1,
      52,    53,    -1,    -1,    56,    -1,    -1,    59,    -1,    -1,
      62,    -1,    -1,    -1,    -1,    67,    68,    69,    70,    71,
      -1,    73,    74,     1,    -1,     3,    -1,    -1,     6,    -1,
       8,     9,    10,    -1,    -1,    13,    -1,    15,    16,    17,
      18,    19,    20,    -1,    22,    23,    24,    -1,    -1,    27,
      28,    29,    30,    31,    32,    -1,    -1,    -1,    36,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    49,    50,    -1,    52,    53,    -1,    -1,    56,    -1,
       3,    59,    -1,    -1,    62,     8,    -1,    -1,    -1,    67,
      68,    69,    70,    71,    17,    73,    74,    -1,    -1,    22,
      23,    24,    -1,    -1,    -1,    -1,    29,    -1,    -1,    -1,
      -1,    -1,    -1,    36,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    49,    50,    -1,    52,
      53,    -1,    -1,    56,    -1,     3,    59,    60,    -1,    -1,
       8,    -1,    -1,    -1,    -1,    -1,    69,    70,    71,    17,
      73,    74,    -1,    -1,    22,    23,    24,    -1,    -1,    -1,
      -1,    29,    -1,    -1,    -1,    -1,    -1,    -1,    36,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    49,    50,    -1,    52,    53,    -1,    -1,    56,    -1,
       3,    59,    -1,    -1,    -1,     8,    -1,    -1,    -1,    67,
      -1,    69,    70,    71,    17,    73,    74,    -1,    -1,    22,
      23,    24,    -1,    -1,    -1,    -1,    29,    -1,    31,    -1,
      -1,    -1,    -1,    36,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    49,    50,    -1,    52,
      53,    -1,    -1,    56,    -1,     3,    59,    -1,    -1,    -1,
       8,    -1,    -1,    -1,    -1,    -1,    69,    70,    71,    17,
      73,    74,    -1,    -1,    22,    23,    24,    -1,    26,    -1,
      -1,    29,    -1,    -1,    -1,    -1,    -1,    -1,    36,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    49,    50,    -1,    52,    53,    -1,    -1,    56,    -1,
       3,    59,    -1,    -1,    -1,     8,    -1,    -1,    -1,    -1,
      -1,    69,    70,    71,    17,    73,    74,    -1,    -1,    22,
      23,    24,    -1,    26,    -1,    -1,    29,    -1,    -1,    -1,
      -1,    -1,    -1,    36,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     3,    -1,    -1,    49,    50,     8,    52,
      53,    -1,    -1,    56,    -1,    -1,    59,    17,    -1,    -1,
      -1,    -1,    22,    23,    24,    -1,    69,    70,    71,    29,
      73,    74,    -1,    -1,    -1,    -1,    36,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,
      50,    -1,    52,    53,    -1,    -1,    56,    -1,     3,    59,
      -1,    -1,    -1,     8,    -1,    -1,    -1,    67,    -1,    69,
      70,    71,    17,    73,    74,    -1,    -1,    22,    23,    24,
      -1,    -1,    -1,    -1,    29,    -1,    -1,    -1,    -1,    -1,
      -1,    36,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     3,    -1,    -1,    49,    50,     8,    52,    53,    -1,
      -1,    56,    -1,    -1,    59,    17,    -1,    -1,    -1,    -1,
      22,    23,    24,    -1,    69,    70,    71,    29,    73,    74,
      -1,    -1,    -1,    -1,    36,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     3,    -1,    -1,    49,    50,     8,
      52,    53,    -1,    -1,    56,    -1,    -1,    59,    17,    -1,
      -1,    -1,    -1,    22,    23,    24,    -1,    69,    70,    71,
      29,    73,    74,    -1,    -1,    -1,    -1,    36,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      49,    50,     8,    52,    53,    -1,    -1,    56,    -1,    -1,
      59,    17,    -1,    -1,    -1,    -1,    22,    23,    24,    -1,
      69,    70,    71,    29,    73,    74,    -1,    -1,     8,    -1,
      36,    11,    -1,    -1,    -1,    -1,     8,    17,    -1,    -1,
      -1,    -1,    22,    23,    24,    17,    -1,    53,    -1,    29,
      22,    23,    24,    59,    -1,    -1,    36,    29,    -1,    -1,
      -1,     8,    -1,    -1,    36,    71,    -1,    73,    74,    75,
      17,    -1,    -1,    53,    -1,    22,    23,    24,    -1,    59,
      -1,    53,    29,    -1,    -1,    -1,    -1,    59,    -1,    36,
      -1,    71,    -1,    73,    74,     8,    -1,    -1,    -1,    71,
      -1,    73,    74,    -1,    17,    -1,    53,    -1,    -1,    22,
      23,    24,    59,    -1,    -1,    -1,    29,    -1,    -1,    -1,
       8,    -1,    -1,    36,    71,    -1,    73,    74,     8,    17,
      -1,    -1,    -1,    -1,    22,    23,    24,    17,    -1,    -1,
      53,    29,    22,    23,    24,    -1,    59,    -1,    36,    29,
      -1,    -1,    -1,     8,    -1,    -1,    36,    -1,    71,    -1,
      73,    74,    17,    -1,    -1,    53,    -1,    22,    23,    24,
      -1,    59,    -1,    53,    29,    -1,    -1,    -1,    -1,    59,
      -1,    36,    -1,    71,    -1,    73,    74,     8,    -1,    -1,
      -1,    71,    -1,    -1,    74,    -1,    17,    -1,    53,    -1,
      -1,    22,    23,    24,    -1,    -1,    -1,    -1,    29,    -1,
      -1,    -1,     8,    -1,    -1,    36,    71,    -1,    73,    74,
      -1,    17,    -1,    -1,    -1,    -1,    22,    23,    24,    -1,
      -1,    -1,    53,    29,    -1,    -1,    -1,    -1,    59,    -1,
      36,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      71,    -1,    -1,    74,    -1,    -1,    -1,    53,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    71,    33,    34,    74,    36,
      -1,    38,    39,    40,    -1,    -1,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    33,
      34,    -1,    36,    -1,    38,    39,    40,    -1,    75,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    -1,    -1,    -1,    -1,    -1,    -1,    33,
      34,    -1,    36,    -1,    38,    39,    40,    -1,    72,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    33,    34,    -1,    36,    -1,    38,    39,
      40,    -1,    66,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    33,    34,    -1,
      36,    -1,    38,    39,    40,    -1,    66,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    33,    34,    -1,    36,    -1,    38,    39,    40,    -1,
      66,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    33,    34,    -1,    -1,    -1,
      38,    39,    40,    -1,    -1,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    33,
      34,    -1,    -1,    -1,    38,    39,    40,    -1,    -1,    43,
      44,    45,    46,    -1,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    34,    -1,    -1,    -1,    38,    39,    40,
      -1,    -1,    43,    44,    45,    46,    -1,    48,    49,    50,
      51,    52,    53,    54,    55,    56
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    77,    79,    80,     0,    25,    78,    25,    86,    24,
      73,    74,   141,   142,    81,    24,    88,    89,     3,    62,
      21,    82,   166,    24,    87,   214,    63,     3,    59,    63,
      83,    85,   141,    62,     1,     3,     6,     8,     9,    10,
      13,    15,    16,    17,    18,    19,    20,    22,    23,    27,
      28,    29,    30,    31,    32,    36,    49,    50,    52,    53,
      56,    59,    69,    70,    71,    90,    91,    92,    98,   110,
     113,   121,   124,   126,   127,   128,   129,   134,   138,   141,
     143,   144,   149,   150,   153,   156,   157,   158,   161,   164,
     165,   181,   186,    62,     9,    17,    21,    31,    32,    64,
     199,    24,    73,    60,    83,    84,     3,    86,    88,     3,
     138,   140,   141,    17,    36,    53,    59,   141,   143,   148,
     152,   153,   154,   161,   140,   128,   134,   111,    59,   141,
     159,   128,   138,   114,    35,    67,   137,    71,   126,   186,
     193,   125,   137,   122,    59,    96,    97,   141,    59,    93,
     139,   141,   185,   127,   127,   127,   127,   127,   127,    36,
      53,   126,   135,   147,   153,   155,   161,   127,   127,    11,
     126,   192,    62,    59,    94,   185,     4,    33,    34,    36,
      37,    38,    39,    40,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    67,
      59,    63,    71,    66,    59,   137,     1,   137,     5,    65,
      75,   142,   200,    59,   160,   200,    24,   200,   201,   200,
      64,    62,   190,    88,    59,    36,    59,   146,   152,   153,
     154,   155,   161,   146,   146,    63,    98,   107,   108,   109,
     186,   194,    11,   136,   141,   145,   146,   177,   178,   179,
      59,    67,   162,   112,   194,    24,    59,    68,   138,   171,
     173,   175,   146,    35,    53,    59,    68,   138,   170,   172,
     173,   174,   184,   112,    60,    97,   169,    65,   146,    60,
      93,   167,    65,    75,   146,     8,   147,    60,    72,    72,
      60,    94,    65,   146,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   130,    60,   135,   187,    59,
     141,   126,   192,   182,   126,   130,     1,    67,    91,   100,
     180,   181,   183,   186,   186,   126,     8,    17,    22,    23,
      24,    29,    36,    53,    65,    71,   142,   202,   204,   205,
     206,   141,   207,   215,   162,    59,     3,   202,   202,    83,
      60,   179,     8,   146,    60,   141,    35,   105,     5,    65,
      62,   146,   136,   145,    75,   191,    60,   179,   183,   115,
      62,    63,    24,   173,    59,   176,    62,   190,    72,   104,
      59,   174,    53,   174,    62,   190,     3,   198,    75,   146,
     123,    62,   190,   146,    62,   190,   186,   139,    65,    36,
      59,   146,   152,   153,   154,   161,    67,   146,   146,    62,
     190,   186,    65,    67,   126,   131,   132,   188,   189,    11,
      75,   191,    31,   135,    72,    66,   180,    75,   191,   189,
     101,    62,    68,    36,    59,   203,   204,   206,    59,    67,
      71,    67,     8,   202,     3,    50,    59,   141,   212,   213,
       3,    72,    65,    11,   202,    60,    75,    62,   195,   215,
      62,    62,    62,    60,    60,   106,    26,    26,   194,   177,
      59,   141,   151,   152,   153,   154,   155,   161,   163,    60,
      68,   105,   194,   141,    60,   179,   175,    68,   146,     7,
      12,    68,    99,   102,   174,   198,   174,    60,   172,    68,
     138,   198,    35,    97,    60,    93,    60,   186,   146,   130,
      94,    95,   168,   185,    60,   186,   130,    66,    75,   191,
      68,   191,   135,    60,    60,    60,   192,    60,    68,   183,
     180,   202,   205,   195,    24,   141,   142,   197,   202,   209,
     217,   202,   141,   196,   208,   216,   202,     3,   212,    62,
      72,   202,   213,   202,   198,   141,   207,    60,   183,   126,
     126,    62,   179,    59,   163,   116,    60,   187,    66,   103,
      60,    60,   198,   104,    60,   189,    62,   190,   146,   189,
      67,   126,   133,   131,   132,    60,    66,    72,    68,    60,
      60,    59,    68,    62,    72,   202,    68,    62,    49,   202,
      62,   198,    59,    59,   202,   210,   211,    68,   194,    60,
     179,   119,   163,     5,    65,    66,    75,   183,   198,   198,
      68,    68,    95,    60,    68,   130,   192,   210,   195,   209,
     202,   198,   208,   212,   195,   195,    60,    14,   117,   120,
     126,   126,   189,    72,    60,    60,    60,    60,   163,    20,
     100,    66,    66,    68,   210,   210,   118,   112,   105
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* This macro is provided for backward compatibility. */

#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  YYSIZE_T yysize1;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = 0;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                yysize1 = yysize + yytnamerr (0, yytname[yyx]);
                if (! (yysize <= yysize1
                       && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                  return 2;
                yysize = yysize1;
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  yysize1 = yysize + yystrlen (yyformat);
  if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
    return 2;
  yysize = yysize1;

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs, yystate;


/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1806 of yacc.c  */
#line 132 "go.y"
    {
		xtop = concat(xtop, (yyvsp[(4) - (4)].list));
	}
    break;

  case 3:

/* Line 1806 of yacc.c  */
#line 138 "go.y"
    {
		prevlineno = lineno;
		yyerror("package statement must be first");
		errorexit();
	}
    break;

  case 4:

/* Line 1806 of yacc.c  */
#line 144 "go.y"
    {
		mkpackage((yyvsp[(2) - (3)].sym)->name);
	}
    break;

  case 5:

/* Line 1806 of yacc.c  */
#line 154 "go.y"
    {
		importpkg = runtimepkg;

		if(debug['A'])
			cannedimports("runtime.builtin", "package runtime\n\n$$\n\n");
		else
			cannedimports("runtime.builtin", runtimeimport);
		curio.importsafe = 1;
	}
    break;

  case 6:

/* Line 1806 of yacc.c  */
#line 165 "go.y"
    {
		importpkg = nil;
	}
    break;

  case 12:

/* Line 1806 of yacc.c  */
#line 179 "go.y"
    {
		Pkg *ipkg;
		Sym *my;
		Node *pack;
		
		ipkg = importpkg;
		my = importmyname;
		importpkg = nil;
		importmyname = S;

		if(my == nil)
			my = lookup(ipkg->name);

		pack = nod(OPACK, N, N);
		pack->sym = my;
		pack->pkg = ipkg;
		pack->lineno = (yyvsp[(1) - (3)].i);

		if(my->name[0] == '.') {
			importdot(ipkg, pack);
			break;
		}
		if(strcmp(my->name, "init") == 0) {
			yyerror("cannot import package as init - init must be a func");
			break;
		}
		if(my->name[0] == '_' && my->name[1] == '\0')
			break;
		if(my->def) {
			lineno = (yyvsp[(1) - (3)].i);
			redeclare(my, "as imported package name");
		}
		my->def = pack;
		my->lastlineno = (yyvsp[(1) - (3)].i);
		my->block = 1;	// at top level
	}
    break;

  case 13:

/* Line 1806 of yacc.c  */
#line 216 "go.y"
    {
		// When an invalid import path is passed to importfile,
		// it calls yyerror and then sets up a fake import with
		// no package statement. This allows us to test more
		// than one invalid import statement in a single file.
		if(nerrors == 0)
			fatal("phase error in import");
	}
    break;

  case 16:

/* Line 1806 of yacc.c  */
#line 231 "go.y"
    {
		// import with original name
		(yyval.i) = parserline();
		importmyname = S;
		importfile(&(yyvsp[(1) - (1)].val), (yyval.i));
	}
    break;

  case 17:

/* Line 1806 of yacc.c  */
#line 238 "go.y"
    {
		// import with given name
		(yyval.i) = parserline();
		importmyname = (yyvsp[(1) - (2)].sym);
		importfile(&(yyvsp[(2) - (2)].val), (yyval.i));
	}
    break;

  case 18:

/* Line 1806 of yacc.c  */
#line 245 "go.y"
    {
		// import into my name space
		(yyval.i) = parserline();
		importmyname = lookup(".");
		importfile(&(yyvsp[(2) - (2)].val), (yyval.i));
	}
    break;

  case 19:

/* Line 1806 of yacc.c  */
#line 254 "go.y"
    {
		if(importpkg->name == nil) {
			importpkg->name = (yyvsp[(2) - (4)].sym)->name;
			pkglookup((yyvsp[(2) - (4)].sym)->name, nil)->npkg++;
		} else if(strcmp(importpkg->name, (yyvsp[(2) - (4)].sym)->name) != 0)
			yyerror("conflicting names %s and %s for package \"%Z\"", importpkg->name, (yyvsp[(2) - (4)].sym)->name, importpkg->path);
		importpkg->direct = 1;
		importpkg->safe = curio.importsafe;

		if(safemode && !curio.importsafe)
			yyerror("cannot import unsafe package \"%Z\"", importpkg->path);
	}
    break;

  case 21:

/* Line 1806 of yacc.c  */
#line 269 "go.y"
    {
		if(strcmp((yyvsp[(1) - (1)].sym)->name, "safe") == 0)
			curio.importsafe = 1;
	}
    break;

  case 22:

/* Line 1806 of yacc.c  */
#line 275 "go.y"
    {
		defercheckwidth();
	}
    break;

  case 23:

/* Line 1806 of yacc.c  */
#line 279 "go.y"
    {
		resumecheckwidth();
		unimportfile();
	}
    break;

  case 24:

/* Line 1806 of yacc.c  */
#line 288 "go.y"
    {
		yyerror("empty top-level declaration");
		(yyval.list) = nil;
	}
    break;

  case 26:

/* Line 1806 of yacc.c  */
#line 294 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 27:

/* Line 1806 of yacc.c  */
#line 298 "go.y"
    {
		yyerror("non-declaration statement outside function body");
		(yyval.list) = nil;
	}
    break;

  case 28:

/* Line 1806 of yacc.c  */
#line 303 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 29:

/* Line 1806 of yacc.c  */
#line 314 "go.y"
    {
		(yyval.list) = (yyvsp[(2) - (2)].list);
	}
    break;

  case 30:

/* Line 1806 of yacc.c  */
#line 322 "go.y"
    {
		(yyval.list) = (yyvsp[(3) - (5)].list);
	}
    break;

  case 31:

/* Line 1806 of yacc.c  */
#line 328 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 32:

/* Line 1806 of yacc.c  */
#line 332 "go.y"
    {
		(yyval.list) = (yyvsp[(2) - (2)].list);
		iota = -100000;
		lastconst = nil;
	}
    break;

  case 33:

/* Line 1806 of yacc.c  */
#line 338 "go.y"
    {
		(yyval.list) = (yyvsp[(3) - (5)].list);
		iota = -100000;
		lastconst = nil;
	}
    break;

  case 34:

/* Line 1806 of yacc.c  */
#line 344 "go.y"
    {
		(yyval.list) = concat((yyvsp[(3) - (7)].list), (yyvsp[(5) - (7)].list));
		iota = -100000;
		lastconst = nil;
	}
    break;

  case 35:

/* Line 1806 of yacc.c  */
#line 350 "go.y"
    {
		(yyval.list) = nil;
		iota = -100000;
	}
    break;

  case 36:

/* Line 1806 of yacc.c  */
#line 356 "go.y"
    {
		(yyval.list) = list1((yyvsp[(2) - (2)].node));
	}
    break;

  case 37:

/* Line 1806 of yacc.c  */
#line 364 "go.y"
    {
		(yyval.list) = (yyvsp[(3) - (5)].list);
	}
    break;

  case 38:

/* Line 1806 of yacc.c  */
#line 368 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 39:

/* Line 1806 of yacc.c  */
#line 374 "go.y"
    {
		iota = 0;
	}
    break;

  case 40:

/* Line 1806 of yacc.c  */
#line 383 "go.y"
    {
		(yyval.list) = variter((yyvsp[(1) - (2)].list), (yyvsp[(2) - (2)].node), nil);
	}
    break;

  case 41:

/* Line 1806 of yacc.c  */
#line 388 "go.y"
    {
		(yyval.list) = variter((yyvsp[(1) - (4)].list), (yyvsp[(2) - (4)].node), (yyvsp[(4) - (4)].list));
	}
    break;

  case 42:

/* Line 1806 of yacc.c  */
#line 393 "go.y"
    {
		(yyval.list) = variter((yyvsp[(1) - (3)].list), nil, (yyvsp[(3) - (3)].list));
	}
    break;

  case 43:

/* Line 1806 of yacc.c  */
#line 399 "go.y"
    {
		(yyval.list) = constiter((yyvsp[(1) - (4)].list), (yyvsp[(2) - (4)].node), (yyvsp[(4) - (4)].list));
	}
    break;

  case 44:

/* Line 1806 of yacc.c  */
#line 403 "go.y"
    {
		(yyval.list) = constiter((yyvsp[(1) - (3)].list), N, (yyvsp[(3) - (3)].list));
	}
    break;

  case 46:

/* Line 1806 of yacc.c  */
#line 410 "go.y"
    {
		(yyval.list) = constiter((yyvsp[(1) - (2)].list), (yyvsp[(2) - (2)].node), nil);
	}
    break;

  case 47:

/* Line 1806 of yacc.c  */
#line 414 "go.y"
    {
		(yyval.list) = constiter((yyvsp[(1) - (1)].list), N, nil);
	}
    break;

  case 48:

/* Line 1806 of yacc.c  */
#line 420 "go.y"
    {
		// different from dclname because the name
		// becomes visible right here, not at the end
		// of the declaration.
		(yyval.node) = typedcl0((yyvsp[(1) - (1)].sym));
	}
    break;

  case 49:

/* Line 1806 of yacc.c  */
#line 432 "go.y"
    {
		(yyval.node) = typedcl1((yyvsp[(1) - (2)].node), (yyvsp[(2) - (2)].node), 1);
	}
    break;

  case 50:

/* Line 1806 of yacc.c  */
#line 437 "go.y"
    {
		(yyval.node) = typedcl1((yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node), 1);
	}
    break;

  case 51:

/* Line 1806 of yacc.c  */
#line 443 "go.y"
    {
		(yyval.node) = (yyvsp[(1) - (1)].node);

		// These nodes do not carry line numbers.
		// Since a bare name used as an expression is an error,
		// introduce a wrapper node to give the correct line.
		switch((yyval.node)->op) {
		case ONAME:
		case ONONAME:
		case OTYPE:
		case OPACK:
		case OLITERAL:
			(yyval.node) = nod(OPAREN, (yyval.node), N);
			(yyval.node)->implicit = 1;
			break;
		}
	}
    break;

  case 52:

/* Line 1806 of yacc.c  */
#line 461 "go.y"
    {
		(yyval.node) = nod(OASOP, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
		(yyval.node)->etype = (yyvsp[(2) - (3)].i);			// rathole to pass opcode
	}
    break;

  case 53:

/* Line 1806 of yacc.c  */
#line 466 "go.y"
    {
		if((yyvsp[(1) - (3)].list)->next == nil && (yyvsp[(3) - (3)].list)->next == nil) {
			// simple
			(yyval.node) = nod(OAS, (yyvsp[(1) - (3)].list)->n, (yyvsp[(3) - (3)].list)->n);
			break;
		}
		// multiple
		(yyval.node) = nod(OAS2, N, N);
		(yyval.node)->list = (yyvsp[(1) - (3)].list);
		(yyval.node)->rlist = (yyvsp[(3) - (3)].list);
	}
    break;

  case 54:

/* Line 1806 of yacc.c  */
#line 478 "go.y"
    {
		if((yyvsp[(3) - (3)].list)->n->op == OTYPESW) {
			(yyval.node) = nod(OTYPESW, N, (yyvsp[(3) - (3)].list)->n->right);
			if((yyvsp[(3) - (3)].list)->next != nil)
				yyerror("expr.(type) must be alone in list");
			if((yyvsp[(1) - (3)].list)->next != nil)
				yyerror("argument count mismatch: %d = %d", count((yyvsp[(1) - (3)].list)), 1);
			else if(((yyvsp[(1) - (3)].list)->n->op != ONAME && (yyvsp[(1) - (3)].list)->n->op != OTYPE && (yyvsp[(1) - (3)].list)->n->op != ONONAME) || isblank((yyvsp[(1) - (3)].list)->n))
				yyerror("invalid variable name %N in type switch", (yyvsp[(1) - (3)].list)->n);
			else
				(yyval.node)->left = dclname((yyvsp[(1) - (3)].list)->n->sym);  // it's a colas, so must not re-use an oldname.
			break;
		}
		(yyval.node) = colas((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].list), (yyvsp[(2) - (3)].i));
	}
    break;

  case 55:

/* Line 1806 of yacc.c  */
#line 494 "go.y"
    {
		(yyval.node) = nod(OASOP, (yyvsp[(1) - (2)].node), nodintconst(1));
		(yyval.node)->etype = OADD;
	}
    break;

  case 56:

/* Line 1806 of yacc.c  */
#line 499 "go.y"
    {
		(yyval.node) = nod(OASOP, (yyvsp[(1) - (2)].node), nodintconst(1));
		(yyval.node)->etype = OSUB;
	}
    break;

  case 57:

/* Line 1806 of yacc.c  */
#line 506 "go.y"
    {
		Node *n, *nn;

		// will be converted to OCASE
		// right will point to next case
		// done in casebody()
		markdcl();
		(yyval.node) = nod(OXCASE, N, N);
		(yyval.node)->list = (yyvsp[(2) - (3)].list);
		if(typesw != N && typesw->right != N && (n=typesw->right->left) != N) {
			// type switch - declare variable
			nn = newname(n->sym);
			declare(nn, dclcontext);
			(yyval.node)->nname = nn;

			// keep track of the instances for reporting unused
			nn->defn = typesw->right;
		}
	}
    break;

  case 58:

/* Line 1806 of yacc.c  */
#line 526 "go.y"
    {
		Node *n;

		// will be converted to OCASE
		// right will point to next case
		// done in casebody()
		markdcl();
		(yyval.node) = nod(OXCASE, N, N);
		if((yyvsp[(2) - (5)].list)->next == nil)
			n = nod(OAS, (yyvsp[(2) - (5)].list)->n, (yyvsp[(4) - (5)].node));
		else {
			n = nod(OAS2, N, N);
			n->list = (yyvsp[(2) - (5)].list);
			n->rlist = list1((yyvsp[(4) - (5)].node));
		}
		(yyval.node)->list = list1(n);
	}
    break;

  case 59:

/* Line 1806 of yacc.c  */
#line 544 "go.y"
    {
		// will be converted to OCASE
		// right will point to next case
		// done in casebody()
		markdcl();
		(yyval.node) = nod(OXCASE, N, N);
		(yyval.node)->list = list1(colas((yyvsp[(2) - (5)].list), list1((yyvsp[(4) - (5)].node)), (yyvsp[(3) - (5)].i)));
	}
    break;

  case 60:

/* Line 1806 of yacc.c  */
#line 553 "go.y"
    {
		Node *n, *nn;

		markdcl();
		(yyval.node) = nod(OXCASE, N, N);
		if(typesw != N && typesw->right != N && (n=typesw->right->left) != N) {
			// type switch - declare variable
			nn = newname(n->sym);
			declare(nn, dclcontext);
			(yyval.node)->nname = nn;

			// keep track of the instances for reporting unused
			nn->defn = typesw->right;
		}
	}
    break;

  case 61:

/* Line 1806 of yacc.c  */
#line 571 "go.y"
    {
		markdcl();
	}
    break;

  case 62:

/* Line 1806 of yacc.c  */
#line 575 "go.y"
    {
		if((yyvsp[(3) - (4)].list) == nil)
			(yyval.node) = nod(OEMPTY, N, N);
		else
			(yyval.node) = liststmt((yyvsp[(3) - (4)].list));
		popdcl();
	}
    break;

  case 63:

/* Line 1806 of yacc.c  */
#line 585 "go.y"
    {
		// If the last token read by the lexer was consumed
		// as part of the case, clear it (parser has cleared yychar).
		// If the last token read by the lexer was the lookahead
		// leave it alone (parser has it cached in yychar).
		// This is so that the stmt_list action doesn't look at
		// the case tokens if the stmt_list is empty.
		yylast = yychar;
	}
    break;

  case 64:

/* Line 1806 of yacc.c  */
#line 595 "go.y"
    {
		int last;

		// This is the only place in the language where a statement
		// list is not allowed to drop the final semicolon, because
		// it's the only place where a statement list is not followed 
		// by a closing brace.  Handle the error for pedantry.

		// Find the final token of the statement list.
		// yylast is lookahead; yyprev is last of stmt_list
		last = yyprev;

		if(last > 0 && last != ';' && yychar != '}')
			yyerror("missing statement after label");
		(yyval.node) = (yyvsp[(1) - (3)].node);
		(yyval.node)->nbody = (yyvsp[(3) - (3)].list);
		popdcl();
	}
    break;

  case 65:

/* Line 1806 of yacc.c  */
#line 615 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 66:

/* Line 1806 of yacc.c  */
#line 619 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (2)].list), (yyvsp[(2) - (2)].node));
	}
    break;

  case 67:

/* Line 1806 of yacc.c  */
#line 625 "go.y"
    {
		markdcl();
	}
    break;

  case 68:

/* Line 1806 of yacc.c  */
#line 629 "go.y"
    {
		(yyval.list) = (yyvsp[(3) - (4)].list);
		popdcl();
	}
    break;

  case 69:

/* Line 1806 of yacc.c  */
#line 636 "go.y"
    {
		(yyval.node) = nod(ORANGE, N, (yyvsp[(4) - (4)].node));
		(yyval.node)->list = (yyvsp[(1) - (4)].list);
		(yyval.node)->etype = 0;	// := flag
	}
    break;

  case 70:

/* Line 1806 of yacc.c  */
#line 642 "go.y"
    {
		(yyval.node) = nod(ORANGE, N, (yyvsp[(4) - (4)].node));
		(yyval.node)->list = (yyvsp[(1) - (4)].list);
		(yyval.node)->colas = 1;
		colasdefn((yyvsp[(1) - (4)].list), (yyval.node));
	}
    break;

  case 71:

/* Line 1806 of yacc.c  */
#line 651 "go.y"
    {
		// init ; test ; incr
		if((yyvsp[(5) - (5)].node) != N && (yyvsp[(5) - (5)].node)->colas != 0)
			yyerror("cannot declare in the for-increment");
		(yyval.node) = nod(OFOR, N, N);
		if((yyvsp[(1) - (5)].node) != N)
			(yyval.node)->ninit = list1((yyvsp[(1) - (5)].node));
		(yyval.node)->ntest = (yyvsp[(3) - (5)].node);
		(yyval.node)->nincr = (yyvsp[(5) - (5)].node);
	}
    break;

  case 72:

/* Line 1806 of yacc.c  */
#line 662 "go.y"
    {
		// normal test
		(yyval.node) = nod(OFOR, N, N);
		(yyval.node)->ntest = (yyvsp[(1) - (1)].node);
	}
    break;

  case 74:

/* Line 1806 of yacc.c  */
#line 671 "go.y"
    {
		(yyval.node) = (yyvsp[(1) - (2)].node);
		(yyval.node)->nbody = concat((yyval.node)->nbody, (yyvsp[(2) - (2)].list));
	}
    break;

  case 75:

/* Line 1806 of yacc.c  */
#line 678 "go.y"
    {
		markdcl();
	}
    break;

  case 76:

/* Line 1806 of yacc.c  */
#line 682 "go.y"
    {
		(yyval.node) = (yyvsp[(3) - (3)].node);
		popdcl();
	}
    break;

  case 77:

/* Line 1806 of yacc.c  */
#line 689 "go.y"
    {
		// test
		(yyval.node) = nod(OIF, N, N);
		(yyval.node)->ntest = (yyvsp[(1) - (1)].node);
	}
    break;

  case 78:

/* Line 1806 of yacc.c  */
#line 695 "go.y"
    {
		// init ; test
		(yyval.node) = nod(OIF, N, N);
		if((yyvsp[(1) - (3)].node) != N)
			(yyval.node)->ninit = list1((yyvsp[(1) - (3)].node));
		(yyval.node)->ntest = (yyvsp[(3) - (3)].node);
	}
    break;

  case 79:

/* Line 1806 of yacc.c  */
#line 706 "go.y"
    {
		markdcl();
	}
    break;

  case 80:

/* Line 1806 of yacc.c  */
#line 710 "go.y"
    {
		if((yyvsp[(3) - (3)].node)->ntest == N)
			yyerror("missing condition in if statement");
	}
    break;

  case 81:

/* Line 1806 of yacc.c  */
#line 715 "go.y"
    {
		(yyvsp[(3) - (5)].node)->nbody = (yyvsp[(5) - (5)].list);
	}
    break;

  case 82:

/* Line 1806 of yacc.c  */
#line 719 "go.y"
    {
		Node *n;
		NodeList *nn;

		(yyval.node) = (yyvsp[(3) - (8)].node);
		n = (yyvsp[(3) - (8)].node);
		popdcl();
		for(nn = concat((yyvsp[(7) - (8)].list), (yyvsp[(8) - (8)].list)); nn; nn = nn->next) {
			if(nn->n->op == OIF)
				popdcl();
			n->nelse = list1(nn->n);
			n = nn->n;
		}
	}
    break;

  case 83:

/* Line 1806 of yacc.c  */
#line 736 "go.y"
    {
		markdcl();
	}
    break;

  case 84:

/* Line 1806 of yacc.c  */
#line 740 "go.y"
    {
		if((yyvsp[(4) - (5)].node)->ntest == N)
			yyerror("missing condition in if statement");
		(yyvsp[(4) - (5)].node)->nbody = (yyvsp[(5) - (5)].list);
		(yyval.list) = list1((yyvsp[(4) - (5)].node));
	}
    break;

  case 85:

/* Line 1806 of yacc.c  */
#line 748 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 86:

/* Line 1806 of yacc.c  */
#line 752 "go.y"
    {
		(yyval.list) = concat((yyvsp[(1) - (2)].list), (yyvsp[(2) - (2)].list));
	}
    break;

  case 87:

/* Line 1806 of yacc.c  */
#line 757 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 88:

/* Line 1806 of yacc.c  */
#line 761 "go.y"
    {
		NodeList *node;
		
		node = mal(sizeof *node);
		node->n = (yyvsp[(2) - (2)].node);
		node->end = node;
		(yyval.list) = node;
	}
    break;

  case 89:

/* Line 1806 of yacc.c  */
#line 772 "go.y"
    {
		markdcl();
	}
    break;

  case 90:

/* Line 1806 of yacc.c  */
#line 776 "go.y"
    {
		Node *n;
		n = (yyvsp[(3) - (3)].node)->ntest;
		if(n != N && n->op != OTYPESW)
			n = N;
		typesw = nod(OXXX, typesw, n);
	}
    break;

  case 91:

/* Line 1806 of yacc.c  */
#line 784 "go.y"
    {
		(yyval.node) = (yyvsp[(3) - (7)].node);
		(yyval.node)->op = OSWITCH;
		(yyval.node)->list = (yyvsp[(6) - (7)].list);
		typesw = typesw->left;
		popdcl();
	}
    break;

  case 92:

/* Line 1806 of yacc.c  */
#line 794 "go.y"
    {
		typesw = nod(OXXX, typesw, N);
	}
    break;

  case 93:

/* Line 1806 of yacc.c  */
#line 798 "go.y"
    {
		(yyval.node) = nod(OSELECT, N, N);
		(yyval.node)->lineno = typesw->lineno;
		(yyval.node)->list = (yyvsp[(4) - (5)].list);
		typesw = typesw->left;
	}
    break;

  case 95:

/* Line 1806 of yacc.c  */
#line 811 "go.y"
    {
		(yyval.node) = nod(OOROR, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 96:

/* Line 1806 of yacc.c  */
#line 815 "go.y"
    {
		(yyval.node) = nod(OANDAND, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 97:

/* Line 1806 of yacc.c  */
#line 819 "go.y"
    {
		(yyval.node) = nod(OEQ, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 98:

/* Line 1806 of yacc.c  */
#line 823 "go.y"
    {
		(yyval.node) = nod(ONE, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 99:

/* Line 1806 of yacc.c  */
#line 827 "go.y"
    {
		(yyval.node) = nod(OLT, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 100:

/* Line 1806 of yacc.c  */
#line 831 "go.y"
    {
		(yyval.node) = nod(OLE, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 101:

/* Line 1806 of yacc.c  */
#line 835 "go.y"
    {
		(yyval.node) = nod(OGE, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 102:

/* Line 1806 of yacc.c  */
#line 839 "go.y"
    {
		(yyval.node) = nod(OGT, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 103:

/* Line 1806 of yacc.c  */
#line 843 "go.y"
    {
		(yyval.node) = nod(OADD, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 104:

/* Line 1806 of yacc.c  */
#line 847 "go.y"
    {
		(yyval.node) = nod(OSUB, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 105:

/* Line 1806 of yacc.c  */
#line 851 "go.y"
    {
		(yyval.node) = nod(OOR, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 106:

/* Line 1806 of yacc.c  */
#line 855 "go.y"
    {
		(yyval.node) = nod(OXOR, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 107:

/* Line 1806 of yacc.c  */
#line 859 "go.y"
    {
		(yyval.node) = nod(OMUL, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 108:

/* Line 1806 of yacc.c  */
#line 863 "go.y"
    {
		(yyval.node) = nod(ODIV, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 109:

/* Line 1806 of yacc.c  */
#line 867 "go.y"
    {
		(yyval.node) = nod(OMOD, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 110:

/* Line 1806 of yacc.c  */
#line 871 "go.y"
    {
		(yyval.node) = nod(OAND, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 111:

/* Line 1806 of yacc.c  */
#line 875 "go.y"
    {
		(yyval.node) = nod(OANDNOT, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 112:

/* Line 1806 of yacc.c  */
#line 879 "go.y"
    {
		(yyval.node) = nod(OLSH, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 113:

/* Line 1806 of yacc.c  */
#line 883 "go.y"
    {
		(yyval.node) = nod(ORSH, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 114:

/* Line 1806 of yacc.c  */
#line 888 "go.y"
    {
		(yyval.node) = nod(OSEND, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 116:

/* Line 1806 of yacc.c  */
#line 895 "go.y"
    {
		(yyval.node) = nod(OIND, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 117:

/* Line 1806 of yacc.c  */
#line 899 "go.y"
    {
		if((yyvsp[(2) - (2)].node)->op == OCOMPLIT) {
			// Special case for &T{...}: turn into (*T){...}.
			(yyval.node) = (yyvsp[(2) - (2)].node);
			(yyval.node)->right = nod(OIND, (yyval.node)->right, N);
			(yyval.node)->right->implicit = 1;
		} else {
			(yyval.node) = nod(OADDR, (yyvsp[(2) - (2)].node), N);
		}
	}
    break;

  case 118:

/* Line 1806 of yacc.c  */
#line 910 "go.y"
    {
		(yyval.node) = nod(OPLUS, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 119:

/* Line 1806 of yacc.c  */
#line 914 "go.y"
    {
		(yyval.node) = nod(OMINUS, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 120:

/* Line 1806 of yacc.c  */
#line 918 "go.y"
    {
		(yyval.node) = nod(ONOT, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 121:

/* Line 1806 of yacc.c  */
#line 922 "go.y"
    {
		yyerror("the bitwise complement operator is ^");
		(yyval.node) = nod(OCOM, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 122:

/* Line 1806 of yacc.c  */
#line 927 "go.y"
    {
		(yyval.node) = nod(OCOM, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 123:

/* Line 1806 of yacc.c  */
#line 931 "go.y"
    {
		(yyval.node) = nod(ORECV, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 124:

/* Line 1806 of yacc.c  */
#line 941 "go.y"
    {
		(yyval.node) = nod(OCALL, (yyvsp[(1) - (3)].node), N);
	}
    break;

  case 125:

/* Line 1806 of yacc.c  */
#line 945 "go.y"
    {
		(yyval.node) = nod(OCALL, (yyvsp[(1) - (5)].node), N);
		(yyval.node)->list = (yyvsp[(3) - (5)].list);
	}
    break;

  case 126:

/* Line 1806 of yacc.c  */
#line 950 "go.y"
    {
		(yyval.node) = nod(OCALL, (yyvsp[(1) - (6)].node), N);
		(yyval.node)->list = (yyvsp[(3) - (6)].list);
		(yyval.node)->isddd = 1;
	}
    break;

  case 127:

/* Line 1806 of yacc.c  */
#line 958 "go.y"
    {
		(yyval.node) = nodlit((yyvsp[(1) - (1)].val));
	}
    break;

  case 129:

/* Line 1806 of yacc.c  */
#line 963 "go.y"
    {
		if((yyvsp[(1) - (3)].node)->op == OPACK) {
			Sym *s;
			s = restrictlookup((yyvsp[(3) - (3)].sym)->name, (yyvsp[(1) - (3)].node)->pkg);
			(yyvsp[(1) - (3)].node)->used = 1;
			(yyval.node) = oldname(s);
			break;
		}
		(yyval.node) = nod(OXDOT, (yyvsp[(1) - (3)].node), newname((yyvsp[(3) - (3)].sym)));
	}
    break;

  case 130:

/* Line 1806 of yacc.c  */
#line 974 "go.y"
    {
		(yyval.node) = nod(ODOTTYPE, (yyvsp[(1) - (5)].node), (yyvsp[(4) - (5)].node));
	}
    break;

  case 131:

/* Line 1806 of yacc.c  */
#line 978 "go.y"
    {
		(yyval.node) = nod(OTYPESW, N, (yyvsp[(1) - (5)].node));
	}
    break;

  case 132:

/* Line 1806 of yacc.c  */
#line 982 "go.y"
    {
		(yyval.node) = nod(OINDEX, (yyvsp[(1) - (4)].node), (yyvsp[(3) - (4)].node));
	}
    break;

  case 133:

/* Line 1806 of yacc.c  */
#line 986 "go.y"
    {
		(yyval.node) = nod(OSLICE, (yyvsp[(1) - (6)].node), nod(OKEY, (yyvsp[(3) - (6)].node), (yyvsp[(5) - (6)].node)));
	}
    break;

  case 134:

/* Line 1806 of yacc.c  */
#line 990 "go.y"
    {
		if((yyvsp[(5) - (8)].node) == N)
			yyerror("middle index required in 3-index slice");
		if((yyvsp[(7) - (8)].node) == N)
			yyerror("final index required in 3-index slice");
		(yyval.node) = nod(OSLICE3, (yyvsp[(1) - (8)].node), nod(OKEY, (yyvsp[(3) - (8)].node), nod(OKEY, (yyvsp[(5) - (8)].node), (yyvsp[(7) - (8)].node))));
	}
    break;

  case 136:

/* Line 1806 of yacc.c  */
#line 999 "go.y"
    {
		// conversion
		(yyval.node) = nod(OCALL, (yyvsp[(1) - (5)].node), N);
		(yyval.node)->list = list1((yyvsp[(3) - (5)].node));
	}
    break;

  case 137:

/* Line 1806 of yacc.c  */
#line 1005 "go.y"
    {
		(yyval.node) = (yyvsp[(3) - (5)].node);
		(yyval.node)->right = (yyvsp[(1) - (5)].node);
		(yyval.node)->list = (yyvsp[(4) - (5)].list);
		fixlbrace((yyvsp[(2) - (5)].i));
	}
    break;

  case 138:

/* Line 1806 of yacc.c  */
#line 1012 "go.y"
    {
		(yyval.node) = (yyvsp[(3) - (5)].node);
		(yyval.node)->right = (yyvsp[(1) - (5)].node);
		(yyval.node)->list = (yyvsp[(4) - (5)].list);
	}
    break;

  case 139:

/* Line 1806 of yacc.c  */
#line 1018 "go.y"
    {
		yyerror("cannot parenthesize type in composite literal");
		(yyval.node) = (yyvsp[(5) - (7)].node);
		(yyval.node)->right = (yyvsp[(2) - (7)].node);
		(yyval.node)->list = (yyvsp[(6) - (7)].list);
	}
    break;

  case 141:

/* Line 1806 of yacc.c  */
#line 1027 "go.y"
    {
		// composite expression.
		// make node early so we get the right line number.
		(yyval.node) = nod(OCOMPLIT, N, N);
	}
    break;

  case 142:

/* Line 1806 of yacc.c  */
#line 1035 "go.y"
    {
		(yyval.node) = nod(OKEY, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 143:

/* Line 1806 of yacc.c  */
#line 1041 "go.y"
    {
		// These nodes do not carry line numbers.
		// Since a composite literal commonly spans several lines,
		// the line number on errors may be misleading.
		// Introduce a wrapper node to give the correct line.
		(yyval.node) = (yyvsp[(1) - (1)].node);
		switch((yyval.node)->op) {
		case ONAME:
		case ONONAME:
		case OTYPE:
		case OPACK:
		case OLITERAL:
			(yyval.node) = nod(OPAREN, (yyval.node), N);
			(yyval.node)->implicit = 1;
		}
	}
    break;

  case 144:

/* Line 1806 of yacc.c  */
#line 1058 "go.y"
    {
		(yyval.node) = (yyvsp[(2) - (4)].node);
		(yyval.node)->list = (yyvsp[(3) - (4)].list);
	}
    break;

  case 146:

/* Line 1806 of yacc.c  */
#line 1066 "go.y"
    {
		(yyval.node) = (yyvsp[(2) - (4)].node);
		(yyval.node)->list = (yyvsp[(3) - (4)].list);
	}
    break;

  case 148:

/* Line 1806 of yacc.c  */
#line 1074 "go.y"
    {
		(yyval.node) = (yyvsp[(2) - (3)].node);
		
		// Need to know on lhs of := whether there are ( ).
		// Don't bother with the OPAREN in other cases:
		// it's just a waste of memory and time.
		switch((yyval.node)->op) {
		case ONAME:
		case ONONAME:
		case OPACK:
		case OTYPE:
		case OLITERAL:
		case OTYPESW:
			(yyval.node) = nod(OPAREN, (yyval.node), N);
		}
	}
    break;

  case 152:

/* Line 1806 of yacc.c  */
#line 1100 "go.y"
    {
		(yyval.i) = LBODY;
	}
    break;

  case 153:

/* Line 1806 of yacc.c  */
#line 1104 "go.y"
    {
		(yyval.i) = '{';
	}
    break;

  case 154:

/* Line 1806 of yacc.c  */
#line 1115 "go.y"
    {
		if((yyvsp[(1) - (1)].sym) == S)
			(yyval.node) = N;
		else
			(yyval.node) = newname((yyvsp[(1) - (1)].sym));
	}
    break;

  case 155:

/* Line 1806 of yacc.c  */
#line 1124 "go.y"
    {
		(yyval.node) = dclname((yyvsp[(1) - (1)].sym));
	}
    break;

  case 156:

/* Line 1806 of yacc.c  */
#line 1129 "go.y"
    {
		(yyval.node) = N;
	}
    break;

  case 158:

/* Line 1806 of yacc.c  */
#line 1136 "go.y"
    {
		(yyval.sym) = (yyvsp[(1) - (1)].sym);
		// during imports, unqualified non-exported identifiers are from builtinpkg
		if(importpkg != nil && !exportname((yyvsp[(1) - (1)].sym)->name))
			(yyval.sym) = pkglookup((yyvsp[(1) - (1)].sym)->name, builtinpkg);
	}
    break;

  case 160:

/* Line 1806 of yacc.c  */
#line 1144 "go.y"
    {
		(yyval.sym) = S;
	}
    break;

  case 161:

/* Line 1806 of yacc.c  */
#line 1150 "go.y"
    {
		Pkg *p;

		if((yyvsp[(2) - (4)].val).u.sval->len == 0)
			p = importpkg;
		else {
			if(isbadimport((yyvsp[(2) - (4)].val).u.sval))
				errorexit();
			p = mkpkg((yyvsp[(2) - (4)].val).u.sval);
		}
		(yyval.sym) = pkglookup((yyvsp[(4) - (4)].sym)->name, p);
	}
    break;

  case 162:

/* Line 1806 of yacc.c  */
#line 1163 "go.y"
    {
		Pkg *p;

		if((yyvsp[(2) - (4)].val).u.sval->len == 0)
			p = importpkg;
		else {
			if(isbadimport((yyvsp[(2) - (4)].val).u.sval))
				errorexit();
			p = mkpkg((yyvsp[(2) - (4)].val).u.sval);
		}
		(yyval.sym) = pkglookup("?", p);
	}
    break;

  case 163:

/* Line 1806 of yacc.c  */
#line 1178 "go.y"
    {
		(yyval.node) = oldname((yyvsp[(1) - (1)].sym));
		if((yyval.node)->pack != N)
			(yyval.node)->pack->used = 1;
	}
    break;

  case 165:

/* Line 1806 of yacc.c  */
#line 1198 "go.y"
    {
		yyerror("final argument in variadic function missing type");
		(yyval.node) = nod(ODDD, typenod(typ(TINTER)), N);
	}
    break;

  case 166:

/* Line 1806 of yacc.c  */
#line 1203 "go.y"
    {
		(yyval.node) = nod(ODDD, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 172:

/* Line 1806 of yacc.c  */
#line 1214 "go.y"
    {
		(yyval.node) = nod(OTPAREN, (yyvsp[(2) - (3)].node), N);
	}
    break;

  case 176:

/* Line 1806 of yacc.c  */
#line 1223 "go.y"
    {
		(yyval.node) = nod(OIND, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 181:

/* Line 1806 of yacc.c  */
#line 1233 "go.y"
    {
		(yyval.node) = nod(OTPAREN, (yyvsp[(2) - (3)].node), N);
	}
    break;

  case 191:

/* Line 1806 of yacc.c  */
#line 1254 "go.y"
    {
		if((yyvsp[(1) - (3)].node)->op == OPACK) {
			Sym *s;
			s = restrictlookup((yyvsp[(3) - (3)].sym)->name, (yyvsp[(1) - (3)].node)->pkg);
			(yyvsp[(1) - (3)].node)->used = 1;
			(yyval.node) = oldname(s);
			break;
		}
		(yyval.node) = nod(OXDOT, (yyvsp[(1) - (3)].node), newname((yyvsp[(3) - (3)].sym)));
	}
    break;

  case 192:

/* Line 1806 of yacc.c  */
#line 1267 "go.y"
    {
		(yyval.node) = nod(OTARRAY, (yyvsp[(2) - (4)].node), (yyvsp[(4) - (4)].node));
	}
    break;

  case 193:

/* Line 1806 of yacc.c  */
#line 1271 "go.y"
    {
		// array literal of nelem
		(yyval.node) = nod(OTARRAY, nod(ODDD, N, N), (yyvsp[(4) - (4)].node));
	}
    break;

  case 194:

/* Line 1806 of yacc.c  */
#line 1276 "go.y"
    {
		(yyval.node) = nod(OTCHAN, (yyvsp[(2) - (2)].node), N);
		(yyval.node)->etype = Cboth;
	}
    break;

  case 195:

/* Line 1806 of yacc.c  */
#line 1281 "go.y"
    {
		(yyval.node) = nod(OTCHAN, (yyvsp[(3) - (3)].node), N);
		(yyval.node)->etype = Csend;
	}
    break;

  case 196:

/* Line 1806 of yacc.c  */
#line 1286 "go.y"
    {
		(yyval.node) = nod(OTMAP, (yyvsp[(3) - (5)].node), (yyvsp[(5) - (5)].node));
	}
    break;

  case 199:

/* Line 1806 of yacc.c  */
#line 1294 "go.y"
    {
		(yyval.node) = nod(OIND, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 200:

/* Line 1806 of yacc.c  */
#line 1300 "go.y"
    {
		(yyval.node) = nod(OTCHAN, (yyvsp[(3) - (3)].node), N);
		(yyval.node)->etype = Crecv;
	}
    break;

  case 201:

/* Line 1806 of yacc.c  */
#line 1307 "go.y"
    {
		(yyval.node) = nod(OTSTRUCT, N, N);
		(yyval.node)->list = (yyvsp[(3) - (5)].list);
		fixlbrace((yyvsp[(2) - (5)].i));
	}
    break;

  case 202:

/* Line 1806 of yacc.c  */
#line 1313 "go.y"
    {
		(yyval.node) = nod(OTSTRUCT, N, N);
		fixlbrace((yyvsp[(2) - (3)].i));
	}
    break;

  case 203:

/* Line 1806 of yacc.c  */
#line 1320 "go.y"
    {
		(yyval.node) = nod(OTINTER, N, N);
		(yyval.node)->list = (yyvsp[(3) - (5)].list);
		fixlbrace((yyvsp[(2) - (5)].i));
	}
    break;

  case 204:

/* Line 1806 of yacc.c  */
#line 1326 "go.y"
    {
		(yyval.node) = nod(OTINTER, N, N);
		fixlbrace((yyvsp[(2) - (3)].i));
	}
    break;

  case 205:

/* Line 1806 of yacc.c  */
#line 1337 "go.y"
    {
		(yyval.node) = (yyvsp[(2) - (3)].node);
		if((yyval.node) == N)
			break;
		if(noescape && (yyvsp[(3) - (3)].list) != nil)
			yyerror("can only use //go:noescape with external func implementations");
		(yyval.node)->nbody = (yyvsp[(3) - (3)].list);
		(yyval.node)->endlineno = lineno;
		(yyval.node)->noescape = noescape;
		funcbody((yyval.node));
	}
    break;

  case 206:

/* Line 1806 of yacc.c  */
#line 1351 "go.y"
    {
		Node *t;

		(yyval.node) = N;
		(yyvsp[(3) - (5)].list) = checkarglist((yyvsp[(3) - (5)].list), 1);

		if(strcmp((yyvsp[(1) - (5)].sym)->name, "init") == 0) {
			(yyvsp[(1) - (5)].sym) = renameinit();
			if((yyvsp[(3) - (5)].list) != nil || (yyvsp[(5) - (5)].list) != nil)
				yyerror("func init must have no arguments and no return values");
		}
		if(strcmp(localpkg->name, "main") == 0 && strcmp((yyvsp[(1) - (5)].sym)->name, "main") == 0) {
			if((yyvsp[(3) - (5)].list) != nil || (yyvsp[(5) - (5)].list) != nil)
				yyerror("func main must have no arguments and no return values");
		}

		t = nod(OTFUNC, N, N);
		t->list = (yyvsp[(3) - (5)].list);
		t->rlist = (yyvsp[(5) - (5)].list);

		(yyval.node) = nod(ODCLFUNC, N, N);
		(yyval.node)->nname = newname((yyvsp[(1) - (5)].sym));
		(yyval.node)->nname->defn = (yyval.node);
		(yyval.node)->nname->ntype = t;		// TODO: check if nname already has an ntype
		declare((yyval.node)->nname, PFUNC);

		funchdr((yyval.node));
	}
    break;

  case 207:

/* Line 1806 of yacc.c  */
#line 1380 "go.y"
    {
		Node *rcvr, *t;

		(yyval.node) = N;
		(yyvsp[(2) - (8)].list) = checkarglist((yyvsp[(2) - (8)].list), 0);
		(yyvsp[(6) - (8)].list) = checkarglist((yyvsp[(6) - (8)].list), 1);

		if((yyvsp[(2) - (8)].list) == nil) {
			yyerror("method has no receiver");
			break;
		}
		if((yyvsp[(2) - (8)].list)->next != nil) {
			yyerror("method has multiple receivers");
			break;
		}
		rcvr = (yyvsp[(2) - (8)].list)->n;
		if(rcvr->op != ODCLFIELD) {
			yyerror("bad receiver in method");
			break;
		}
		if(rcvr->right->op == OTPAREN || (rcvr->right->op == OIND && rcvr->right->left->op == OTPAREN))
			yyerror("cannot parenthesize receiver type");

		t = nod(OTFUNC, rcvr, N);
		t->list = (yyvsp[(6) - (8)].list);
		t->rlist = (yyvsp[(8) - (8)].list);

		(yyval.node) = nod(ODCLFUNC, N, N);
		(yyval.node)->shortname = newname((yyvsp[(4) - (8)].sym));
		(yyval.node)->nname = methodname1((yyval.node)->shortname, rcvr->right);
		(yyval.node)->nname->defn = (yyval.node);
		(yyval.node)->nname->ntype = t;
		(yyval.node)->nname->nointerface = nointerface;
		declare((yyval.node)->nname, PFUNC);

		funchdr((yyval.node));
	}
    break;

  case 208:

/* Line 1806 of yacc.c  */
#line 1420 "go.y"
    {
		Sym *s;
		Type *t;

		(yyval.node) = N;

		s = (yyvsp[(1) - (5)].sym);
		t = functype(N, (yyvsp[(3) - (5)].list), (yyvsp[(5) - (5)].list));

		importsym(s, ONAME);
		if(s->def != N && s->def->op == ONAME) {
			if(eqtype(t, s->def->type)) {
				dclcontext = PDISCARD;  // since we skip funchdr below
				break;
			}
			yyerror("inconsistent definition for func %S during import\n\t%T\n\t%T", s, s->def->type, t);
		}

		(yyval.node) = newname(s);
		(yyval.node)->type = t;
		declare((yyval.node), PFUNC);

		funchdr((yyval.node));
	}
    break;

  case 209:

/* Line 1806 of yacc.c  */
#line 1445 "go.y"
    {
		(yyval.node) = methodname1(newname((yyvsp[(4) - (8)].sym)), (yyvsp[(2) - (8)].list)->n->right); 
		(yyval.node)->type = functype((yyvsp[(2) - (8)].list)->n, (yyvsp[(6) - (8)].list), (yyvsp[(8) - (8)].list));

		checkwidth((yyval.node)->type);
		addmethod((yyvsp[(4) - (8)].sym), (yyval.node)->type, 0, nointerface);
		nointerface = 0;
		funchdr((yyval.node));
		
		// inl.c's inlnode in on a dotmeth node expects to find the inlineable body as
		// (dotmeth's type)->nname->inl, and dotmeth's type has been pulled
		// out by typecheck's lookdot as this $$->ttype.  So by providing
		// this back link here we avoid special casing there.
		(yyval.node)->type->nname = (yyval.node);
	}
    break;

  case 210:

/* Line 1806 of yacc.c  */
#line 1463 "go.y"
    {
		(yyvsp[(3) - (5)].list) = checkarglist((yyvsp[(3) - (5)].list), 1);
		(yyval.node) = nod(OTFUNC, N, N);
		(yyval.node)->list = (yyvsp[(3) - (5)].list);
		(yyval.node)->rlist = (yyvsp[(5) - (5)].list);
	}
    break;

  case 211:

/* Line 1806 of yacc.c  */
#line 1471 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 212:

/* Line 1806 of yacc.c  */
#line 1475 "go.y"
    {
		(yyval.list) = (yyvsp[(2) - (3)].list);
		if((yyval.list) == nil)
			(yyval.list) = list1(nod(OEMPTY, N, N));
	}
    break;

  case 213:

/* Line 1806 of yacc.c  */
#line 1483 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 214:

/* Line 1806 of yacc.c  */
#line 1487 "go.y"
    {
		(yyval.list) = list1(nod(ODCLFIELD, N, (yyvsp[(1) - (1)].node)));
	}
    break;

  case 215:

/* Line 1806 of yacc.c  */
#line 1491 "go.y"
    {
		(yyvsp[(2) - (3)].list) = checkarglist((yyvsp[(2) - (3)].list), 0);
		(yyval.list) = (yyvsp[(2) - (3)].list);
	}
    break;

  case 216:

/* Line 1806 of yacc.c  */
#line 1498 "go.y"
    {
		closurehdr((yyvsp[(1) - (1)].node));
	}
    break;

  case 217:

/* Line 1806 of yacc.c  */
#line 1504 "go.y"
    {
		(yyval.node) = closurebody((yyvsp[(3) - (4)].list));
		fixlbrace((yyvsp[(2) - (4)].i));
	}
    break;

  case 218:

/* Line 1806 of yacc.c  */
#line 1509 "go.y"
    {
		(yyval.node) = closurebody(nil);
	}
    break;

  case 219:

/* Line 1806 of yacc.c  */
#line 1520 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 220:

/* Line 1806 of yacc.c  */
#line 1524 "go.y"
    {
		(yyval.list) = concat((yyvsp[(1) - (3)].list), (yyvsp[(2) - (3)].list));
		if(nsyntaxerrors == 0)
			testdclstack();
		nointerface = 0;
		noescape = 0;
	}
    break;

  case 222:

/* Line 1806 of yacc.c  */
#line 1542 "go.y"
    {
		(yyval.list) = concat((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].list));
	}
    break;

  case 224:

/* Line 1806 of yacc.c  */
#line 1549 "go.y"
    {
		(yyval.list) = concat((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].list));
	}
    break;

  case 225:

/* Line 1806 of yacc.c  */
#line 1555 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 226:

/* Line 1806 of yacc.c  */
#line 1559 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].node));
	}
    break;

  case 228:

/* Line 1806 of yacc.c  */
#line 1566 "go.y"
    {
		(yyval.list) = concat((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].list));
	}
    break;

  case 229:

/* Line 1806 of yacc.c  */
#line 1572 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 230:

/* Line 1806 of yacc.c  */
#line 1576 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].node));
	}
    break;

  case 231:

/* Line 1806 of yacc.c  */
#line 1582 "go.y"
    {
		NodeList *l;

		Node *n;
		l = (yyvsp[(1) - (3)].list);
		if(l == nil) {
			// ? symbol, during import (list1(N) == nil)
			n = (yyvsp[(2) - (3)].node);
			if(n->op == OIND)
				n = n->left;
			n = embedded(n->sym, importpkg);
			n->right = (yyvsp[(2) - (3)].node);
			n->val = (yyvsp[(3) - (3)].val);
			(yyval.list) = list1(n);
			break;
		}

		for(l=(yyvsp[(1) - (3)].list); l; l=l->next) {
			l->n = nod(ODCLFIELD, l->n, (yyvsp[(2) - (3)].node));
			l->n->val = (yyvsp[(3) - (3)].val);
		}
	}
    break;

  case 232:

/* Line 1806 of yacc.c  */
#line 1605 "go.y"
    {
		(yyvsp[(1) - (2)].node)->val = (yyvsp[(2) - (2)].val);
		(yyval.list) = list1((yyvsp[(1) - (2)].node));
	}
    break;

  case 233:

/* Line 1806 of yacc.c  */
#line 1610 "go.y"
    {
		(yyvsp[(2) - (4)].node)->val = (yyvsp[(4) - (4)].val);
		(yyval.list) = list1((yyvsp[(2) - (4)].node));
		yyerror("cannot parenthesize embedded type");
	}
    break;

  case 234:

/* Line 1806 of yacc.c  */
#line 1616 "go.y"
    {
		(yyvsp[(2) - (3)].node)->right = nod(OIND, (yyvsp[(2) - (3)].node)->right, N);
		(yyvsp[(2) - (3)].node)->val = (yyvsp[(3) - (3)].val);
		(yyval.list) = list1((yyvsp[(2) - (3)].node));
	}
    break;

  case 235:

/* Line 1806 of yacc.c  */
#line 1622 "go.y"
    {
		(yyvsp[(3) - (5)].node)->right = nod(OIND, (yyvsp[(3) - (5)].node)->right, N);
		(yyvsp[(3) - (5)].node)->val = (yyvsp[(5) - (5)].val);
		(yyval.list) = list1((yyvsp[(3) - (5)].node));
		yyerror("cannot parenthesize embedded type");
	}
    break;

  case 236:

/* Line 1806 of yacc.c  */
#line 1629 "go.y"
    {
		(yyvsp[(3) - (5)].node)->right = nod(OIND, (yyvsp[(3) - (5)].node)->right, N);
		(yyvsp[(3) - (5)].node)->val = (yyvsp[(5) - (5)].val);
		(yyval.list) = list1((yyvsp[(3) - (5)].node));
		yyerror("cannot parenthesize embedded type");
	}
    break;

  case 237:

/* Line 1806 of yacc.c  */
#line 1638 "go.y"
    {
		Node *n;

		(yyval.sym) = (yyvsp[(1) - (1)].sym);
		n = oldname((yyvsp[(1) - (1)].sym));
		if(n->pack != N)
			n->pack->used = 1;
	}
    break;

  case 238:

/* Line 1806 of yacc.c  */
#line 1647 "go.y"
    {
		Pkg *pkg;

		if((yyvsp[(1) - (3)].sym)->def == N || (yyvsp[(1) - (3)].sym)->def->op != OPACK) {
			yyerror("%S is not a package", (yyvsp[(1) - (3)].sym));
			pkg = localpkg;
		} else {
			(yyvsp[(1) - (3)].sym)->def->used = 1;
			pkg = (yyvsp[(1) - (3)].sym)->def->pkg;
		}
		(yyval.sym) = restrictlookup((yyvsp[(3) - (3)].sym)->name, pkg);
	}
    break;

  case 239:

/* Line 1806 of yacc.c  */
#line 1662 "go.y"
    {
		(yyval.node) = embedded((yyvsp[(1) - (1)].sym), localpkg);
	}
    break;

  case 240:

/* Line 1806 of yacc.c  */
#line 1668 "go.y"
    {
		(yyval.node) = nod(ODCLFIELD, (yyvsp[(1) - (2)].node), (yyvsp[(2) - (2)].node));
		ifacedcl((yyval.node));
	}
    break;

  case 241:

/* Line 1806 of yacc.c  */
#line 1673 "go.y"
    {
		(yyval.node) = nod(ODCLFIELD, N, oldname((yyvsp[(1) - (1)].sym)));
	}
    break;

  case 242:

/* Line 1806 of yacc.c  */
#line 1677 "go.y"
    {
		(yyval.node) = nod(ODCLFIELD, N, oldname((yyvsp[(2) - (3)].sym)));
		yyerror("cannot parenthesize embedded type");
	}
    break;

  case 243:

/* Line 1806 of yacc.c  */
#line 1684 "go.y"
    {
		// without func keyword
		(yyvsp[(2) - (4)].list) = checkarglist((yyvsp[(2) - (4)].list), 1);
		(yyval.node) = nod(OTFUNC, fakethis(), N);
		(yyval.node)->list = (yyvsp[(2) - (4)].list);
		(yyval.node)->rlist = (yyvsp[(4) - (4)].list);
	}
    break;

  case 245:

/* Line 1806 of yacc.c  */
#line 1698 "go.y"
    {
		(yyval.node) = nod(ONONAME, N, N);
		(yyval.node)->sym = (yyvsp[(1) - (2)].sym);
		(yyval.node) = nod(OKEY, (yyval.node), (yyvsp[(2) - (2)].node));
	}
    break;

  case 246:

/* Line 1806 of yacc.c  */
#line 1704 "go.y"
    {
		(yyval.node) = nod(ONONAME, N, N);
		(yyval.node)->sym = (yyvsp[(1) - (2)].sym);
		(yyval.node) = nod(OKEY, (yyval.node), (yyvsp[(2) - (2)].node));
	}
    break;

  case 248:

/* Line 1806 of yacc.c  */
#line 1713 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 249:

/* Line 1806 of yacc.c  */
#line 1717 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].node));
	}
    break;

  case 250:

/* Line 1806 of yacc.c  */
#line 1722 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 251:

/* Line 1806 of yacc.c  */
#line 1726 "go.y"
    {
		(yyval.list) = (yyvsp[(1) - (2)].list);
	}
    break;

  case 252:

/* Line 1806 of yacc.c  */
#line 1734 "go.y"
    {
		(yyval.node) = N;
	}
    break;

  case 254:

/* Line 1806 of yacc.c  */
#line 1739 "go.y"
    {
		(yyval.node) = liststmt((yyvsp[(1) - (1)].list));
	}
    break;

  case 256:

/* Line 1806 of yacc.c  */
#line 1744 "go.y"
    {
		(yyval.node) = N;
	}
    break;

  case 262:

/* Line 1806 of yacc.c  */
#line 1755 "go.y"
    {
		(yyvsp[(1) - (2)].node) = nod(OLABEL, (yyvsp[(1) - (2)].node), N);
		(yyvsp[(1) - (2)].node)->sym = dclstack;  // context, for goto restrictions
	}
    break;

  case 263:

/* Line 1806 of yacc.c  */
#line 1760 "go.y"
    {
		NodeList *l;

		(yyvsp[(1) - (4)].node)->defn = (yyvsp[(4) - (4)].node);
		l = list1((yyvsp[(1) - (4)].node));
		if((yyvsp[(4) - (4)].node))
			l = list(l, (yyvsp[(4) - (4)].node));
		(yyval.node) = liststmt(l);
	}
    break;

  case 264:

/* Line 1806 of yacc.c  */
#line 1770 "go.y"
    {
		// will be converted to OFALL
		(yyval.node) = nod(OXFALL, N, N);
	}
    break;

  case 265:

/* Line 1806 of yacc.c  */
#line 1775 "go.y"
    {
		(yyval.node) = nod(OBREAK, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 266:

/* Line 1806 of yacc.c  */
#line 1779 "go.y"
    {
		(yyval.node) = nod(OCONTINUE, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 267:

/* Line 1806 of yacc.c  */
#line 1783 "go.y"
    {
		(yyval.node) = nod(OPROC, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 268:

/* Line 1806 of yacc.c  */
#line 1787 "go.y"
    {
		(yyval.node) = nod(ODEFER, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 269:

/* Line 1806 of yacc.c  */
#line 1791 "go.y"
    {
		(yyval.node) = nod(OGOTO, (yyvsp[(2) - (2)].node), N);
		(yyval.node)->sym = dclstack;  // context, for goto restrictions
	}
    break;

  case 270:

/* Line 1806 of yacc.c  */
#line 1796 "go.y"
    {
		(yyval.node) = nod(ORETURN, N, N);
		(yyval.node)->list = (yyvsp[(2) - (2)].list);
		if((yyval.node)->list == nil && curfn != N) {
			NodeList *l;

			for(l=curfn->dcl; l; l=l->next) {
				if(l->n->class == PPARAM)
					continue;
				if(l->n->class != PPARAMOUT)
					break;
				if(l->n->sym->def != l->n)
					yyerror("%s is shadowed during return", l->n->sym->name);
			}
		}
	}
    break;

  case 271:

/* Line 1806 of yacc.c  */
#line 1815 "go.y"
    {
		(yyval.list) = nil;
		if((yyvsp[(1) - (1)].node) != N)
			(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 272:

/* Line 1806 of yacc.c  */
#line 1821 "go.y"
    {
		(yyval.list) = (yyvsp[(1) - (3)].list);
		if((yyvsp[(3) - (3)].node) != N)
			(yyval.list) = list((yyval.list), (yyvsp[(3) - (3)].node));
	}
    break;

  case 273:

/* Line 1806 of yacc.c  */
#line 1829 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 274:

/* Line 1806 of yacc.c  */
#line 1833 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].node));
	}
    break;

  case 275:

/* Line 1806 of yacc.c  */
#line 1839 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 276:

/* Line 1806 of yacc.c  */
#line 1843 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].node));
	}
    break;

  case 277:

/* Line 1806 of yacc.c  */
#line 1849 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 278:

/* Line 1806 of yacc.c  */
#line 1853 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].node));
	}
    break;

  case 279:

/* Line 1806 of yacc.c  */
#line 1859 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 280:

/* Line 1806 of yacc.c  */
#line 1863 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].node));
	}
    break;

  case 281:

/* Line 1806 of yacc.c  */
#line 1872 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 282:

/* Line 1806 of yacc.c  */
#line 1876 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 283:

/* Line 1806 of yacc.c  */
#line 1880 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].node));
	}
    break;

  case 284:

/* Line 1806 of yacc.c  */
#line 1884 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].node));
	}
    break;

  case 285:

/* Line 1806 of yacc.c  */
#line 1889 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 286:

/* Line 1806 of yacc.c  */
#line 1893 "go.y"
    {
		(yyval.list) = (yyvsp[(1) - (2)].list);
	}
    break;

  case 291:

/* Line 1806 of yacc.c  */
#line 1907 "go.y"
    {
		(yyval.node) = N;
	}
    break;

  case 293:

/* Line 1806 of yacc.c  */
#line 1913 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 295:

/* Line 1806 of yacc.c  */
#line 1919 "go.y"
    {
		(yyval.node) = N;
	}
    break;

  case 297:

/* Line 1806 of yacc.c  */
#line 1925 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 299:

/* Line 1806 of yacc.c  */
#line 1931 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 301:

/* Line 1806 of yacc.c  */
#line 1937 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 303:

/* Line 1806 of yacc.c  */
#line 1943 "go.y"
    {
		(yyval.val).ctype = CTxxx;
	}
    break;

  case 305:

/* Line 1806 of yacc.c  */
#line 1953 "go.y"
    {
		importimport((yyvsp[(2) - (4)].sym), (yyvsp[(3) - (4)].val).u.sval);
	}
    break;

  case 306:

/* Line 1806 of yacc.c  */
#line 1957 "go.y"
    {
		importvar((yyvsp[(2) - (4)].sym), (yyvsp[(3) - (4)].type));
	}
    break;

  case 307:

/* Line 1806 of yacc.c  */
#line 1961 "go.y"
    {
		importconst((yyvsp[(2) - (5)].sym), types[TIDEAL], (yyvsp[(4) - (5)].node));
	}
    break;

  case 308:

/* Line 1806 of yacc.c  */
#line 1965 "go.y"
    {
		importconst((yyvsp[(2) - (6)].sym), (yyvsp[(3) - (6)].type), (yyvsp[(5) - (6)].node));
	}
    break;

  case 309:

/* Line 1806 of yacc.c  */
#line 1969 "go.y"
    {
		importtype((yyvsp[(2) - (4)].type), (yyvsp[(3) - (4)].type));
	}
    break;

  case 310:

/* Line 1806 of yacc.c  */
#line 1973 "go.y"
    {
		if((yyvsp[(2) - (4)].node) == N) {
			dclcontext = PEXTERN;  // since we skip the funcbody below
			break;
		}

		(yyvsp[(2) - (4)].node)->inl = (yyvsp[(3) - (4)].list);

		funcbody((yyvsp[(2) - (4)].node));
		importlist = list(importlist, (yyvsp[(2) - (4)].node));

		if(debug['E']) {
			print("import [%Z] func %lN \n", importpkg->path, (yyvsp[(2) - (4)].node));
			if(debug['m'] > 2 && (yyvsp[(2) - (4)].node)->inl)
				print("inl body:%+H\n", (yyvsp[(2) - (4)].node)->inl);
		}
	}
    break;

  case 311:

/* Line 1806 of yacc.c  */
#line 1993 "go.y"
    {
		(yyval.sym) = (yyvsp[(1) - (1)].sym);
		structpkg = (yyval.sym)->pkg;
	}
    break;

  case 312:

/* Line 1806 of yacc.c  */
#line 2000 "go.y"
    {
		(yyval.type) = pkgtype((yyvsp[(1) - (1)].sym));
		importsym((yyvsp[(1) - (1)].sym), OTYPE);
	}
    break;

  case 318:

/* Line 1806 of yacc.c  */
#line 2020 "go.y"
    {
		(yyval.type) = pkgtype((yyvsp[(1) - (1)].sym));
	}
    break;

  case 319:

/* Line 1806 of yacc.c  */
#line 2024 "go.y"
    {
		// predefined name like uint8
		(yyvsp[(1) - (1)].sym) = pkglookup((yyvsp[(1) - (1)].sym)->name, builtinpkg);
		if((yyvsp[(1) - (1)].sym)->def == N || (yyvsp[(1) - (1)].sym)->def->op != OTYPE) {
			yyerror("%s is not a type", (yyvsp[(1) - (1)].sym)->name);
			(yyval.type) = T;
		} else
			(yyval.type) = (yyvsp[(1) - (1)].sym)->def->type;
	}
    break;

  case 320:

/* Line 1806 of yacc.c  */
#line 2034 "go.y"
    {
		(yyval.type) = aindex(N, (yyvsp[(3) - (3)].type));
	}
    break;

  case 321:

/* Line 1806 of yacc.c  */
#line 2038 "go.y"
    {
		(yyval.type) = aindex(nodlit((yyvsp[(2) - (4)].val)), (yyvsp[(4) - (4)].type));
	}
    break;

  case 322:

/* Line 1806 of yacc.c  */
#line 2042 "go.y"
    {
		(yyval.type) = maptype((yyvsp[(3) - (5)].type), (yyvsp[(5) - (5)].type));
	}
    break;

  case 323:

/* Line 1806 of yacc.c  */
#line 2046 "go.y"
    {
		(yyval.type) = tostruct((yyvsp[(3) - (4)].list));
	}
    break;

  case 324:

/* Line 1806 of yacc.c  */
#line 2050 "go.y"
    {
		(yyval.type) = tointerface((yyvsp[(3) - (4)].list));
	}
    break;

  case 325:

/* Line 1806 of yacc.c  */
#line 2054 "go.y"
    {
		(yyval.type) = ptrto((yyvsp[(2) - (2)].type));
	}
    break;

  case 326:

/* Line 1806 of yacc.c  */
#line 2058 "go.y"
    {
		(yyval.type) = typ(TCHAN);
		(yyval.type)->type = (yyvsp[(2) - (2)].type);
		(yyval.type)->chan = Cboth;
	}
    break;

  case 327:

/* Line 1806 of yacc.c  */
#line 2064 "go.y"
    {
		(yyval.type) = typ(TCHAN);
		(yyval.type)->type = (yyvsp[(3) - (4)].type);
		(yyval.type)->chan = Cboth;
	}
    break;

  case 328:

/* Line 1806 of yacc.c  */
#line 2070 "go.y"
    {
		(yyval.type) = typ(TCHAN);
		(yyval.type)->type = (yyvsp[(3) - (3)].type);
		(yyval.type)->chan = Csend;
	}
    break;

  case 329:

/* Line 1806 of yacc.c  */
#line 2078 "go.y"
    {
		(yyval.type) = typ(TCHAN);
		(yyval.type)->type = (yyvsp[(3) - (3)].type);
		(yyval.type)->chan = Crecv;
	}
    break;

  case 330:

/* Line 1806 of yacc.c  */
#line 2086 "go.y"
    {
		(yyval.type) = functype(nil, (yyvsp[(3) - (5)].list), (yyvsp[(5) - (5)].list));
	}
    break;

  case 331:

/* Line 1806 of yacc.c  */
#line 2092 "go.y"
    {
		(yyval.node) = nod(ODCLFIELD, N, typenod((yyvsp[(2) - (3)].type)));
		if((yyvsp[(1) - (3)].sym))
			(yyval.node)->left = newname((yyvsp[(1) - (3)].sym));
		(yyval.node)->val = (yyvsp[(3) - (3)].val);
	}
    break;

  case 332:

/* Line 1806 of yacc.c  */
#line 2099 "go.y"
    {
		Type *t;
	
		t = typ(TARRAY);
		t->bound = -1;
		t->type = (yyvsp[(3) - (4)].type);

		(yyval.node) = nod(ODCLFIELD, N, typenod(t));
		if((yyvsp[(1) - (4)].sym))
			(yyval.node)->left = newname((yyvsp[(1) - (4)].sym));
		(yyval.node)->isddd = 1;
		(yyval.node)->val = (yyvsp[(4) - (4)].val);
	}
    break;

  case 333:

/* Line 1806 of yacc.c  */
#line 2115 "go.y"
    {
		Sym *s;
		Pkg *p;

		if((yyvsp[(1) - (3)].sym) != S && strcmp((yyvsp[(1) - (3)].sym)->name, "?") != 0) {
			(yyval.node) = nod(ODCLFIELD, newname((yyvsp[(1) - (3)].sym)), typenod((yyvsp[(2) - (3)].type)));
			(yyval.node)->val = (yyvsp[(3) - (3)].val);
		} else {
			s = (yyvsp[(2) - (3)].type)->sym;
			if(s == S && isptr[(yyvsp[(2) - (3)].type)->etype])
				s = (yyvsp[(2) - (3)].type)->type->sym;
			p = importpkg;
			if((yyvsp[(1) - (3)].sym) != S)
				p = (yyvsp[(1) - (3)].sym)->pkg;
			(yyval.node) = embedded(s, p);
			(yyval.node)->right = typenod((yyvsp[(2) - (3)].type));
			(yyval.node)->val = (yyvsp[(3) - (3)].val);
		}
	}
    break;

  case 334:

/* Line 1806 of yacc.c  */
#line 2137 "go.y"
    {
		(yyval.node) = nod(ODCLFIELD, newname((yyvsp[(1) - (5)].sym)), typenod(functype(fakethis(), (yyvsp[(3) - (5)].list), (yyvsp[(5) - (5)].list))));
	}
    break;

  case 335:

/* Line 1806 of yacc.c  */
#line 2141 "go.y"
    {
		(yyval.node) = nod(ODCLFIELD, N, typenod((yyvsp[(1) - (1)].type)));
	}
    break;

  case 336:

/* Line 1806 of yacc.c  */
#line 2146 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 338:

/* Line 1806 of yacc.c  */
#line 2153 "go.y"
    {
		(yyval.list) = (yyvsp[(2) - (3)].list);
	}
    break;

  case 339:

/* Line 1806 of yacc.c  */
#line 2157 "go.y"
    {
		(yyval.list) = list1(nod(ODCLFIELD, N, typenod((yyvsp[(1) - (1)].type))));
	}
    break;

  case 340:

/* Line 1806 of yacc.c  */
#line 2167 "go.y"
    {
		(yyval.node) = nodlit((yyvsp[(1) - (1)].val));
	}
    break;

  case 341:

/* Line 1806 of yacc.c  */
#line 2171 "go.y"
    {
		(yyval.node) = nodlit((yyvsp[(2) - (2)].val));
		switch((yyval.node)->val.ctype){
		case CTINT:
		case CTRUNE:
			mpnegfix((yyval.node)->val.u.xval);
			break;
		case CTFLT:
			mpnegflt((yyval.node)->val.u.fval);
			break;
		default:
			yyerror("bad negated constant");
		}
	}
    break;

  case 342:

/* Line 1806 of yacc.c  */
#line 2186 "go.y"
    {
		(yyval.node) = oldname(pkglookup((yyvsp[(1) - (1)].sym)->name, builtinpkg));
		if((yyval.node)->op != OLITERAL)
			yyerror("bad constant %S", (yyval.node)->sym);
	}
    break;

  case 344:

/* Line 1806 of yacc.c  */
#line 2195 "go.y"
    {
		if((yyvsp[(2) - (5)].node)->val.ctype == CTRUNE && (yyvsp[(4) - (5)].node)->val.ctype == CTINT) {
			(yyval.node) = (yyvsp[(2) - (5)].node);
			mpaddfixfix((yyvsp[(2) - (5)].node)->val.u.xval, (yyvsp[(4) - (5)].node)->val.u.xval, 0);
			break;
		}
		(yyvsp[(4) - (5)].node)->val.u.cval->real = (yyvsp[(4) - (5)].node)->val.u.cval->imag;
		mpmovecflt(&(yyvsp[(4) - (5)].node)->val.u.cval->imag, 0.0);
		(yyval.node) = nodcplxlit((yyvsp[(2) - (5)].node)->val, (yyvsp[(4) - (5)].node)->val);
	}
    break;

  case 347:

/* Line 1806 of yacc.c  */
#line 2211 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 348:

/* Line 1806 of yacc.c  */
#line 2215 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].node));
	}
    break;

  case 349:

/* Line 1806 of yacc.c  */
#line 2221 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 350:

/* Line 1806 of yacc.c  */
#line 2225 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].node));
	}
    break;

  case 351:

/* Line 1806 of yacc.c  */
#line 2231 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 352:

/* Line 1806 of yacc.c  */
#line 2235 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].node));
	}
    break;



/* Line 1806 of yacc.c  */
#line 5455 "y.tab.c"
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 2067 of yacc.c  */
#line 2239 "go.y"


static void
fixlbrace(int lbr)
{
	// If the opening brace was an LBODY,
	// set up for another one now that we're done.
	// See comment in lex.c about loophack.
	if(lbr == LBODY)
		loophack = 1;
}


