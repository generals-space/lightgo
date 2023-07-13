/* A Bison parser, made by GNU Bison 2.5.  */

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
#define YYLAST   2358

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  76
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  142
/* YYNRULES -- Number of rules.  */
#define YYNRULES  353
/* YYNRULES -- Number of states.  */
#define YYNSTATES  671

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
     231,   236,   239,   245,   247,   249,   252,   253,   257,   259,
     263,   264,   265,   266,   275,   276,   282,   283,   286,   287,
     290,   291,   292,   300,   301,   307,   309,   313,   317,   321,
     325,   329,   333,   337,   341,   345,   349,   353,   357,   361,
     365,   369,   373,   377,   381,   385,   389,   391,   394,   397,
     400,   403,   406,   409,   412,   415,   419,   425,   432,   434,
     436,   440,   446,   452,   457,   464,   473,   475,   481,   487,
     493,   501,   503,   504,   508,   510,   515,   517,   522,   524,
     528,   530,   532,   534,   536,   538,   540,   542,   543,   545,
     547,   549,   551,   556,   561,   563,   565,   567,   570,   572,
     574,   576,   578,   580,   584,   586,   588,   590,   593,   595,
     597,   599,   601,   605,   607,   609,   611,   613,   615,   617,
     619,   621,   623,   627,   632,   637,   640,   644,   650,   652,
     654,   657,   661,   667,   671,   677,   681,   685,   691,   700,
     706,   715,   721,   722,   726,   727,   729,   733,   735,   740,
     743,   744,   748,   750,   754,   756,   760,   762,   766,   768,
     772,   774,   778,   782,   785,   790,   794,   800,   806,   808,
     812,   814,   817,   819,   823,   828,   830,   833,   836,   838,
     840,   844,   845,   848,   849,   851,   853,   855,   857,   859,
     861,   863,   865,   867,   868,   873,   875,   878,   881,   884,
     887,   890,   893,   895,   899,   901,   905,   907,   911,   913,
     917,   919,   923,   925,   927,   931,   935,   936,   939,   940,
     942,   943,   945,   946,   948,   949,   951,   952,   954,   955,
     957,   958,   960,   961,   963,   964,   966,   971,   976,   982,
     989,   994,   999,  1001,  1003,  1005,  1007,  1009,  1011,  1013,
    1015,  1017,  1021,  1026,  1032,  1037,  1042,  1045,  1048,  1053,
    1057,  1061,  1067,  1071,  1076,  1080,  1086,  1088,  1089,  1091,
    1095,  1097,  1099,  1102,  1104,  1106,  1112,  1113,  1116,  1118,
    1122,  1124,  1128,  1130
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
      -1,   186,     5,    26,   126,    -1,    26,   126,    -1,   194,
      62,   194,    62,   194,    -1,   194,    -1,   107,    -1,   108,
     105,    -1,    -1,    16,   111,   109,    -1,   194,    -1,   194,
      62,   194,    -1,    -1,    -1,    -1,    20,   114,   112,   115,
     105,   116,   119,   120,    -1,    -1,    14,    20,   118,   112,
     105,    -1,    -1,   119,   117,    -1,    -1,    14,   100,    -1,
      -1,    -1,    30,   122,   112,   123,    35,   104,    68,    -1,
      -1,    28,   125,    35,   104,    68,    -1,   127,    -1,   126,
      47,   126,    -1,   126,    33,   126,    -1,   126,    38,   126,
      -1,   126,    46,   126,    -1,   126,    45,   126,    -1,   126,
      43,   126,    -1,   126,    39,   126,    -1,   126,    40,   126,
      -1,   126,    49,   126,    -1,   126,    50,   126,    -1,   126,
      51,   126,    -1,   126,    52,   126,    -1,   126,    53,   126,
      -1,   126,    54,   126,    -1,   126,    55,   126,    -1,   126,
      56,   126,    -1,   126,    34,   126,    -1,   126,    44,   126,
      -1,   126,    48,   126,    -1,   126,    36,   126,    -1,   134,
      -1,    53,   127,    -1,    56,   127,    -1,    49,   127,    -1,
      50,   127,    -1,    69,   127,    -1,    70,   127,    -1,    52,
     127,    -1,    36,   127,    -1,   134,    59,    60,    -1,   134,
      59,   187,   191,    60,    -1,   134,    59,   187,    11,   191,
      60,    -1,     3,    -1,   143,    -1,   134,    63,   141,    -1,
     134,    63,    59,   135,    60,    -1,   134,    63,    59,    31,
      60,    -1,   134,    71,   126,    72,    -1,   134,    71,   192,
      66,   192,    72,    -1,   134,    71,   192,    66,   192,    66,
     192,    72,    -1,   128,    -1,   149,    59,   126,   191,    60,
      -1,   150,   137,   130,   189,    68,    -1,   129,    67,   130,
     189,    68,    -1,    59,   135,    60,    67,   130,   189,    68,
      -1,   165,    -1,    -1,   126,    66,   133,    -1,   126,    -1,
      67,   130,   189,    68,    -1,   126,    -1,    67,   130,   189,
      68,    -1,   129,    -1,    59,   135,    60,    -1,   126,    -1,
     147,    -1,   146,    -1,    35,    -1,    67,    -1,   141,    -1,
     141,    -1,    -1,   138,    -1,    24,    -1,   142,    -1,    73,
      -1,    74,     3,    63,    24,    -1,    74,     3,    63,    73,
      -1,   141,    -1,   138,    -1,    11,    -1,    11,   146,    -1,
     155,    -1,   161,    -1,   153,    -1,   154,    -1,   152,    -1,
      59,   146,    60,    -1,   155,    -1,   161,    -1,   153,    -1,
      53,   147,    -1,   161,    -1,   153,    -1,   154,    -1,   152,
      -1,    59,   146,    60,    -1,   161,    -1,   153,    -1,   153,
      -1,   155,    -1,   161,    -1,   153,    -1,   154,    -1,   152,
      -1,   143,    -1,   143,    63,   141,    -1,    71,   192,    72,
     146,    -1,    71,    11,    72,   146,    -1,     8,   148,    -1,
       8,    36,   146,    -1,    23,    71,   146,    72,   146,    -1,
     156,    -1,   157,    -1,    53,   146,    -1,    36,     8,   146,
      -1,    29,   137,   170,   190,    68,    -1,    29,   137,    68,
      -1,    22,   137,   171,   190,    68,    -1,    22,   137,    68,
      -1,    17,   159,   162,    -1,   141,    59,   179,    60,   163,
      -1,    59,   179,    60,   141,    59,   179,    60,   163,    -1,
     200,    59,   195,    60,   210,    -1,    59,   215,    60,   141,
      59,   195,    60,   210,    -1,    17,    59,   179,    60,   163,
      -1,    -1,    67,   183,    68,    -1,    -1,   151,    -1,    59,
     179,    60,    -1,   161,    -1,   164,   137,   183,    68,    -1,
     164,     1,    -1,    -1,   166,    90,    62,    -1,    93,    -1,
     167,    62,    93,    -1,    95,    -1,   168,    62,    95,    -1,
      97,    -1,   169,    62,    97,    -1,   172,    -1,   170,    62,
     172,    -1,   175,    -1,   171,    62,   175,    -1,   184,   146,
     198,    -1,   174,   198,    -1,    59,   174,    60,   198,    -1,
      53,   174,   198,    -1,    59,    53,   174,    60,   198,    -1,
      53,    59,   174,    60,   198,    -1,    24,    -1,    24,    63,
     141,    -1,   173,    -1,   138,   176,    -1,   173,    -1,    59,
     173,    60,    -1,    59,   179,    60,   163,    -1,   136,    -1,
     141,   136,    -1,   141,   145,    -1,   145,    -1,   177,    -1,
     178,    75,   177,    -1,    -1,   178,   191,    -1,    -1,   100,
      -1,    91,    -1,   181,    -1,     1,    -1,    98,    -1,   110,
      -1,   121,    -1,   124,    -1,   113,    -1,    -1,   144,    66,
     182,   180,    -1,    15,    -1,     6,   140,    -1,    10,   140,
      -1,    18,   128,    -1,    13,   128,    -1,    19,   138,    -1,
      27,   193,    -1,   180,    -1,   183,    62,   180,    -1,   138,
      -1,   184,    75,   138,    -1,   139,    -1,   185,    75,   139,
      -1,   126,    -1,   186,    75,   126,    -1,   135,    -1,   187,
      75,   135,    -1,   131,    -1,   132,    -1,   188,    75,   131,
      -1,   188,    75,   132,    -1,    -1,   188,   191,    -1,    -1,
      62,    -1,    -1,    75,    -1,    -1,   126,    -1,    -1,   186,
      -1,    -1,    98,    -1,    -1,   215,    -1,    -1,   216,    -1,
      -1,   217,    -1,    -1,     3,    -1,    21,    24,     3,    62,
      -1,    32,   200,   202,    62,    -1,     9,   200,    65,   213,
      62,    -1,     9,   200,   202,    65,   213,    62,    -1,    31,
     201,   202,    62,    -1,    17,   160,   162,    62,    -1,   142,
      -1,   200,    -1,   204,    -1,   205,    -1,   206,    -1,   204,
      -1,   206,    -1,   142,    -1,    24,    -1,    71,    72,   202,
      -1,    71,     3,    72,   202,    -1,    23,    71,   202,    72,
     202,    -1,    29,    67,   196,    68,    -1,    22,    67,   197,
      68,    -1,    53,   202,    -1,     8,   203,    -1,     8,    59,
     205,    60,    -1,     8,    36,   202,    -1,    36,     8,   202,
      -1,    17,    59,   195,    60,   210,    -1,   141,   202,   198,
      -1,   141,    11,   202,   198,    -1,   141,   202,   198,    -1,
     141,    59,   195,    60,   210,    -1,   202,    -1,    -1,   211,
      -1,    59,   195,    60,    -1,   202,    -1,     3,    -1,    50,
       3,    -1,   141,    -1,   212,    -1,    59,   212,    49,   212,
      60,    -1,    -1,   214,   199,    -1,   207,    -1,   215,    75,
     207,    -1,   208,    -1,   216,    62,   208,    -1,   209,    -1,
     217,    62,   209,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   128,   128,   137,   143,   154,   154,   169,   170,   173,
     174,   175,   178,   215,   226,   227,   230,   237,   244,   253,
     267,   268,   275,   275,   288,   292,   293,   297,   302,   313,
     321,   327,   331,   337,   343,   349,   355,   363,   367,   373,
     382,   387,   392,   398,   402,   408,   409,   413,   419,   431,
     436,   442,   461,   467,   480,   497,   503,   510,   530,   548,
     557,   576,   575,   590,   589,   620,   623,   630,   629,   641,
     649,   657,   669,   681,   688,   691,   700,   699,   710,   716,
     728,   732,   737,   727,   758,   757,   770,   773,   779,   782,
     794,   798,   793,   816,   815,   831,   832,   836,   840,   844,
     848,   852,   856,   860,   864,   868,   872,   876,   880,   884,
     888,   892,   896,   900,   904,   909,   915,   916,   920,   931,
     935,   939,   943,   948,   952,   962,   966,   971,   979,   983,
     984,   995,   999,  1003,  1007,  1011,  1019,  1020,  1026,  1033,
    1039,  1046,  1049,  1056,  1062,  1079,  1086,  1087,  1094,  1095,
    1114,  1115,  1118,  1121,  1125,  1136,  1145,  1151,  1154,  1157,
    1164,  1165,  1171,  1184,  1199,  1207,  1219,  1224,  1230,  1231,
    1232,  1233,  1234,  1235,  1241,  1242,  1243,  1244,  1250,  1251,
    1252,  1253,  1254,  1260,  1261,  1264,  1267,  1268,  1269,  1270,
    1271,  1274,  1275,  1288,  1292,  1297,  1302,  1307,  1311,  1312,
    1315,  1321,  1328,  1334,  1341,  1347,  1358,  1372,  1401,  1441,
    1466,  1484,  1493,  1496,  1504,  1508,  1512,  1519,  1525,  1530,
    1542,  1545,  1561,  1563,  1569,  1570,  1576,  1580,  1586,  1587,
    1593,  1597,  1603,  1626,  1631,  1637,  1643,  1650,  1659,  1668,
    1683,  1689,  1694,  1698,  1705,  1718,  1719,  1725,  1731,  1734,
    1738,  1744,  1747,  1756,  1759,  1760,  1764,  1765,  1771,  1772,
    1773,  1774,  1775,  1777,  1776,  1791,  1796,  1800,  1804,  1808,
    1812,  1817,  1836,  1842,  1850,  1854,  1860,  1864,  1870,  1874,
    1880,  1884,  1893,  1897,  1901,  1905,  1911,  1914,  1922,  1923,
    1925,  1926,  1929,  1932,  1935,  1938,  1941,  1944,  1947,  1950,
    1953,  1956,  1959,  1962,  1965,  1968,  1974,  1978,  1982,  1986,
    1990,  1994,  2014,  2021,  2032,  2033,  2034,  2037,  2038,  2041,
    2045,  2055,  2059,  2063,  2067,  2071,  2075,  2079,  2085,  2091,
    2099,  2107,  2113,  2120,  2136,  2158,  2162,  2168,  2171,  2174,
    2178,  2188,  2192,  2207,  2215,  2216,  2228,  2229,  2232,  2236,
    2242,  2246,  2252,  2256
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
     107,   107,   108,   108,   108,   109,   111,   110,   112,   112,
     114,   115,   116,   113,   118,   117,   119,   119,   120,   120,
     122,   123,   121,   125,   124,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   127,   127,   127,   127,
     127,   127,   127,   127,   127,   128,   128,   128,   129,   129,
     129,   129,   129,   129,   129,   129,   129,   129,   129,   129,
     129,   129,   130,   131,   132,   132,   133,   133,   134,   134,
     135,   135,   136,   137,   137,   138,   139,   140,   140,   141,
     141,   141,   142,   142,   143,   144,   145,   145,   146,   146,
     146,   146,   146,   146,   147,   147,   147,   147,   148,   148,
     148,   148,   148,   149,   149,   150,   151,   151,   151,   151,
     151,   152,   152,   153,   153,   153,   153,   153,   153,   153,
     154,   155,   156,   156,   157,   157,   158,   159,   159,   160,
     160,   161,   162,   162,   163,   163,   163,   164,   165,   165,
     166,   166,   167,   167,   168,   168,   169,   169,   170,   170,
     171,   171,   172,   172,   172,   172,   172,   172,   173,   173,
     174,   175,   175,   175,   176,   177,   177,   177,   177,   178,
     178,   179,   179,   180,   180,   180,   180,   180,   181,   181,
     181,   181,   181,   182,   181,   181,   181,   181,   181,   181,
     181,   181,   183,   183,   184,   184,   185,   185,   186,   186,
     187,   187,   188,   188,   188,   188,   189,   189,   190,   190,
     191,   191,   192,   192,   193,   193,   194,   194,   195,   195,
     196,   196,   197,   197,   198,   198,   199,   199,   199,   199,
     199,   199,   200,   201,   202,   202,   202,   203,   203,   204,
     204,   204,   204,   204,   204,   204,   204,   204,   204,   204,
     205,   206,   207,   207,   208,   209,   209,   210,   210,   211,
     211,   212,   212,   212,   213,   213,   214,   214,   215,   215,
     216,   216,   217,   217
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
       4,     2,     5,     1,     1,     2,     0,     3,     1,     3,
       0,     0,     0,     8,     0,     5,     0,     2,     0,     2,
       0,     0,     7,     0,     5,     1,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     1,     2,     2,     2,
       2,     2,     2,     2,     2,     3,     5,     6,     1,     1,
       3,     5,     5,     4,     6,     8,     1,     5,     5,     5,
       7,     1,     0,     3,     1,     4,     1,     4,     1,     3,
       1,     1,     1,     1,     1,     1,     1,     0,     1,     1,
       1,     1,     4,     4,     1,     1,     1,     2,     1,     1,
       1,     1,     1,     3,     1,     1,     1,     2,     1,     1,
       1,     1,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     3,     4,     4,     2,     3,     5,     1,     1,
       2,     3,     5,     3,     5,     3,     3,     5,     8,     5,
       8,     5,     0,     3,     0,     1,     3,     1,     4,     2,
       0,     3,     1,     3,     1,     3,     1,     3,     1,     3,
       1,     3,     3,     2,     4,     3,     5,     5,     1,     3,
       1,     2,     1,     3,     4,     1,     2,     2,     1,     1,
       3,     0,     2,     0,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     0,     4,     1,     2,     2,     2,     2,
       2,     2,     1,     3,     1,     3,     1,     3,     1,     3,
       1,     3,     1,     1,     3,     3,     0,     2,     0,     1,
       0,     1,     0,     1,     0,     1,     0,     1,     0,     1,
       0,     1,     0,     1,     0,     1,     4,     4,     5,     6,
       4,     4,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     3,     4,     5,     4,     4,     2,     2,     4,     3,
       3,     5,     3,     4,     3,     5,     1,     0,     1,     3,
       1,     1,     2,     1,     1,     5,     0,     2,     1,     3,
       1,     3,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       5,     0,     3,     0,     1,     0,     7,     0,    22,   159,
     161,     0,     0,   160,   220,    20,     6,   346,     0,     4,
       0,     0,     0,    21,     0,     0,     0,    16,     0,     0,
       9,    22,     0,     8,    28,   128,   157,     0,    39,   157,
       0,   265,    76,     0,     0,     0,    80,     0,     0,   294,
      93,     0,    90,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   292,     0,    25,     0,   258,   259,
     262,   260,   261,    51,    95,   136,   148,   116,   165,   164,
     129,     0,     0,     0,   185,   198,   199,    26,   217,     0,
     141,    27,     0,    19,     0,     0,     0,     0,     0,     0,
     347,   162,   163,    11,    14,   288,    18,    22,    13,    17,
     158,   266,   155,     0,     0,     0,     0,   164,   191,   195,
     181,   179,   180,   178,   267,   136,     0,   296,   251,     0,
     212,   136,   270,   296,   153,   154,     0,     0,   278,   295,
     271,     0,     0,   296,     0,     0,    36,    48,     0,    29,
     276,   156,     0,   124,   119,   120,   123,   117,   118,     0,
       0,   150,     0,   151,   176,   174,   175,   121,   122,     0,
     293,     0,   221,     0,    32,     0,     0,     0,     0,     0,
      56,     0,     0,     0,    55,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   142,
       0,     0,   292,   263,     0,   142,   219,     0,     0,     0,
       0,   312,     0,     0,   212,     0,     0,   313,     0,     0,
      23,   289,     0,    12,   251,     0,     0,   196,   172,   170,
     171,   168,   169,   200,     0,     0,     0,   297,    74,     0,
      77,     0,    73,   166,   245,   164,   248,   152,   249,   290,
       0,   251,     0,   206,    81,    78,   159,     0,   205,     0,
     288,   242,   230,     0,    65,     0,     0,   203,   274,   288,
     228,   240,   304,     0,    91,    38,   226,   288,     0,    49,
      31,   222,   288,     0,     0,    40,     0,   177,   149,     0,
       0,    35,   288,     0,     0,    52,    97,   112,   115,    98,
     102,   103,   101,   113,   100,    99,    96,   114,   104,   105,
     106,   107,   108,   109,   110,   111,   286,   125,   280,   290,
       0,   130,   293,     0,     0,   290,   286,   257,    61,   255,
     254,   272,   256,     0,    54,    53,   279,     0,     0,     0,
       0,   320,     0,     0,     0,     0,     0,   319,     0,   314,
     315,   316,     0,   348,     0,     0,   298,     0,     0,     0,
      15,    10,     0,     0,     0,   182,   192,    71,    67,    75,
       0,     0,   296,   167,   246,   247,   291,   252,   214,     0,
       0,     0,   296,     0,   238,     0,   251,   241,   289,     0,
       0,     0,     0,   304,     0,     0,   289,     0,   305,   233,
       0,   304,     0,   289,     0,    50,   289,     0,    42,   277,
       0,     0,     0,   201,   172,   170,   171,   169,   142,   194,
     193,   289,     0,    44,     0,   142,   144,   282,   283,   290,
       0,   290,   291,     0,     0,     0,   133,   292,   264,   291,
       0,     0,     0,     0,   218,     0,     0,   327,   317,   318,
     298,   302,     0,   300,     0,   326,   341,     0,     0,   343,
     344,     0,     0,     0,     0,     0,   304,     0,     0,   311,
       0,   299,   306,   310,   307,   214,   173,     0,     0,     0,
       0,   250,   251,   164,   215,   190,   188,   189,   186,   187,
     211,   214,   213,    82,    79,   239,   243,     0,   231,   204,
     197,     0,     0,    94,    63,    66,     0,   235,     0,   304,
     229,   202,   275,   232,    65,   227,    37,   223,    30,    41,
       0,   286,    45,   224,   288,    47,    33,    43,   286,     0,
     291,   287,   139,     0,   281,   126,   132,   131,     0,   137,
     138,     0,   273,   329,     0,     0,   320,     0,   319,     0,
     336,   352,   303,     0,     0,     0,   350,   301,   330,   342,
       0,   308,     0,   321,     0,   304,   332,     0,   349,   337,
       0,    70,    69,   296,     0,   251,   207,    86,   214,     0,
      60,     0,   304,   304,   234,     0,   173,     0,   289,     0,
      46,     0,   142,   146,   143,   284,   285,   127,   292,   134,
      62,   328,   337,   298,   325,     0,     0,   304,   324,     0,
       0,   322,   309,   333,   298,   298,   340,   209,   338,    68,
      72,   216,     0,    88,   244,     0,     0,    57,     0,    64,
     237,   236,    92,   140,   225,    34,   145,   286,     0,   331,
       0,   353,   323,   334,   351,     0,     0,     0,   214,     0,
      87,    83,     0,     0,     0,   135,   337,   345,   337,   339,
     208,    84,    89,    59,    58,   147,   335,   210,   296,     0,
      85
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     6,     2,     3,    14,    21,    30,   105,    31,
       8,    24,    16,    17,    65,   329,    67,   149,   522,   523,
     145,   146,    68,   504,   330,   442,   505,   581,   391,   369,
     477,   238,   239,   240,    69,   127,   254,    70,   133,   381,
     577,   650,   668,   623,   651,    71,   143,   402,    72,   141,
      73,    74,    75,    76,   316,   427,   428,   594,    77,   318,
     244,   136,    78,   150,   111,   117,    13,    80,    81,   246,
     247,   163,   119,    82,    83,   484,   228,    84,   230,   231,
      85,    86,    87,   130,   214,    88,   253,   490,    89,    90,
      22,   282,   524,   277,   269,   260,   270,   271,   272,   262,
     387,   248,   249,   250,   331,   332,   324,   333,   273,   152,
      92,   319,   429,   430,   222,   377,   171,   140,   255,   470,
     555,   549,   399,   100,   212,   218,   616,   447,   349,   350,
     351,   353,   556,   551,   617,   618,   460,   461,    25,   471,
     557,   552
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -475
static const yytype_int16 yypact[] =
{
    -475,    49,    29,    32,  -475,    15,  -475,    62,  -475,  -475,
    -475,    67,    11,  -475,    72,    74,  -475,  -475,    64,  -475,
      96,    84,  1103,  -475,    89,   359,     9,  -475,   257,   131,
    -475,    32,   161,  -475,  -475,  -475,    15,  1850,  -475,    15,
     494,  -475,  -475,   305,   494,    15,  -475,    18,   100,  1688,
    -475,    18,  -475,   363,   393,  1688,  1688,  1688,  1688,  1688,
    1688,  1731,  1688,  1688,  1327,   110,  -475,   416,  -475,  -475,
    -475,  -475,  -475,   993,  -475,  -475,   113,   129,  -475,   116,
    -475,   145,   136,    18,   139,  -475,  -475,  -475,   154,    25,
    -475,  -475,    78,  -475,   172,    -3,   205,   172,   172,   192,
    -475,  -475,  -475,  -475,  -475,   200,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,   215,  1876,  1876,  1876,  -475,   216,  -475,
    -475,  -475,  -475,  -475,  -475,   155,   129,   797,   578,   230,
     235,   223,  -475,  1688,  -475,  -475,     7,  1876,  2231,   207,
    -475,   268,   357,  1688,   309,  1842,  -475,  -475,   427,  -475,
    -475,  -475,   873,  -475,  -475,  -475,  -475,  -475,  -475,  1774,
    1731,  2231,   237,  -475,   126,  -475,    93,  -475,  -475,   232,
    2231,   246,  -475,   460,  -475,   942,  1688,  1688,  1688,  1688,
    -475,  1688,  1688,  1688,  -475,  1688,  1688,  1688,  1688,  1688,
    1688,  1688,  1688,  1688,  1688,  1688,  1688,  1688,  1688,  -475,
    1370,   434,  1688,  -475,  1688,  -475,  -475,  1272,  1688,  1688,
    1688,  -475,  1962,    15,   235,   263,   329,  -475,  2057,  2057,
    -475,    43,   288,  -475,   578,   342,  1876,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,   291,    15,  1688,  -475,  -475,   321,
    -475,    82,   299,  1876,  -475,   578,  -475,  -475,  -475,   287,
     303,   578,  1272,  -475,  -475,   308,   176,   364,  -475,   341,
     340,  -475,  -475,   331,  -475,    48,    23,  -475,  -475,   346,
    -475,  -475,   401,  1817,  -475,  -475,  -475,   347,  1876,  -475,
    -475,  -475,   353,  1688,    15,   356,  1903,  -475,   360,  1876,
    1876,  -475,   362,  1688,   369,  2231,  2302,  -475,  2255,   935,
     935,   935,   935,  -475,   935,   935,  2279,  -475,   606,   606,
     606,   606,  -475,  -475,  -475,  -475,  1425,  -475,  -475,    30,
    1480,  -475,  2129,   375,  1198,  2096,  1425,  -475,  -475,  -475,
    -475,  -475,  -475,   160,   207,   207,  2231,  1970,   370,   368,
     371,  -475,   381,   445,  2057,   262,    47,  -475,   392,  -475,
    -475,  -475,  1996,  -475,   115,   399,    15,   400,   403,   406,
    -475,  -475,   410,  1876,   413,  -475,  -475,  2231,  -475,  -475,
    1535,  1590,  1688,  -475,  -475,  -475,   578,  -475,  1929,   414,
     163,   321,  1688,    15,   418,   419,   578,  -475,   456,   408,
    1876,    33,   364,   401,   364,   426,   281,   424,  -475,  -475,
      15,   401,   463,    15,   439,  -475,    15,   444,   207,  -475,
    1688,  1937,  1876,  -475,   143,   241,   260,   293,  -475,  -475,
    -475,    15,   449,   207,  1688,  -475,  2159,  -475,  -475,   435,
     446,   438,  1731,   459,   464,   467,  -475,  1688,  -475,  -475,
     471,   458,  1272,  1198,  -475,  2057,   496,  -475,  -475,  -475,
      15,  2023,  2057,    15,  2057,  -475,  -475,   532,    40,  -475,
    -475,   474,   465,  2057,   262,  2057,   401,    15,    15,  -475,
     479,   468,  -475,  -475,  -475,  1929,  -475,  1272,  1688,  1688,
     478,  -475,   578,   485,  -475,  -475,  -475,  -475,  -475,  -475,
    -475,  1929,  -475,  -475,  -475,  -475,  -475,   488,  -475,  -475,
    -475,  1731,   483,  -475,  -475,  -475,   490,  -475,   491,   401,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,   207,
     492,  1425,  -475,  -475,   493,   942,  -475,   207,  1425,  1633,
    1425,  -475,  -475,   497,  -475,  -475,  -475,  -475,    -7,  -475,
    -475,   180,  -475,  -475,   498,   500,   486,   502,   503,   495,
    -475,  -475,   504,   509,  2057,   506,  -475,   522,  -475,  -475,
     536,  -475,  2057,  -475,   525,   401,  -475,   529,  -475,  2049,
     189,  2231,  2231,  1688,   530,   578,  -475,  -475,  1929,    46,
    -475,  1198,   401,   401,  -475,    56,   361,   524,    15,   533,
     369,   535,  -475,  2231,  -475,  -475,  -475,  -475,  1688,  -475,
    -475,  -475,  2049,    15,  -475,  2023,  2057,   401,  -475,    15,
      40,  -475,  -475,  -475,    15,    15,  -475,  -475,  -475,  -475,
    -475,  -475,   537,   584,  -475,  1688,  1688,  -475,  1731,   542,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,  1425,   534,  -475,
     545,  -475,  -475,  -475,  -475,   548,   550,   553,  1929,   124,
    -475,  -475,  2183,  2207,   551,  -475,  2049,  -475,  2049,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  1688,   321,
    -475
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -475,  -475,  -475,  -475,  -475,  -475,  -475,    -9,  -475,  -475,
     585,  -475,    -4,  -475,  -475,   596,  -475,  -133,   -44,    35,
    -475,  -134,   -95,  -475,   -29,  -475,  -475,  -475,   111,  -373,
    -475,  -475,  -475,  -475,  -475,  -475,  -140,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,
     572,   520,   193,  -475,  -198,    94,    97,  -475,   240,   -59,
     383,   132,   -11,   348,   591,   270,   352,    21,  -475,   389,
     549,   475,  -475,  -475,  -475,  -475,   -36,   -37,   -31,    -6,
    -475,  -475,  -475,  -475,  -475,   -19,   425,  -474,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,   242,  -125,  -244,   253,
    -475,   267,  -475,  -189,  -304,   622,  -475,  -243,  -475,   -63,
      -5,   144,  -475,  -312,  -208,  -289,  -197,  -475,  -114,  -438,
    -475,  -475,  -364,  -475,   248,  -475,   466,  -475,   310,   209,
     320,   199,    59,    66,  -472,  -475,  -442,   208,  -475,   461,
    -475,  -475
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -279
static const yytype_int16 yytable[] =
{
     121,   120,   162,   274,   175,   323,   122,   326,   493,   380,
     276,   261,   545,   242,   441,   281,   560,   576,   123,   104,
     438,   393,   395,   174,   164,   110,   206,   108,   110,   507,
     433,   256,   237,   101,   132,   362,   440,   513,   237,     9,
     501,   431,   166,   456,   139,   502,    27,   384,   237,     4,
     462,   625,   389,   134,     5,   165,   213,     7,   118,   598,
     134,   397,   379,   501,     9,   599,   257,     9,   502,   404,
      18,    11,   384,    19,   407,   258,   394,   229,   229,   229,
      10,    11,   102,   208,   422,   135,    15,   370,    10,    11,
     457,   229,   135,    20,  -217,   232,   232,   232,    23,    27,
     229,   503,   566,   223,   624,   432,    29,   392,   229,   232,
     175,   626,   627,    10,    11,   229,    10,    11,   232,   463,
       9,   628,   241,   164,   632,   259,   232,    26,  -217,   292,
     639,   268,   385,   232,   106,   118,   118,   118,   229,   542,
     531,   166,   533,   209,   661,   584,    33,   371,   506,   118,
     508,    93,  -183,   210,   165,    28,   232,   210,   118,    29,
    -217,  -185,  -269,   164,   109,   640,   118,  -269,   645,    10,
      11,   137,   172,   118,   660,   467,   646,   647,  -181,  -238,
     199,   166,  -155,   142,   666,  -184,   667,   229,   200,   229,
     468,   328,   201,  -185,   165,   204,   118,   497,  -184,   541,
     202,   613,  -181,   334,   335,   232,   229,   232,   229,   587,
    -181,   203,   360,  -183,   229,   205,   591,  -269,   630,   631,
     521,   207,   443,  -269,   232,   443,   232,   528,   444,   216,
    -268,   492,   232,   125,   570,  -268,   229,   131,  -238,   383,
     538,   229,   443,   643,  -238,   118,    11,   118,   600,   415,
     414,   443,   229,   229,   232,   416,   220,   619,   480,   232,
      27,   435,   221,   261,   118,   456,   118,   417,   494,   515,
     232,   232,   118,   517,   224,    12,  -179,   237,   408,   235,
     126,     9,   210,   164,   126,  -268,     9,   237,   423,   251,
      32,  -268,    79,   574,   118,  -180,   670,   288,    32,   118,
    -179,   166,   252,   264,   289,   256,   112,   118,  -179,   112,
     118,   118,   457,   129,   165,   112,   589,   103,   290,  -180,
      29,   458,   356,   147,   151,   654,   229,  -180,  -178,     9,
      10,    11,   357,     9,   265,    10,    11,   151,   629,   229,
     266,   486,   485,   215,   232,   217,   219,   487,   361,   229,
     363,   365,  -178,   229,    10,    11,   368,   232,   525,   489,
    -178,   372,   376,   378,   128,   334,   335,   232,    94,   275,
     382,   232,   488,   534,   229,   229,    95,   259,    10,    11,
      96,   256,    10,    11,   118,   268,   622,     9,   384,   512,
      97,    98,   232,   232,   637,   164,  -182,   118,   245,   118,
     386,   638,   388,   390,   398,   519,   112,   118,   396,   403,
     265,   118,   112,   166,   147,   406,   266,     9,   151,   527,
    -182,   410,   144,    99,   421,   267,   165,   418,  -182,   450,
      10,    11,   118,   118,   424,   451,    10,    11,   486,   485,
       9,   437,   452,   151,   487,   229,   211,   211,   453,   211,
     211,     9,   148,   454,   486,   485,   489,   464,     9,   620,
     487,   469,   472,   232,   164,   473,    10,    11,   474,   488,
     475,   321,   489,   476,   491,   173,   499,    79,   237,   496,
     256,   383,   166,   352,     9,   488,   509,   280,   229,    10,
      11,    32,   511,   320,   245,   165,   118,    35,   514,   516,
      10,    11,    37,   118,   518,   366,   232,    10,    11,   526,
     530,   113,   118,   439,   532,   257,    47,    48,     9,   535,
     291,   245,    79,    51,   536,   525,   540,   537,   669,    10,
      11,   539,   343,    10,    11,   559,   561,   562,   229,   569,
     573,   486,   485,   468,   575,  -159,   118,   487,   578,   580,
     582,   583,   586,    61,   151,   588,   232,   597,   601,   489,
     602,   603,  -160,   604,   347,    64,   605,    10,    11,   534,
     347,   347,   488,   237,   608,   153,   154,   155,   156,   157,
     158,   606,   167,   168,   609,   610,    37,   612,   614,   243,
     621,   164,   633,   635,    79,   113,   118,   648,   649,   118,
      47,    48,     9,   636,   443,   656,   655,    51,   657,   166,
     658,   486,   485,   659,   225,   459,   107,   487,    66,   665,
     662,   138,   165,   634,   595,   585,   352,   596,   374,   489,
     124,   115,   409,   161,   375,   287,   170,   226,   510,   355,
     178,   498,   488,   481,    91,   579,   245,   448,   483,    64,
     186,    10,    11,   495,   190,   544,   245,   449,   112,   195,
     196,   197,   198,   227,   233,   234,   112,   568,   644,   118,
     112,   641,   564,   147,   354,     0,   151,     0,   348,   153,
     157,     0,     0,     0,   358,   359,   263,     0,     0,   347,
       0,   151,     0,     0,   279,     0,   347,     0,     0,     0,
       0,   285,     0,     0,   347,     0,     0,     0,     0,     0,
       0,     0,    79,    79,     0,     0,     0,     0,     0,     0,
     352,   547,     0,   554,   294,     0,     0,     0,   459,     0,
       0,     0,     0,     0,   459,     0,     0,   567,   352,     0,
       0,     0,     0,     0,     0,     0,     0,    79,   295,   296,
     297,   298,   245,   299,   300,   301,     0,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,     0,   161,     0,   322,   364,   325,     0,     0,     0,
     138,   138,   336,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   373,     0,     0,     0,     0,   347,     0,     0,
      35,     0,     0,   548,   347,    37,   347,     0,   367,     0,
     455,     0,     0,     0,   113,   347,     0,   347,   466,    47,
      48,     9,   401,   236,     0,     0,    51,   405,     0,     0,
       0,     0,     0,    55,     0,   413,     0,     0,   419,   420,
       0,     0,     0,     0,     0,   245,    56,    57,     0,    58,
      59,    79,     0,    60,     0,   138,    61,     0,   151,     0,
       0,     0,     0,     0,     0,   138,    62,    63,    64,     0,
      10,    11,     0,   352,     0,   547,     0,     0,     0,   554,
     459,    37,     0,     0,   352,   352,     0,     0,   426,     0,
     113,     0,   161,     0,     0,    47,    48,     9,   426,     0,
       0,     0,    51,     0,     0,     0,   347,     0,     0,   225,
       0,   543,   413,     0,   347,     0,     0,   550,   553,     0,
     558,   347,     0,     0,     0,     0,   115,     0,     0,   563,
       0,   565,   226,     0,     0,     0,     0,     0,   283,   500,
       0,     0,   138,   138,    64,     0,    10,    11,   284,     0,
      37,     0,     0,     0,   347,     0,     0,   548,   347,   113,
     227,   520,     0,     0,    47,    48,     9,     0,     0,   178,
       0,    51,     0,     0,     0,     0,     0,     0,   225,   186,
       0,     0,   138,   190,   191,   192,   193,   194,   195,   196,
     197,   198,     0,     0,     0,   115,   138,   176,  -278,     0,
       0,   226,     0,     0,   161,     0,     0,   293,   347,   170,
     347,     0,     0,    64,     0,    10,    11,   284,     0,     0,
     607,     0,     0,     0,     0,     0,   177,   178,   611,   179,
     180,   181,   182,   183,     0,   184,   185,   186,   187,   188,
     189,   190,   191,   192,   193,   194,   195,   196,   197,   198,
     571,   572,     0,     0,     0,     0,     0,     0,  -278,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  -278,     0,
       0,   550,   642,   161,   590,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   426,     0,     0,     0,     0,     0,     0,
     426,   593,   426,    -2,    34,     0,    35,     0,     0,    36,
       0,    37,    38,    39,     0,     0,    40,     0,    41,    42,
      43,    44,    45,    46,     0,    47,    48,     9,     0,     0,
      49,    50,    51,    52,    53,    54,     0,     0,     0,    55,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    56,    57,     0,    58,    59,     0,     0,    60,
       0,     0,    61,     0,     0,   -24,     0,     0,     0,     0,
     170,     0,    62,    63,    64,     0,    10,    11,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   652,   653,   327,
     161,    35,     0,     0,    36,  -253,    37,    38,    39,   426,
    -253,    40,     0,    41,    42,   113,    44,    45,    46,     0,
      47,    48,     9,     0,     0,    49,    50,    51,    52,    53,
      54,     0,     0,     0,    55,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    56,    57,     0,
      58,    59,     0,     0,    60,     0,     0,    61,     0,     0,
    -253,     0,     0,     0,     0,   328,  -253,    62,    63,    64,
       0,    10,    11,   327,     0,    35,     0,     0,    36,     0,
      37,    38,    39,     0,     0,    40,     0,    41,    42,   113,
      44,    45,    46,     0,    47,    48,     9,     0,     0,    49,
      50,    51,    52,    53,    54,     0,     0,     0,    55,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    56,    57,     0,    58,    59,     0,     0,    60,     0,
      35,    61,     0,     0,  -253,    37,     0,     0,   169,   328,
    -253,    62,    63,    64,   113,    10,    11,     0,     0,    47,
      48,     9,     0,     0,     0,     0,    51,     0,     0,     0,
       0,     0,     0,    55,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    35,     0,     0,    56,    57,    37,    58,
      59,     0,     0,    60,     0,     0,    61,   113,     0,     0,
       0,     0,    47,    48,     9,     0,    62,    63,    64,    51,
      10,    11,     0,     0,     0,     0,   159,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    56,
      57,     0,    58,   160,     0,     0,    60,     0,    35,    61,
     317,     0,     0,    37,     0,     0,     0,     0,     0,    62,
      63,    64,   113,    10,    11,     0,     0,    47,    48,     9,
       0,     0,     0,     0,    51,     0,     0,     0,     0,     0,
       0,    55,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    56,    57,     0,    58,    59,     0,
       0,    60,     0,    35,    61,     0,     0,     0,    37,     0,
       0,     0,   425,     0,    62,    63,    64,   113,    10,    11,
       0,     0,    47,    48,     9,     0,     0,     0,     0,    51,
       0,   434,     0,     0,     0,     0,   159,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    56,
      57,     0,    58,   160,     0,     0,    60,     0,    35,    61,
       0,     0,     0,    37,     0,     0,     0,     0,     0,    62,
      63,    64,   113,    10,    11,     0,     0,    47,    48,     9,
       0,   478,     0,     0,    51,     0,     0,     0,     0,     0,
       0,    55,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    56,    57,     0,    58,    59,     0,
       0,    60,     0,    35,    61,     0,     0,     0,    37,     0,
       0,     0,     0,     0,    62,    63,    64,   113,    10,    11,
       0,     0,    47,    48,     9,     0,   479,     0,     0,    51,
       0,     0,     0,     0,     0,     0,    55,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    35,     0,     0,    56,
      57,    37,    58,    59,     0,     0,    60,     0,     0,    61,
     113,     0,     0,     0,     0,    47,    48,     9,     0,    62,
      63,    64,    51,    10,    11,     0,     0,     0,     0,    55,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    56,    57,     0,    58,    59,     0,     0,    60,
       0,    35,    61,     0,     0,     0,    37,     0,     0,     0,
     592,     0,    62,    63,    64,   113,    10,    11,     0,     0,
      47,    48,     9,     0,     0,     0,     0,    51,     0,     0,
       0,     0,     0,     0,    55,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    35,     0,     0,    56,    57,    37,
      58,    59,     0,     0,    60,     0,     0,    61,   113,     0,
       0,     0,     0,    47,    48,     9,     0,    62,    63,    64,
      51,    10,    11,     0,     0,     0,     0,   159,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    35,     0,     0,
      56,    57,   286,    58,   160,     0,     0,    60,     0,     0,
      61,   113,     0,     0,     0,     0,    47,    48,     9,     0,
      62,    63,    64,    51,    10,    11,     0,     0,     0,     0,
      55,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    56,    57,    37,    58,    59,     0,     0,
      60,     0,     0,    61,   113,     0,     0,     0,     0,    47,
      48,     9,     0,    62,    63,    64,    51,    10,    11,     0,
      37,     0,     0,   225,     0,     0,     0,     0,    37,   113,
       0,     0,     0,     0,    47,    48,     9,   113,     0,     0,
     115,    51,    47,    48,     9,     0,   226,     0,   225,    51,
       0,     0,     0,     0,    37,     0,   114,     0,    64,     0,
      10,    11,   400,   113,     0,   115,     0,     0,    47,    48,
       9,   226,     0,   115,     0,    51,     0,   278,     0,   116,
       0,    37,   225,    64,     0,    10,    11,     0,     0,     0,
     113,    64,     0,    10,    11,    47,    48,     9,     0,   115,
       0,     0,    51,     0,     0,   226,     0,    37,     0,   411,
       0,     0,     0,     0,     0,   286,   113,    64,     0,    10,
      11,    47,    48,     9,   113,     0,   115,     0,    51,    47,
      48,     9,   412,     0,     0,   225,    51,     0,     0,     0,
     337,     0,     0,   225,    64,     0,    10,    11,   337,   338,
       0,     0,   115,     0,   339,   340,   341,   338,   482,     0,
     115,   342,   339,   340,   341,     0,   226,     0,   343,   342,
      64,     0,    10,    11,   337,     0,   445,   465,    64,     0,
      10,    11,     0,   338,     0,   344,     0,     0,   339,   340,
     341,     0,     0,   344,     0,   342,     0,   345,     0,   446,
       0,   337,   343,   346,     0,     0,    11,     0,     0,     0,
     338,   346,     0,     0,    11,   339,   340,   546,     0,   344,
       0,     0,   342,     0,     0,     0,     0,   337,     0,   343,
       0,     0,     0,     0,     0,   337,   338,   346,     0,     0,
      11,   339,   340,   341,   338,     0,   344,     0,   342,   339,
     340,   341,     0,     0,     0,   343,   342,     0,     0,     0,
       0,     0,     0,   343,   346,     0,    10,    11,     0,     0,
       0,     0,   344,     0,     0,     0,     0,     0,   615,     0,
     344,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     346,     0,     0,    11,     0,     0,     0,     0,   346,   177,
     178,    11,   179,     0,   181,   182,   183,     0,     0,   185,
     186,   187,   188,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   177,   178,     0,   179,     0,   181,   182,   183,
       0,   439,   185,   186,   187,   188,   189,   190,   191,   192,
     193,   194,   195,   196,   197,   198,     0,     0,     0,     0,
       0,     0,   177,   178,     0,   179,     0,   181,   182,   183,
       0,   436,   185,   186,   187,   188,   189,   190,   191,   192,
     193,   194,   195,   196,   197,   198,   177,   178,     0,   179,
       0,   181,   182,   183,     0,   529,   185,   186,   187,   188,
     189,   190,   191,   192,   193,   194,   195,   196,   197,   198,
     177,   178,     0,   179,     0,   181,   182,   183,     0,   663,
     185,   186,   187,   188,   189,   190,   191,   192,   193,   194,
     195,   196,   197,   198,   177,   178,     0,   179,     0,   181,
     182,   183,     0,   664,   185,   186,   187,   188,   189,   190,
     191,   192,   193,   194,   195,   196,   197,   198,   177,   178,
       0,     0,     0,   181,   182,   183,     0,     0,   185,   186,
     187,   188,   189,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   177,   178,     0,     0,     0,   181,   182,   183,
       0,     0,   185,   186,   187,   188,     0,   190,   191,   192,
     193,   194,   195,   196,   197,   198,   178,     0,     0,     0,
     181,   182,   183,     0,     0,   185,   186,   187,   188,     0,
     190,   191,   192,   193,   194,   195,   196,   197,   198
};

#define yypact_value_is_default(yystate) \
  ((yystate) == (-475))

#define yytable_value_is_error(yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
      37,    37,    61,   143,    67,   202,    37,   205,   381,   252,
     144,   136,   450,   127,   326,   148,   458,   491,    37,    28,
     324,   265,   266,    67,    61,    36,     1,    31,    39,   393,
     319,    24,   127,    24,    45,   224,   325,   401,   133,    24,
       7,    11,    61,     3,    49,    12,     3,    24,   143,     0,
       3,     5,   260,    35,    25,    61,    59,    25,    37,    66,
      35,   269,   251,     7,    24,    72,    59,    24,    12,   277,
       3,    74,    24,    62,   282,    68,    53,   114,   115,   116,
      73,    74,    73,     5,   292,    67,    24,     5,    73,    74,
      50,   128,    67,    21,     1,   114,   115,   116,    24,     3,
     137,    68,   466,   107,   578,    75,    63,    59,   145,   128,
     173,    65,    66,    73,    74,   152,    73,    74,   137,    72,
      24,    75,   127,   160,    68,   136,   145,    63,    35,   173,
     602,   142,   257,   152,     3,   114,   115,   116,   175,   443,
     429,   160,   431,    65,    20,   509,    62,    65,   392,   128,
     394,    62,    59,    75,   160,    59,   175,    75,   137,    63,
      67,    35,     7,   200,     3,   603,   145,    12,   610,    73,
      74,    71,    62,   152,   648,    60,   614,   615,    35,     3,
      67,   200,    66,    51,   656,    59,   658,   224,    59,   226,
      75,    67,    63,    67,   200,    59,   175,   386,    59,   442,
      71,   565,    59,   208,   209,   224,   243,   226,   245,   521,
      67,    66,   221,    59,   251,    83,   528,    62,   582,   583,
     418,    89,    62,    68,   243,    62,   245,   425,    68,    24,
       7,    68,   251,    40,   477,    12,   273,    44,    62,    63,
     437,   278,    62,   607,    68,   224,    74,   226,    68,   286,
     286,    62,   289,   290,   273,   286,    64,    68,   372,   278,
       3,   320,    62,   388,   243,     3,   245,   286,   382,   403,
     289,   290,   251,   406,    59,     5,    35,   372,   283,    63,
      40,    24,    75,   320,    44,    62,    24,   382,   293,    59,
      20,    68,    22,   482,   273,    35,   669,    60,    28,   278,
      59,   320,    67,    35,    72,    24,    36,   286,    67,    39,
     289,   290,    50,    43,   320,    45,   524,    60,    72,    59,
      63,    59,    59,    53,    54,   637,   363,    67,    35,    24,
      73,    74,     3,    24,    53,    73,    74,    67,   581,   376,
      59,   378,   378,    95,   363,    97,    98,   378,    60,   386,
       8,    60,    59,   390,    73,    74,    35,   376,   421,   378,
      67,    62,    75,    60,    59,   370,   371,   386,     9,    60,
      62,   390,   378,   432,   411,   412,    17,   388,    73,    74,
      21,    24,    73,    74,   363,   396,   575,    24,    24,   400,
      31,    32,   411,   412,   592,   432,    35,   376,   128,   378,
      59,   598,    62,    72,     3,   410,   136,   386,    62,    62,
      53,   390,   142,   432,   144,    62,    59,    24,   148,   424,
      59,    65,    59,    64,    62,    68,   432,    67,    67,    59,
      73,    74,   411,   412,    65,    67,    73,    74,   475,   475,
      24,    66,    71,   173,   475,   482,    94,    95,    67,    97,
      98,    24,    59,     8,   491,   491,   475,    65,    24,   573,
     491,    62,    62,   482,   501,    62,    73,    74,    62,   475,
      60,   201,   491,    60,    60,    59,    68,   207,   573,    60,
      24,    63,   501,   213,    24,   491,    60,    60,   525,    73,
      74,   221,    68,    59,   224,   501,   475,     3,    35,    60,
      73,    74,     8,   482,    60,   235,   525,    73,    74,    60,
      75,    17,   491,    75,    68,    59,    22,    23,    24,    60,
      60,   251,   252,    29,    60,   588,    68,    60,   668,    73,
      74,    60,    36,    73,    74,     3,    62,    72,   575,    60,
      62,   578,   578,    75,    59,    59,   525,   578,    60,    66,
      60,    60,    60,    59,   284,    62,   575,    60,    60,   578,
      60,    59,    59,    68,   212,    71,    62,    73,    74,   628,
     218,   219,   578,   668,    68,    55,    56,    57,    58,    59,
      60,    72,    62,    63,    62,    49,     8,    62,    59,    11,
      60,   628,    68,    60,   324,    17,   575,    60,    14,   578,
      22,    23,    24,    68,    62,    60,    72,    29,    60,   628,
      60,   648,   648,    60,    36,   345,    31,   648,    22,    68,
     649,    49,   628,   588,   530,   514,   356,   530,   245,   648,
      39,    53,   284,    61,   245,   160,    64,    59,   396,   214,
      34,   388,   648,   376,    22,   501,   376,   337,   378,    71,
      44,    73,    74,   383,    48,   446,   386,   337,   388,    53,
      54,    55,    56,   114,   115,   116,   396,   468,   609,   648,
     400,   605,   464,   403,   213,    -1,   406,    -1,   212,   159,
     160,    -1,    -1,    -1,   218,   219,   137,    -1,    -1,   337,
      -1,   421,    -1,    -1,   145,    -1,   344,    -1,    -1,    -1,
      -1,   152,    -1,    -1,   352,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   442,   443,    -1,    -1,    -1,    -1,    -1,    -1,
     450,   451,    -1,   453,   175,    -1,    -1,    -1,   458,    -1,
      -1,    -1,    -1,    -1,   464,    -1,    -1,   467,   468,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   477,   176,   177,
     178,   179,   482,   181,   182,   183,    -1,   185,   186,   187,
     188,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,    -1,   200,    -1,   202,   226,   204,    -1,    -1,    -1,
     208,   209,   210,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   243,    -1,    -1,    -1,    -1,   445,    -1,    -1,
       3,    -1,    -1,   451,   452,     8,   454,    -1,   236,    -1,
     344,    -1,    -1,    -1,    17,   463,    -1,   465,   352,    22,
      23,    24,   273,    26,    -1,    -1,    29,   278,    -1,    -1,
      -1,    -1,    -1,    36,    -1,   286,    -1,    -1,   289,   290,
      -1,    -1,    -1,    -1,    -1,   575,    49,    50,    -1,    52,
      53,   581,    -1,    56,    -1,   283,    59,    -1,   588,    -1,
      -1,    -1,    -1,    -1,    -1,   293,    69,    70,    71,    -1,
      73,    74,    -1,   603,    -1,   605,    -1,    -1,    -1,   609,
     610,     8,    -1,    -1,   614,   615,    -1,    -1,   316,    -1,
      17,    -1,   320,    -1,    -1,    22,    23,    24,   326,    -1,
      -1,    -1,    29,    -1,    -1,    -1,   554,    -1,    -1,    36,
      -1,   445,   363,    -1,   562,    -1,    -1,   451,   452,    -1,
     454,   569,    -1,    -1,    -1,    -1,    53,    -1,    -1,   463,
      -1,   465,    59,    -1,    -1,    -1,    -1,    -1,    65,   390,
      -1,    -1,   370,   371,    71,    -1,    73,    74,    75,    -1,
       8,    -1,    -1,    -1,   602,    -1,    -1,   605,   606,    17,
     411,   412,    -1,    -1,    22,    23,    24,    -1,    -1,    34,
      -1,    29,    -1,    -1,    -1,    -1,    -1,    -1,    36,    44,
      -1,    -1,   410,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    -1,    -1,    -1,    53,   424,     4,     5,    -1,
      -1,    59,    -1,    -1,   432,    -1,    -1,    65,   656,   437,
     658,    -1,    -1,    71,    -1,    73,    74,    75,    -1,    -1,
     554,    -1,    -1,    -1,    -1,    -1,    33,    34,   562,    36,
      37,    38,    39,    40,    -1,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
     478,   479,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    75,    -1,
      -1,   605,   606,   501,   525,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   521,    -1,    -1,    -1,    -1,    -1,    -1,
     528,   529,   530,     0,     1,    -1,     3,    -1,    -1,     6,
      -1,     8,     9,    10,    -1,    -1,    13,    -1,    15,    16,
      17,    18,    19,    20,    -1,    22,    23,    24,    -1,    -1,
      27,    28,    29,    30,    31,    32,    -1,    -1,    -1,    36,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    49,    50,    -1,    52,    53,    -1,    -1,    56,
      -1,    -1,    59,    -1,    -1,    62,    -1,    -1,    -1,    -1,
     598,    -1,    69,    70,    71,    -1,    73,    74,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   625,   626,     1,
     628,     3,    -1,    -1,     6,     7,     8,     9,    10,   637,
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
       3,    59,    -1,    -1,    62,     8,    -1,    -1,    11,    67,
      68,    69,    70,    71,    17,    73,    74,    -1,    -1,    22,
      23,    24,    -1,    -1,    -1,    -1,    29,    -1,    -1,    -1,
      -1,    -1,    -1,    36,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     3,    -1,    -1,    49,    50,     8,    52,
      53,    -1,    -1,    56,    -1,    -1,    59,    17,    -1,    -1,
      -1,    -1,    22,    23,    24,    -1,    69,    70,    71,    29,
      73,    74,    -1,    -1,    -1,    -1,    36,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,
      50,    -1,    52,    53,    -1,    -1,    56,    -1,     3,    59,
      60,    -1,    -1,     8,    -1,    -1,    -1,    -1,    -1,    69,
      70,    71,    17,    73,    74,    -1,    -1,    22,    23,    24,
      -1,    -1,    -1,    -1,    29,    -1,    -1,    -1,    -1,    -1,
      -1,    36,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    49,    50,    -1,    52,    53,    -1,
      -1,    56,    -1,     3,    59,    -1,    -1,    -1,     8,    -1,
      -1,    -1,    67,    -1,    69,    70,    71,    17,    73,    74,
      -1,    -1,    22,    23,    24,    -1,    -1,    -1,    -1,    29,
      -1,    31,    -1,    -1,    -1,    -1,    36,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,
      50,    -1,    52,    53,    -1,    -1,    56,    -1,     3,    59,
      -1,    -1,    -1,     8,    -1,    -1,    -1,    -1,    -1,    69,
      70,    71,    17,    73,    74,    -1,    -1,    22,    23,    24,
      -1,    26,    -1,    -1,    29,    -1,    -1,    -1,    -1,    -1,
      -1,    36,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    49,    50,    -1,    52,    53,    -1,
      -1,    56,    -1,     3,    59,    -1,    -1,    -1,     8,    -1,
      -1,    -1,    -1,    -1,    69,    70,    71,    17,    73,    74,
      -1,    -1,    22,    23,    24,    -1,    26,    -1,    -1,    29,
      -1,    -1,    -1,    -1,    -1,    -1,    36,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     3,    -1,    -1,    49,
      50,     8,    52,    53,    -1,    -1,    56,    -1,    -1,    59,
      17,    -1,    -1,    -1,    -1,    22,    23,    24,    -1,    69,
      70,    71,    29,    73,    74,    -1,    -1,    -1,    -1,    36,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    49,    50,    -1,    52,    53,    -1,    -1,    56,
      -1,     3,    59,    -1,    -1,    -1,     8,    -1,    -1,    -1,
      67,    -1,    69,    70,    71,    17,    73,    74,    -1,    -1,
      22,    23,    24,    -1,    -1,    -1,    -1,    29,    -1,    -1,
      -1,    -1,    -1,    -1,    36,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     3,    -1,    -1,    49,    50,     8,
      52,    53,    -1,    -1,    56,    -1,    -1,    59,    17,    -1,
      -1,    -1,    -1,    22,    23,    24,    -1,    69,    70,    71,
      29,    73,    74,    -1,    -1,    -1,    -1,    36,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,    -1,    -1,
      49,    50,     8,    52,    53,    -1,    -1,    56,    -1,    -1,
      59,    17,    -1,    -1,    -1,    -1,    22,    23,    24,    -1,
      69,    70,    71,    29,    73,    74,    -1,    -1,    -1,    -1,
      36,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    49,    50,     8,    52,    53,    -1,    -1,
      56,    -1,    -1,    59,    17,    -1,    -1,    -1,    -1,    22,
      23,    24,    -1,    69,    70,    71,    29,    73,    74,    -1,
       8,    -1,    -1,    36,    -1,    -1,    -1,    -1,     8,    17,
      -1,    -1,    -1,    -1,    22,    23,    24,    17,    -1,    -1,
      53,    29,    22,    23,    24,    -1,    59,    -1,    36,    29,
      -1,    -1,    -1,    -1,     8,    -1,    36,    -1,    71,    -1,
      73,    74,    75,    17,    -1,    53,    -1,    -1,    22,    23,
      24,    59,    -1,    53,    -1,    29,    -1,    65,    -1,    59,
      -1,     8,    36,    71,    -1,    73,    74,    -1,    -1,    -1,
      17,    71,    -1,    73,    74,    22,    23,    24,    -1,    53,
      -1,    -1,    29,    -1,    -1,    59,    -1,     8,    -1,    36,
      -1,    -1,    -1,    -1,    -1,     8,    17,    71,    -1,    73,
      74,    22,    23,    24,    17,    -1,    53,    -1,    29,    22,
      23,    24,    59,    -1,    -1,    36,    29,    -1,    -1,    -1,
       8,    -1,    -1,    36,    71,    -1,    73,    74,     8,    17,
      -1,    -1,    53,    -1,    22,    23,    24,    17,    59,    -1,
      53,    29,    22,    23,    24,    -1,    59,    -1,    36,    29,
      71,    -1,    73,    74,     8,    -1,    36,    11,    71,    -1,
      73,    74,    -1,    17,    -1,    53,    -1,    -1,    22,    23,
      24,    -1,    -1,    53,    -1,    29,    -1,    65,    -1,    59,
      -1,     8,    36,    71,    -1,    -1,    74,    -1,    -1,    -1,
      17,    71,    -1,    -1,    74,    22,    23,    24,    -1,    53,
      -1,    -1,    29,    -1,    -1,    -1,    -1,     8,    -1,    36,
      -1,    -1,    -1,    -1,    -1,     8,    17,    71,    -1,    -1,
      74,    22,    23,    24,    17,    -1,    53,    -1,    29,    22,
      23,    24,    -1,    -1,    -1,    36,    29,    -1,    -1,    -1,
      -1,    -1,    -1,    36,    71,    -1,    73,    74,    -1,    -1,
      -1,    -1,    53,    -1,    -1,    -1,    -1,    -1,    59,    -1,
      53,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      71,    -1,    -1,    74,    -1,    -1,    -1,    -1,    71,    33,
      34,    74,    36,    -1,    38,    39,    40,    -1,    -1,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    33,    34,    -1,    36,    -1,    38,    39,    40,
      -1,    75,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    -1,    -1,    -1,    -1,
      -1,    -1,    33,    34,    -1,    36,    -1,    38,    39,    40,
      -1,    72,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    33,    34,    -1,    36,
      -1,    38,    39,    40,    -1,    66,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      33,    34,    -1,    36,    -1,    38,    39,    40,    -1,    66,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    33,    34,    -1,    36,    -1,    38,
      39,    40,    -1,    66,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    33,    34,
      -1,    -1,    -1,    38,    39,    40,    -1,    -1,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    33,    34,    -1,    -1,    -1,    38,    39,    40,
      -1,    -1,    43,    44,    45,    46,    -1,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    34,    -1,    -1,    -1,
      38,    39,    40,    -1,    -1,    43,    44,    45,    46,    -1,
      48,    49,    50,    51,    52,    53,    54,    55,    56
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
     154,   155,   161,   146,   146,    63,    26,    98,   107,   108,
     109,   186,   194,    11,   136,   141,   145,   146,   177,   178,
     179,    59,    67,   162,   112,   194,    24,    59,    68,   138,
     171,   173,   175,   146,    35,    53,    59,    68,   138,   170,
     172,   173,   174,   184,   112,    60,    97,   169,    65,   146,
      60,    93,   167,    65,    75,   146,     8,   147,    60,    72,
      72,    60,    94,    65,   146,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   130,    60,   135,   187,
      59,   141,   126,   192,   182,   126,   130,     1,    67,    91,
     100,   180,   181,   183,   186,   186,   126,     8,    17,    22,
      23,    24,    29,    36,    53,    65,    71,   142,   202,   204,
     205,   206,   141,   207,   215,   162,    59,     3,   202,   202,
      83,    60,   179,     8,   146,    60,   141,   126,    35,   105,
       5,    65,    62,   146,   136,   145,    75,   191,    60,   179,
     183,   115,    62,    63,    24,   173,    59,   176,    62,   190,
      72,   104,    59,   174,    53,   174,    62,   190,     3,   198,
      75,   146,   123,    62,   190,   146,    62,   190,   186,   139,
      65,    36,    59,   146,   152,   153,   154,   161,    67,   146,
     146,    62,   190,   186,    65,    67,   126,   131,   132,   188,
     189,    11,    75,   191,    31,   135,    72,    66,   180,    75,
     191,   189,   101,    62,    68,    36,    59,   203,   204,   206,
      59,    67,    71,    67,     8,   202,     3,    50,    59,   141,
     212,   213,     3,    72,    65,    11,   202,    60,    75,    62,
     195,   215,    62,    62,    62,    60,    60,   106,    26,    26,
     194,   177,    59,   141,   151,   152,   153,   154,   155,   161,
     163,    60,    68,   105,   194,   141,    60,   179,   175,    68,
     146,     7,    12,    68,    99,   102,   174,   198,   174,    60,
     172,    68,   138,   198,    35,    97,    60,    93,    60,   186,
     146,   130,    94,    95,   168,   185,    60,   186,   130,    66,
      75,   191,    68,   191,   135,    60,    60,    60,   192,    60,
      68,   183,   180,   202,   205,   195,    24,   141,   142,   197,
     202,   209,   217,   202,   141,   196,   208,   216,   202,     3,
     212,    62,    72,   202,   213,   202,   198,   141,   207,    60,
     183,   126,   126,    62,   179,    59,   163,   116,    60,   187,
      66,   103,    60,    60,   198,   104,    60,   189,    62,   190,
     146,   189,    67,   126,   133,   131,   132,    60,    66,    72,
      68,    60,    60,    59,    68,    62,    72,   202,    68,    62,
      49,   202,    62,   198,    59,    59,   202,   210,   211,    68,
     194,    60,   179,   119,   163,     5,    65,    66,    75,   183,
     198,   198,    68,    68,    95,    60,    68,   130,   192,   210,
     195,   209,   202,   198,   208,   212,   195,   195,    60,    14,
     117,   120,   126,   126,   189,    72,    60,    60,    60,    60,
     163,    20,   100,    66,    66,    68,   210,   210,   118,   112,
     105
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
#line 462 "go.y"
    {
		(yyval.node) = nod(OASOP, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
		(yyval.node)->etype = (yyvsp[(2) - (3)].i);			// rathole to pass opcode
	}
    break;

  case 53:

/* Line 1806 of yacc.c  */
#line 468 "go.y"
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
#line 481 "go.y"
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
#line 498 "go.y"
    {
		(yyval.node) = nod(OASOP, (yyvsp[(1) - (2)].node), nodintconst(1));
		(yyval.node)->etype = OADD;
	}
    break;

  case 56:

/* Line 1806 of yacc.c  */
#line 504 "go.y"
    {
		(yyval.node) = nod(OASOP, (yyvsp[(1) - (2)].node), nodintconst(1));
		(yyval.node)->etype = OSUB;
	}
    break;

  case 57:

/* Line 1806 of yacc.c  */
#line 511 "go.y"
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
#line 531 "go.y"
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
#line 549 "go.y"
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
#line 558 "go.y"
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
#line 576 "go.y"
    {
		markdcl();
	}
    break;

  case 62:

/* Line 1806 of yacc.c  */
#line 580 "go.y"
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
#line 590 "go.y"
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
#line 600 "go.y"
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
#line 620 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 66:

/* Line 1806 of yacc.c  */
#line 624 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (2)].list), (yyvsp[(2) - (2)].node));
	}
    break;

  case 67:

/* Line 1806 of yacc.c  */
#line 630 "go.y"
    {
		markdcl();
	}
    break;

  case 68:

/* Line 1806 of yacc.c  */
#line 634 "go.y"
    {
		(yyval.list) = (yyvsp[(3) - (4)].list);
		popdcl();
	}
    break;

  case 69:

/* Line 1806 of yacc.c  */
#line 642 "go.y"
    {
		(yyval.node) = nod(ORANGE, N, (yyvsp[(4) - (4)].node));
		(yyval.node)->list = (yyvsp[(1) - (4)].list);
		(yyval.node)->etype = 0;	// := flag
	}
    break;

  case 70:

/* Line 1806 of yacc.c  */
#line 650 "go.y"
    {
		(yyval.node) = nod(ORANGE, N, (yyvsp[(4) - (4)].node));
		(yyval.node)->list = (yyvsp[(1) - (4)].list);
		(yyval.node)->colas = 1;
		colasdefn((yyvsp[(1) - (4)].list), (yyval.node));
	}
    break;

  case 71:

/* Line 1806 of yacc.c  */
#line 658 "go.y"
    {
		// normal test
		(yyval.node) = nod(ORANGE, N, (yyvsp[(2) - (2)].node));
		(yyval.node)->list = list1(nod(ONAME, N, N));
		(yyval.node)->etype = 0;	// := flag

	}
    break;

  case 72:

/* Line 1806 of yacc.c  */
#line 670 "go.y"
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

  case 73:

/* Line 1806 of yacc.c  */
#line 682 "go.y"
    {
		// normal test
		(yyval.node) = nod(OFOR, N, N);
		(yyval.node)->ntest = (yyvsp[(1) - (1)].node);
	}
    break;

  case 75:

/* Line 1806 of yacc.c  */
#line 692 "go.y"
    {
		(yyval.node) = (yyvsp[(1) - (2)].node);
		(yyval.node)->nbody = concat((yyval.node)->nbody, (yyvsp[(2) - (2)].list));
	}
    break;

  case 76:

/* Line 1806 of yacc.c  */
#line 700 "go.y"
    {
		markdcl();
	}
    break;

  case 77:

/* Line 1806 of yacc.c  */
#line 704 "go.y"
    {
		(yyval.node) = (yyvsp[(3) - (3)].node);
		popdcl();
	}
    break;

  case 78:

/* Line 1806 of yacc.c  */
#line 711 "go.y"
    {
		// test
		(yyval.node) = nod(OIF, N, N);
		(yyval.node)->ntest = (yyvsp[(1) - (1)].node);
	}
    break;

  case 79:

/* Line 1806 of yacc.c  */
#line 717 "go.y"
    {
		// init ; test
		(yyval.node) = nod(OIF, N, N);
		if((yyvsp[(1) - (3)].node) != N)
			(yyval.node)->ninit = list1((yyvsp[(1) - (3)].node));
		(yyval.node)->ntest = (yyvsp[(3) - (3)].node);
	}
    break;

  case 80:

/* Line 1806 of yacc.c  */
#line 728 "go.y"
    {
		markdcl();
	}
    break;

  case 81:

/* Line 1806 of yacc.c  */
#line 732 "go.y"
    {
		if((yyvsp[(3) - (3)].node)->ntest == N)
			yyerror("missing condition in if statement");
	}
    break;

  case 82:

/* Line 1806 of yacc.c  */
#line 737 "go.y"
    {
		(yyvsp[(3) - (5)].node)->nbody = (yyvsp[(5) - (5)].list);
	}
    break;

  case 83:

/* Line 1806 of yacc.c  */
#line 741 "go.y"
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

  case 84:

/* Line 1806 of yacc.c  */
#line 758 "go.y"
    {
		markdcl();
	}
    break;

  case 85:

/* Line 1806 of yacc.c  */
#line 762 "go.y"
    {
		if((yyvsp[(4) - (5)].node)->ntest == N)
			yyerror("missing condition in if statement");
		(yyvsp[(4) - (5)].node)->nbody = (yyvsp[(5) - (5)].list);
		(yyval.list) = list1((yyvsp[(4) - (5)].node));
	}
    break;

  case 86:

/* Line 1806 of yacc.c  */
#line 770 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 87:

/* Line 1806 of yacc.c  */
#line 774 "go.y"
    {
		(yyval.list) = concat((yyvsp[(1) - (2)].list), (yyvsp[(2) - (2)].list));
	}
    break;

  case 88:

/* Line 1806 of yacc.c  */
#line 779 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 89:

/* Line 1806 of yacc.c  */
#line 783 "go.y"
    {
		NodeList *node;
		
		node = mal(sizeof *node);
		node->n = (yyvsp[(2) - (2)].node);
		node->end = node;
		(yyval.list) = node;
	}
    break;

  case 90:

/* Line 1806 of yacc.c  */
#line 794 "go.y"
    {
		markdcl();
	}
    break;

  case 91:

/* Line 1806 of yacc.c  */
#line 798 "go.y"
    {
		Node *n;
		n = (yyvsp[(3) - (3)].node)->ntest;
		if(n != N && n->op != OTYPESW)
			n = N;
		typesw = nod(OXXX, typesw, n);
	}
    break;

  case 92:

/* Line 1806 of yacc.c  */
#line 806 "go.y"
    {
		(yyval.node) = (yyvsp[(3) - (7)].node);
		(yyval.node)->op = OSWITCH;
		(yyval.node)->list = (yyvsp[(6) - (7)].list);
		typesw = typesw->left;
		popdcl();
	}
    break;

  case 93:

/* Line 1806 of yacc.c  */
#line 816 "go.y"
    {
		typesw = nod(OXXX, typesw, N);
	}
    break;

  case 94:

/* Line 1806 of yacc.c  */
#line 820 "go.y"
    {
		(yyval.node) = nod(OSELECT, N, N);
		(yyval.node)->lineno = typesw->lineno;
		(yyval.node)->list = (yyvsp[(4) - (5)].list);
		typesw = typesw->left;
	}
    break;

  case 96:

/* Line 1806 of yacc.c  */
#line 833 "go.y"
    {
		(yyval.node) = nod(OOROR, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 97:

/* Line 1806 of yacc.c  */
#line 837 "go.y"
    {
		(yyval.node) = nod(OANDAND, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 98:

/* Line 1806 of yacc.c  */
#line 841 "go.y"
    {
		(yyval.node) = nod(OEQ, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 99:

/* Line 1806 of yacc.c  */
#line 845 "go.y"
    {
		(yyval.node) = nod(ONE, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 100:

/* Line 1806 of yacc.c  */
#line 849 "go.y"
    {
		(yyval.node) = nod(OLT, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 101:

/* Line 1806 of yacc.c  */
#line 853 "go.y"
    {
		(yyval.node) = nod(OLE, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 102:

/* Line 1806 of yacc.c  */
#line 857 "go.y"
    {
		(yyval.node) = nod(OGE, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 103:

/* Line 1806 of yacc.c  */
#line 861 "go.y"
    {
		(yyval.node) = nod(OGT, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 104:

/* Line 1806 of yacc.c  */
#line 865 "go.y"
    {
		(yyval.node) = nod(OADD, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 105:

/* Line 1806 of yacc.c  */
#line 869 "go.y"
    {
		(yyval.node) = nod(OSUB, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 106:

/* Line 1806 of yacc.c  */
#line 873 "go.y"
    {
		(yyval.node) = nod(OOR, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 107:

/* Line 1806 of yacc.c  */
#line 877 "go.y"
    {
		(yyval.node) = nod(OXOR, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 108:

/* Line 1806 of yacc.c  */
#line 881 "go.y"
    {
		(yyval.node) = nod(OMUL, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 109:

/* Line 1806 of yacc.c  */
#line 885 "go.y"
    {
		(yyval.node) = nod(ODIV, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 110:

/* Line 1806 of yacc.c  */
#line 889 "go.y"
    {
		(yyval.node) = nod(OMOD, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 111:

/* Line 1806 of yacc.c  */
#line 893 "go.y"
    {
		(yyval.node) = nod(OAND, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 112:

/* Line 1806 of yacc.c  */
#line 897 "go.y"
    {
		(yyval.node) = nod(OANDNOT, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 113:

/* Line 1806 of yacc.c  */
#line 901 "go.y"
    {
		(yyval.node) = nod(OLSH, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 114:

/* Line 1806 of yacc.c  */
#line 905 "go.y"
    {
		(yyval.node) = nod(ORSH, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 115:

/* Line 1806 of yacc.c  */
#line 910 "go.y"
    {
		(yyval.node) = nod(OSEND, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 117:

/* Line 1806 of yacc.c  */
#line 917 "go.y"
    {
		(yyval.node) = nod(OIND, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 118:

/* Line 1806 of yacc.c  */
#line 921 "go.y"
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

  case 119:

/* Line 1806 of yacc.c  */
#line 932 "go.y"
    {
		(yyval.node) = nod(OPLUS, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 120:

/* Line 1806 of yacc.c  */
#line 936 "go.y"
    {
		(yyval.node) = nod(OMINUS, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 121:

/* Line 1806 of yacc.c  */
#line 940 "go.y"
    {
		(yyval.node) = nod(ONOT, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 122:

/* Line 1806 of yacc.c  */
#line 944 "go.y"
    {
		yyerror("the bitwise complement operator is ^");
		(yyval.node) = nod(OCOM, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 123:

/* Line 1806 of yacc.c  */
#line 949 "go.y"
    {
		(yyval.node) = nod(OCOM, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 124:

/* Line 1806 of yacc.c  */
#line 953 "go.y"
    {
		(yyval.node) = nod(ORECV, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 125:

/* Line 1806 of yacc.c  */
#line 963 "go.y"
    {
		(yyval.node) = nod(OCALL, (yyvsp[(1) - (3)].node), N);
	}
    break;

  case 126:

/* Line 1806 of yacc.c  */
#line 967 "go.y"
    {
		(yyval.node) = nod(OCALL, (yyvsp[(1) - (5)].node), N);
		(yyval.node)->list = (yyvsp[(3) - (5)].list);
	}
    break;

  case 127:

/* Line 1806 of yacc.c  */
#line 972 "go.y"
    {
		(yyval.node) = nod(OCALL, (yyvsp[(1) - (6)].node), N);
		(yyval.node)->list = (yyvsp[(3) - (6)].list);
		(yyval.node)->isddd = 1;
	}
    break;

  case 128:

/* Line 1806 of yacc.c  */
#line 980 "go.y"
    {
		(yyval.node) = nodlit((yyvsp[(1) - (1)].val));
	}
    break;

  case 130:

/* Line 1806 of yacc.c  */
#line 985 "go.y"
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

  case 131:

/* Line 1806 of yacc.c  */
#line 996 "go.y"
    {
		(yyval.node) = nod(ODOTTYPE, (yyvsp[(1) - (5)].node), (yyvsp[(4) - (5)].node));
	}
    break;

  case 132:

/* Line 1806 of yacc.c  */
#line 1000 "go.y"
    {
		(yyval.node) = nod(OTYPESW, N, (yyvsp[(1) - (5)].node));
	}
    break;

  case 133:

/* Line 1806 of yacc.c  */
#line 1004 "go.y"
    {
		(yyval.node) = nod(OINDEX, (yyvsp[(1) - (4)].node), (yyvsp[(3) - (4)].node));
	}
    break;

  case 134:

/* Line 1806 of yacc.c  */
#line 1008 "go.y"
    {
		(yyval.node) = nod(OSLICE, (yyvsp[(1) - (6)].node), nod(OKEY, (yyvsp[(3) - (6)].node), (yyvsp[(5) - (6)].node)));
	}
    break;

  case 135:

/* Line 1806 of yacc.c  */
#line 1012 "go.y"
    {
		if((yyvsp[(5) - (8)].node) == N)
			yyerror("middle index required in 3-index slice");
		if((yyvsp[(7) - (8)].node) == N)
			yyerror("final index required in 3-index slice");
		(yyval.node) = nod(OSLICE3, (yyvsp[(1) - (8)].node), nod(OKEY, (yyvsp[(3) - (8)].node), nod(OKEY, (yyvsp[(5) - (8)].node), (yyvsp[(7) - (8)].node))));
	}
    break;

  case 137:

/* Line 1806 of yacc.c  */
#line 1021 "go.y"
    {
		// conversion
		(yyval.node) = nod(OCALL, (yyvsp[(1) - (5)].node), N);
		(yyval.node)->list = list1((yyvsp[(3) - (5)].node));
	}
    break;

  case 138:

/* Line 1806 of yacc.c  */
#line 1027 "go.y"
    {
		(yyval.node) = (yyvsp[(3) - (5)].node);
		(yyval.node)->right = (yyvsp[(1) - (5)].node);
		(yyval.node)->list = (yyvsp[(4) - (5)].list);
		fixlbrace((yyvsp[(2) - (5)].i));
	}
    break;

  case 139:

/* Line 1806 of yacc.c  */
#line 1034 "go.y"
    {
		(yyval.node) = (yyvsp[(3) - (5)].node);
		(yyval.node)->right = (yyvsp[(1) - (5)].node);
		(yyval.node)->list = (yyvsp[(4) - (5)].list);
	}
    break;

  case 140:

/* Line 1806 of yacc.c  */
#line 1040 "go.y"
    {
		yyerror("cannot parenthesize type in composite literal");
		(yyval.node) = (yyvsp[(5) - (7)].node);
		(yyval.node)->right = (yyvsp[(2) - (7)].node);
		(yyval.node)->list = (yyvsp[(6) - (7)].list);
	}
    break;

  case 142:

/* Line 1806 of yacc.c  */
#line 1049 "go.y"
    {
		// composite expression.
		// make node early so we get the right line number.
		(yyval.node) = nod(OCOMPLIT, N, N);
	}
    break;

  case 143:

/* Line 1806 of yacc.c  */
#line 1057 "go.y"
    {
		(yyval.node) = nod(OKEY, (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node));
	}
    break;

  case 144:

/* Line 1806 of yacc.c  */
#line 1063 "go.y"
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

  case 145:

/* Line 1806 of yacc.c  */
#line 1080 "go.y"
    {
		(yyval.node) = (yyvsp[(2) - (4)].node);
		(yyval.node)->list = (yyvsp[(3) - (4)].list);
	}
    break;

  case 147:

/* Line 1806 of yacc.c  */
#line 1088 "go.y"
    {
		(yyval.node) = (yyvsp[(2) - (4)].node);
		(yyval.node)->list = (yyvsp[(3) - (4)].list);
	}
    break;

  case 149:

/* Line 1806 of yacc.c  */
#line 1096 "go.y"
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

  case 153:

/* Line 1806 of yacc.c  */
#line 1122 "go.y"
    {
		(yyval.i) = LBODY;
	}
    break;

  case 154:

/* Line 1806 of yacc.c  */
#line 1126 "go.y"
    {
		(yyval.i) = '{';
	}
    break;

  case 155:

/* Line 1806 of yacc.c  */
#line 1137 "go.y"
    {
		if((yyvsp[(1) - (1)].sym) == S)
			(yyval.node) = N;
		else
			(yyval.node) = newname((yyvsp[(1) - (1)].sym));
	}
    break;

  case 156:

/* Line 1806 of yacc.c  */
#line 1146 "go.y"
    {
		(yyval.node) = dclname((yyvsp[(1) - (1)].sym));
	}
    break;

  case 157:

/* Line 1806 of yacc.c  */
#line 1151 "go.y"
    {
		(yyval.node) = N;
	}
    break;

  case 159:

/* Line 1806 of yacc.c  */
#line 1158 "go.y"
    {
		(yyval.sym) = (yyvsp[(1) - (1)].sym);
		// during imports, unqualified non-exported identifiers are from builtinpkg
		if(importpkg != nil && !exportname((yyvsp[(1) - (1)].sym)->name))
			(yyval.sym) = pkglookup((yyvsp[(1) - (1)].sym)->name, builtinpkg);
	}
    break;

  case 161:

/* Line 1806 of yacc.c  */
#line 1166 "go.y"
    {
		(yyval.sym) = S;
	}
    break;

  case 162:

/* Line 1806 of yacc.c  */
#line 1172 "go.y"
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

  case 163:

/* Line 1806 of yacc.c  */
#line 1185 "go.y"
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

  case 164:

/* Line 1806 of yacc.c  */
#line 1200 "go.y"
    {
		(yyval.node) = oldname((yyvsp[(1) - (1)].sym));
		if((yyval.node)->pack != N)
			(yyval.node)->pack->used = 1;
	}
    break;

  case 166:

/* Line 1806 of yacc.c  */
#line 1220 "go.y"
    {
		yyerror("final argument in variadic function missing type");
		(yyval.node) = nod(ODDD, typenod(typ(TINTER)), N);
	}
    break;

  case 167:

/* Line 1806 of yacc.c  */
#line 1225 "go.y"
    {
		(yyval.node) = nod(ODDD, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 173:

/* Line 1806 of yacc.c  */
#line 1236 "go.y"
    {
		(yyval.node) = nod(OTPAREN, (yyvsp[(2) - (3)].node), N);
	}
    break;

  case 177:

/* Line 1806 of yacc.c  */
#line 1245 "go.y"
    {
		(yyval.node) = nod(OIND, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 182:

/* Line 1806 of yacc.c  */
#line 1255 "go.y"
    {
		(yyval.node) = nod(OTPAREN, (yyvsp[(2) - (3)].node), N);
	}
    break;

  case 192:

/* Line 1806 of yacc.c  */
#line 1276 "go.y"
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

  case 193:

/* Line 1806 of yacc.c  */
#line 1289 "go.y"
    {
		(yyval.node) = nod(OTARRAY, (yyvsp[(2) - (4)].node), (yyvsp[(4) - (4)].node));
	}
    break;

  case 194:

/* Line 1806 of yacc.c  */
#line 1293 "go.y"
    {
		// array literal of nelem
		(yyval.node) = nod(OTARRAY, nod(ODDD, N, N), (yyvsp[(4) - (4)].node));
	}
    break;

  case 195:

/* Line 1806 of yacc.c  */
#line 1298 "go.y"
    {
		(yyval.node) = nod(OTCHAN, (yyvsp[(2) - (2)].node), N);
		(yyval.node)->etype = Cboth;
	}
    break;

  case 196:

/* Line 1806 of yacc.c  */
#line 1303 "go.y"
    {
		(yyval.node) = nod(OTCHAN, (yyvsp[(3) - (3)].node), N);
		(yyval.node)->etype = Csend;
	}
    break;

  case 197:

/* Line 1806 of yacc.c  */
#line 1308 "go.y"
    {
		(yyval.node) = nod(OTMAP, (yyvsp[(3) - (5)].node), (yyvsp[(5) - (5)].node));
	}
    break;

  case 200:

/* Line 1806 of yacc.c  */
#line 1316 "go.y"
    {
		(yyval.node) = nod(OIND, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 201:

/* Line 1806 of yacc.c  */
#line 1322 "go.y"
    {
		(yyval.node) = nod(OTCHAN, (yyvsp[(3) - (3)].node), N);
		(yyval.node)->etype = Crecv;
	}
    break;

  case 202:

/* Line 1806 of yacc.c  */
#line 1329 "go.y"
    {
		(yyval.node) = nod(OTSTRUCT, N, N);
		(yyval.node)->list = (yyvsp[(3) - (5)].list);
		fixlbrace((yyvsp[(2) - (5)].i));
	}
    break;

  case 203:

/* Line 1806 of yacc.c  */
#line 1335 "go.y"
    {
		(yyval.node) = nod(OTSTRUCT, N, N);
		fixlbrace((yyvsp[(2) - (3)].i));
	}
    break;

  case 204:

/* Line 1806 of yacc.c  */
#line 1342 "go.y"
    {
		(yyval.node) = nod(OTINTER, N, N);
		(yyval.node)->list = (yyvsp[(3) - (5)].list);
		fixlbrace((yyvsp[(2) - (5)].i));
	}
    break;

  case 205:

/* Line 1806 of yacc.c  */
#line 1348 "go.y"
    {
		(yyval.node) = nod(OTINTER, N, N);
		fixlbrace((yyvsp[(2) - (3)].i));
	}
    break;

  case 206:

/* Line 1806 of yacc.c  */
#line 1359 "go.y"
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

  case 207:

/* Line 1806 of yacc.c  */
#line 1373 "go.y"
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

  case 208:

/* Line 1806 of yacc.c  */
#line 1402 "go.y"
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

  case 209:

/* Line 1806 of yacc.c  */
#line 1442 "go.y"
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

  case 210:

/* Line 1806 of yacc.c  */
#line 1467 "go.y"
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

  case 211:

/* Line 1806 of yacc.c  */
#line 1485 "go.y"
    {
		(yyvsp[(3) - (5)].list) = checkarglist((yyvsp[(3) - (5)].list), 1);
		(yyval.node) = nod(OTFUNC, N, N);
		(yyval.node)->list = (yyvsp[(3) - (5)].list);
		(yyval.node)->rlist = (yyvsp[(5) - (5)].list);
	}
    break;

  case 212:

/* Line 1806 of yacc.c  */
#line 1493 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 213:

/* Line 1806 of yacc.c  */
#line 1497 "go.y"
    {
		(yyval.list) = (yyvsp[(2) - (3)].list);
		if((yyval.list) == nil)
			(yyval.list) = list1(nod(OEMPTY, N, N));
	}
    break;

  case 214:

/* Line 1806 of yacc.c  */
#line 1505 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 215:

/* Line 1806 of yacc.c  */
#line 1509 "go.y"
    {
		(yyval.list) = list1(nod(ODCLFIELD, N, (yyvsp[(1) - (1)].node)));
	}
    break;

  case 216:

/* Line 1806 of yacc.c  */
#line 1513 "go.y"
    {
		(yyvsp[(2) - (3)].list) = checkarglist((yyvsp[(2) - (3)].list), 0);
		(yyval.list) = (yyvsp[(2) - (3)].list);
	}
    break;

  case 217:

/* Line 1806 of yacc.c  */
#line 1520 "go.y"
    {
		closurehdr((yyvsp[(1) - (1)].node));
	}
    break;

  case 218:

/* Line 1806 of yacc.c  */
#line 1526 "go.y"
    {
		(yyval.node) = closurebody((yyvsp[(3) - (4)].list));
		fixlbrace((yyvsp[(2) - (4)].i));
	}
    break;

  case 219:

/* Line 1806 of yacc.c  */
#line 1531 "go.y"
    {
		(yyval.node) = closurebody(nil);
	}
    break;

  case 220:

/* Line 1806 of yacc.c  */
#line 1542 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 221:

/* Line 1806 of yacc.c  */
#line 1546 "go.y"
    {
		(yyval.list) = concat((yyvsp[(1) - (3)].list), (yyvsp[(2) - (3)].list));
		if(nsyntaxerrors == 0)
			testdclstack();
		nointerface = 0;
		noescape = 0;
	}
    break;

  case 223:

/* Line 1806 of yacc.c  */
#line 1564 "go.y"
    {
		(yyval.list) = concat((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].list));
	}
    break;

  case 225:

/* Line 1806 of yacc.c  */
#line 1571 "go.y"
    {
		(yyval.list) = concat((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].list));
	}
    break;

  case 226:

/* Line 1806 of yacc.c  */
#line 1577 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 227:

/* Line 1806 of yacc.c  */
#line 1581 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].node));
	}
    break;

  case 229:

/* Line 1806 of yacc.c  */
#line 1588 "go.y"
    {
		(yyval.list) = concat((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].list));
	}
    break;

  case 230:

/* Line 1806 of yacc.c  */
#line 1594 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 231:

/* Line 1806 of yacc.c  */
#line 1598 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].node));
	}
    break;

  case 232:

/* Line 1806 of yacc.c  */
#line 1604 "go.y"
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

  case 233:

/* Line 1806 of yacc.c  */
#line 1627 "go.y"
    {
		(yyvsp[(1) - (2)].node)->val = (yyvsp[(2) - (2)].val);
		(yyval.list) = list1((yyvsp[(1) - (2)].node));
	}
    break;

  case 234:

/* Line 1806 of yacc.c  */
#line 1632 "go.y"
    {
		(yyvsp[(2) - (4)].node)->val = (yyvsp[(4) - (4)].val);
		(yyval.list) = list1((yyvsp[(2) - (4)].node));
		yyerror("cannot parenthesize embedded type");
	}
    break;

  case 235:

/* Line 1806 of yacc.c  */
#line 1638 "go.y"
    {
		(yyvsp[(2) - (3)].node)->right = nod(OIND, (yyvsp[(2) - (3)].node)->right, N);
		(yyvsp[(2) - (3)].node)->val = (yyvsp[(3) - (3)].val);
		(yyval.list) = list1((yyvsp[(2) - (3)].node));
	}
    break;

  case 236:

/* Line 1806 of yacc.c  */
#line 1644 "go.y"
    {
		(yyvsp[(3) - (5)].node)->right = nod(OIND, (yyvsp[(3) - (5)].node)->right, N);
		(yyvsp[(3) - (5)].node)->val = (yyvsp[(5) - (5)].val);
		(yyval.list) = list1((yyvsp[(3) - (5)].node));
		yyerror("cannot parenthesize embedded type");
	}
    break;

  case 237:

/* Line 1806 of yacc.c  */
#line 1651 "go.y"
    {
		(yyvsp[(3) - (5)].node)->right = nod(OIND, (yyvsp[(3) - (5)].node)->right, N);
		(yyvsp[(3) - (5)].node)->val = (yyvsp[(5) - (5)].val);
		(yyval.list) = list1((yyvsp[(3) - (5)].node));
		yyerror("cannot parenthesize embedded type");
	}
    break;

  case 238:

/* Line 1806 of yacc.c  */
#line 1660 "go.y"
    {
		Node *n;

		(yyval.sym) = (yyvsp[(1) - (1)].sym);
		n = oldname((yyvsp[(1) - (1)].sym));
		if(n->pack != N)
			n->pack->used = 1;
	}
    break;

  case 239:

/* Line 1806 of yacc.c  */
#line 1669 "go.y"
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

  case 240:

/* Line 1806 of yacc.c  */
#line 1684 "go.y"
    {
		(yyval.node) = embedded((yyvsp[(1) - (1)].sym), localpkg);
	}
    break;

  case 241:

/* Line 1806 of yacc.c  */
#line 1690 "go.y"
    {
		(yyval.node) = nod(ODCLFIELD, (yyvsp[(1) - (2)].node), (yyvsp[(2) - (2)].node));
		ifacedcl((yyval.node));
	}
    break;

  case 242:

/* Line 1806 of yacc.c  */
#line 1695 "go.y"
    {
		(yyval.node) = nod(ODCLFIELD, N, oldname((yyvsp[(1) - (1)].sym)));
	}
    break;

  case 243:

/* Line 1806 of yacc.c  */
#line 1699 "go.y"
    {
		(yyval.node) = nod(ODCLFIELD, N, oldname((yyvsp[(2) - (3)].sym)));
		yyerror("cannot parenthesize embedded type");
	}
    break;

  case 244:

/* Line 1806 of yacc.c  */
#line 1706 "go.y"
    {
		// without func keyword
		(yyvsp[(2) - (4)].list) = checkarglist((yyvsp[(2) - (4)].list), 1);
		(yyval.node) = nod(OTFUNC, fakethis(), N);
		(yyval.node)->list = (yyvsp[(2) - (4)].list);
		(yyval.node)->rlist = (yyvsp[(4) - (4)].list);
	}
    break;

  case 246:

/* Line 1806 of yacc.c  */
#line 1720 "go.y"
    {
		(yyval.node) = nod(ONONAME, N, N);
		(yyval.node)->sym = (yyvsp[(1) - (2)].sym);
		(yyval.node) = nod(OKEY, (yyval.node), (yyvsp[(2) - (2)].node));
	}
    break;

  case 247:

/* Line 1806 of yacc.c  */
#line 1726 "go.y"
    {
		(yyval.node) = nod(ONONAME, N, N);
		(yyval.node)->sym = (yyvsp[(1) - (2)].sym);
		(yyval.node) = nod(OKEY, (yyval.node), (yyvsp[(2) - (2)].node));
	}
    break;

  case 249:

/* Line 1806 of yacc.c  */
#line 1735 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 250:

/* Line 1806 of yacc.c  */
#line 1739 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].node));
	}
    break;

  case 251:

/* Line 1806 of yacc.c  */
#line 1744 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 252:

/* Line 1806 of yacc.c  */
#line 1748 "go.y"
    {
		(yyval.list) = (yyvsp[(1) - (2)].list);
	}
    break;

  case 253:

/* Line 1806 of yacc.c  */
#line 1756 "go.y"
    {
		(yyval.node) = N;
	}
    break;

  case 255:

/* Line 1806 of yacc.c  */
#line 1761 "go.y"
    {
		(yyval.node) = liststmt((yyvsp[(1) - (1)].list));
	}
    break;

  case 257:

/* Line 1806 of yacc.c  */
#line 1766 "go.y"
    {
		(yyval.node) = N;
	}
    break;

  case 263:

/* Line 1806 of yacc.c  */
#line 1777 "go.y"
    {
		(yyvsp[(1) - (2)].node) = nod(OLABEL, (yyvsp[(1) - (2)].node), N);
		(yyvsp[(1) - (2)].node)->sym = dclstack;  // context, for goto restrictions
	}
    break;

  case 264:

/* Line 1806 of yacc.c  */
#line 1782 "go.y"
    {
		NodeList *l;

		(yyvsp[(1) - (4)].node)->defn = (yyvsp[(4) - (4)].node);
		l = list1((yyvsp[(1) - (4)].node));
		if((yyvsp[(4) - (4)].node))
			l = list(l, (yyvsp[(4) - (4)].node));
		(yyval.node) = liststmt(l);
	}
    break;

  case 265:

/* Line 1806 of yacc.c  */
#line 1792 "go.y"
    {
		// will be converted to OFALL
		(yyval.node) = nod(OXFALL, N, N);
	}
    break;

  case 266:

/* Line 1806 of yacc.c  */
#line 1797 "go.y"
    {
		(yyval.node) = nod(OBREAK, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 267:

/* Line 1806 of yacc.c  */
#line 1801 "go.y"
    {
		(yyval.node) = nod(OCONTINUE, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 268:

/* Line 1806 of yacc.c  */
#line 1805 "go.y"
    {
		(yyval.node) = nod(OPROC, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 269:

/* Line 1806 of yacc.c  */
#line 1809 "go.y"
    {
		(yyval.node) = nod(ODEFER, (yyvsp[(2) - (2)].node), N);
	}
    break;

  case 270:

/* Line 1806 of yacc.c  */
#line 1813 "go.y"
    {
		(yyval.node) = nod(OGOTO, (yyvsp[(2) - (2)].node), N);
		(yyval.node)->sym = dclstack;  // context, for goto restrictions
	}
    break;

  case 271:

/* Line 1806 of yacc.c  */
#line 1818 "go.y"
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

  case 272:

/* Line 1806 of yacc.c  */
#line 1837 "go.y"
    {
		(yyval.list) = nil;
		if((yyvsp[(1) - (1)].node) != N)
			(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 273:

/* Line 1806 of yacc.c  */
#line 1843 "go.y"
    {
		(yyval.list) = (yyvsp[(1) - (3)].list);
		if((yyvsp[(3) - (3)].node) != N)
			(yyval.list) = list((yyval.list), (yyvsp[(3) - (3)].node));
	}
    break;

  case 274:

/* Line 1806 of yacc.c  */
#line 1851 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 275:

/* Line 1806 of yacc.c  */
#line 1855 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].node));
	}
    break;

  case 276:

/* Line 1806 of yacc.c  */
#line 1861 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 277:

/* Line 1806 of yacc.c  */
#line 1865 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].node));
	}
    break;

  case 278:

/* Line 1806 of yacc.c  */
#line 1871 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 279:

/* Line 1806 of yacc.c  */
#line 1875 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].node));
	}
    break;

  case 280:

/* Line 1806 of yacc.c  */
#line 1881 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 281:

/* Line 1806 of yacc.c  */
#line 1885 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].node));
	}
    break;

  case 282:

/* Line 1806 of yacc.c  */
#line 1894 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 283:

/* Line 1806 of yacc.c  */
#line 1898 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 284:

/* Line 1806 of yacc.c  */
#line 1902 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].node));
	}
    break;

  case 285:

/* Line 1806 of yacc.c  */
#line 1906 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].node));
	}
    break;

  case 286:

/* Line 1806 of yacc.c  */
#line 1911 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 287:

/* Line 1806 of yacc.c  */
#line 1915 "go.y"
    {
		(yyval.list) = (yyvsp[(1) - (2)].list);
	}
    break;

  case 292:

/* Line 1806 of yacc.c  */
#line 1929 "go.y"
    {
		(yyval.node) = N;
	}
    break;

  case 294:

/* Line 1806 of yacc.c  */
#line 1935 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 296:

/* Line 1806 of yacc.c  */
#line 1941 "go.y"
    {
		(yyval.node) = N;
	}
    break;

  case 298:

/* Line 1806 of yacc.c  */
#line 1947 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 300:

/* Line 1806 of yacc.c  */
#line 1953 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 302:

/* Line 1806 of yacc.c  */
#line 1959 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 304:

/* Line 1806 of yacc.c  */
#line 1965 "go.y"
    {
		(yyval.val).ctype = CTxxx;
	}
    break;

  case 306:

/* Line 1806 of yacc.c  */
#line 1975 "go.y"
    {
		importimport((yyvsp[(2) - (4)].sym), (yyvsp[(3) - (4)].val).u.sval);
	}
    break;

  case 307:

/* Line 1806 of yacc.c  */
#line 1979 "go.y"
    {
		importvar((yyvsp[(2) - (4)].sym), (yyvsp[(3) - (4)].type));
	}
    break;

  case 308:

/* Line 1806 of yacc.c  */
#line 1983 "go.y"
    {
		importconst((yyvsp[(2) - (5)].sym), types[TIDEAL], (yyvsp[(4) - (5)].node));
	}
    break;

  case 309:

/* Line 1806 of yacc.c  */
#line 1987 "go.y"
    {
		importconst((yyvsp[(2) - (6)].sym), (yyvsp[(3) - (6)].type), (yyvsp[(5) - (6)].node));
	}
    break;

  case 310:

/* Line 1806 of yacc.c  */
#line 1991 "go.y"
    {
		importtype((yyvsp[(2) - (4)].type), (yyvsp[(3) - (4)].type));
	}
    break;

  case 311:

/* Line 1806 of yacc.c  */
#line 1995 "go.y"
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

  case 312:

/* Line 1806 of yacc.c  */
#line 2015 "go.y"
    {
		(yyval.sym) = (yyvsp[(1) - (1)].sym);
		structpkg = (yyval.sym)->pkg;
	}
    break;

  case 313:

/* Line 1806 of yacc.c  */
#line 2022 "go.y"
    {
		(yyval.type) = pkgtype((yyvsp[(1) - (1)].sym));
		importsym((yyvsp[(1) - (1)].sym), OTYPE);
	}
    break;

  case 319:

/* Line 1806 of yacc.c  */
#line 2042 "go.y"
    {
		(yyval.type) = pkgtype((yyvsp[(1) - (1)].sym));
	}
    break;

  case 320:

/* Line 1806 of yacc.c  */
#line 2046 "go.y"
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

  case 321:

/* Line 1806 of yacc.c  */
#line 2056 "go.y"
    {
		(yyval.type) = aindex(N, (yyvsp[(3) - (3)].type));
	}
    break;

  case 322:

/* Line 1806 of yacc.c  */
#line 2060 "go.y"
    {
		(yyval.type) = aindex(nodlit((yyvsp[(2) - (4)].val)), (yyvsp[(4) - (4)].type));
	}
    break;

  case 323:

/* Line 1806 of yacc.c  */
#line 2064 "go.y"
    {
		(yyval.type) = maptype((yyvsp[(3) - (5)].type), (yyvsp[(5) - (5)].type));
	}
    break;

  case 324:

/* Line 1806 of yacc.c  */
#line 2068 "go.y"
    {
		(yyval.type) = tostruct((yyvsp[(3) - (4)].list));
	}
    break;

  case 325:

/* Line 1806 of yacc.c  */
#line 2072 "go.y"
    {
		(yyval.type) = tointerface((yyvsp[(3) - (4)].list));
	}
    break;

  case 326:

/* Line 1806 of yacc.c  */
#line 2076 "go.y"
    {
		(yyval.type) = ptrto((yyvsp[(2) - (2)].type));
	}
    break;

  case 327:

/* Line 1806 of yacc.c  */
#line 2080 "go.y"
    {
		(yyval.type) = typ(TCHAN);
		(yyval.type)->type = (yyvsp[(2) - (2)].type);
		(yyval.type)->chan = Cboth;
	}
    break;

  case 328:

/* Line 1806 of yacc.c  */
#line 2086 "go.y"
    {
		(yyval.type) = typ(TCHAN);
		(yyval.type)->type = (yyvsp[(3) - (4)].type);
		(yyval.type)->chan = Cboth;
	}
    break;

  case 329:

/* Line 1806 of yacc.c  */
#line 2092 "go.y"
    {
		(yyval.type) = typ(TCHAN);
		(yyval.type)->type = (yyvsp[(3) - (3)].type);
		(yyval.type)->chan = Csend;
	}
    break;

  case 330:

/* Line 1806 of yacc.c  */
#line 2100 "go.y"
    {
		(yyval.type) = typ(TCHAN);
		(yyval.type)->type = (yyvsp[(3) - (3)].type);
		(yyval.type)->chan = Crecv;
	}
    break;

  case 331:

/* Line 1806 of yacc.c  */
#line 2108 "go.y"
    {
		(yyval.type) = functype(nil, (yyvsp[(3) - (5)].list), (yyvsp[(5) - (5)].list));
	}
    break;

  case 332:

/* Line 1806 of yacc.c  */
#line 2114 "go.y"
    {
		(yyval.node) = nod(ODCLFIELD, N, typenod((yyvsp[(2) - (3)].type)));
		if((yyvsp[(1) - (3)].sym))
			(yyval.node)->left = newname((yyvsp[(1) - (3)].sym));
		(yyval.node)->val = (yyvsp[(3) - (3)].val);
	}
    break;

  case 333:

/* Line 1806 of yacc.c  */
#line 2121 "go.y"
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

  case 334:

/* Line 1806 of yacc.c  */
#line 2137 "go.y"
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

  case 335:

/* Line 1806 of yacc.c  */
#line 2159 "go.y"
    {
		(yyval.node) = nod(ODCLFIELD, newname((yyvsp[(1) - (5)].sym)), typenod(functype(fakethis(), (yyvsp[(3) - (5)].list), (yyvsp[(5) - (5)].list))));
	}
    break;

  case 336:

/* Line 1806 of yacc.c  */
#line 2163 "go.y"
    {
		(yyval.node) = nod(ODCLFIELD, N, typenod((yyvsp[(1) - (1)].type)));
	}
    break;

  case 337:

/* Line 1806 of yacc.c  */
#line 2168 "go.y"
    {
		(yyval.list) = nil;
	}
    break;

  case 339:

/* Line 1806 of yacc.c  */
#line 2175 "go.y"
    {
		(yyval.list) = (yyvsp[(2) - (3)].list);
	}
    break;

  case 340:

/* Line 1806 of yacc.c  */
#line 2179 "go.y"
    {
		(yyval.list) = list1(nod(ODCLFIELD, N, typenod((yyvsp[(1) - (1)].type))));
	}
    break;

  case 341:

/* Line 1806 of yacc.c  */
#line 2189 "go.y"
    {
		(yyval.node) = nodlit((yyvsp[(1) - (1)].val));
	}
    break;

  case 342:

/* Line 1806 of yacc.c  */
#line 2193 "go.y"
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

  case 343:

/* Line 1806 of yacc.c  */
#line 2208 "go.y"
    {
		(yyval.node) = oldname(pkglookup((yyvsp[(1) - (1)].sym)->name, builtinpkg));
		if((yyval.node)->op != OLITERAL)
			yyerror("bad constant %S", (yyval.node)->sym);
	}
    break;

  case 345:

/* Line 1806 of yacc.c  */
#line 2217 "go.y"
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

  case 348:

/* Line 1806 of yacc.c  */
#line 2233 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 349:

/* Line 1806 of yacc.c  */
#line 2237 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].node));
	}
    break;

  case 350:

/* Line 1806 of yacc.c  */
#line 2243 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 351:

/* Line 1806 of yacc.c  */
#line 2247 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].node));
	}
    break;

  case 352:

/* Line 1806 of yacc.c  */
#line 2253 "go.y"
    {
		(yyval.list) = list1((yyvsp[(1) - (1)].node));
	}
    break;

  case 353:

/* Line 1806 of yacc.c  */
#line 2257 "go.y"
    {
		(yyval.list) = list((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].node));
	}
    break;



/* Line 1806 of yacc.c  */
#line 5481 "y.tab.c"
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
#line 2261 "go.y"


static void
fixlbrace(int lbr)
{
	// If the opening brace was an LBODY,
	// set up for another one now that we're done.
	// See comment in lex.c about loophack.
	if(lbr == LBODY)
		loophack = 1;
}


