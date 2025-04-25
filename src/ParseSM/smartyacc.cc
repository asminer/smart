/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     IDENT = 258,
     BOOLCONST = 259,
     INTCONST = 260,
     REALCONST = 261,
     STRCONST = 262,
     MODIF = 263,
     TYPE = 264,
     FORMALISM = 265,
     LPAR = 266,
     RPAR = 267,
     LBRAK = 268,
     RBRAK = 269,
     LBRACE = 270,
     RBRACE = 271,
     COMMA = 272,
     SEMI = 273,
     COLON = 274,
     POUND = 275,
     ENDPND = 276,
     DOT = 277,
     DOTDOT = 278,
     GETS = 279,
     PLUS = 280,
     MINUS = 281,
     TIMES = 282,
     DIVIDE = 283,
     MOD = 284,
     OR = 285,
     AND = 286,
     SET_DIFF = 287,
     IMPLIES = 288,
     NOT = 289,
     EQUALS = 290,
     NEQUAL = 291,
     GT = 292,
     GE = 293,
     LT = 294,
     LE = 295,
     FOR = 296,
     END = 297,
     CONVERGE = 298,
     IN = 299,
     GUESS = 300,
     NUL = 301,
     DEFAULT = 302,
     PROC = 303,
     FORALL = 304,
     EXISTS = 305,
     FUTURE = 306,
     GLOBALLY = 307,
     UNTIL = 308,
     NEXT = 309,
     TEMPORALAND = 310,
     UMINUS = 311
   };
#endif
/* Tokens.  */
#define IDENT 258
#define BOOLCONST 259
#define INTCONST 260
#define REALCONST 261
#define STRCONST 262
#define MODIF 263
#define TYPE 264
#define FORMALISM 265
#define LPAR 266
#define RPAR 267
#define LBRAK 268
#define RBRAK 269
#define LBRACE 270
#define RBRACE 271
#define COMMA 272
#define SEMI 273
#define COLON 274
#define POUND 275
#define ENDPND 276
#define DOT 277
#define DOTDOT 278
#define GETS 279
#define PLUS 280
#define MINUS 281
#define TIMES 282
#define DIVIDE 283
#define MOD 284
#define OR 285
#define AND 286
#define SET_DIFF 287
#define IMPLIES 288
#define NOT 289
#define EQUALS 290
#define NEQUAL 291
#define GT 292
#define GE 293
#define LT 294
#define LE 295
#define FOR 296
#define END 297
#define CONVERGE 298
#define IN 299
#define GUESS 300
#define NUL 301
#define DEFAULT 302
#define PROC 303
#define FORALL 304
#define EXISTS 305
#define FUTURE 306
#define GLOBALLY 307
#define UNTIL 308
#define NEXT 309
#define TEMPORALAND 310
#define UMINUS 311




/* Copy the first part of user declarations.  */
#line 11 "ParseSM/smartyacc.yy"


#include "ParseSM/compile.h" // compile-time functionality 



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 17 "ParseSM/smartyacc.yy"
{
  char* name;
  const type* Type_ID;
  int count;
  expr* Expr;
  parser_list* List;
  option* Option;
  symbol* Symbol;
  shared_object* other;
}
/* Line 193 of yacc.c.  */
#line 225 "ParseSM/smartyacc.cc"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 238 "ParseSM/smartyacc.cc"

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
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
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
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
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
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
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
  yytype_int16 yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

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

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  77
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   611

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  57
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  54
/* YYNRULES -- Number of rules.  */
#define YYNRULES  143
/* YYNRULES -- Number of states.  */
#define YYNSTATES  272

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   311

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
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
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,     8,    13,    18,    20,    22,    24,
      26,    29,    31,    35,    37,    42,    44,    48,    52,    57,
      62,    68,    71,    76,    79,    81,    85,    87,    92,    96,
      99,   102,   104,   106,   110,   114,   116,   118,   122,   128,
     131,   136,   142,   148,   153,   158,   164,   168,   175,   181,
     184,   187,   189,   192,   196,   198,   203,   207,   212,   214,
     217,   219,   221,   223,   227,   231,   235,   237,   239,   243,
     245,   247,   251,   255,   259,   263,   267,   271,   275,   278,
     281,   284,   287,   290,   294,   298,   302,   304,   306,   310,
     314,   318,   320,   322,   324,   326,   328,   330,   334,   337,
     340,   345,   350,   354,   358,   362,   364,   366,   368,   370,
     372,   374,   377,   380,   384,   389,   394,   400,   402,   405,
     408,   410,   413,   416,   419,   423,   425,   428,   433,   438,
     442,   447,   451,   453,   457,   461,   463,   465,   467,   471,
     474,   478,   480,   484
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      58,     0,    -1,    58,    59,    -1,    59,    -1,    61,    15,
      58,    16,    -1,    62,    15,    58,    16,    -1,    74,    -1,
      75,    -1,    78,    -1,    63,    -1,    84,    18,    -1,    18,
      -1,    60,    17,     3,    -1,     3,    -1,    41,    11,    67,
      12,    -1,    43,    -1,    64,    96,    21,    -1,    64,     3,
      21,    -1,    64,    25,    60,    21,    -1,    64,    26,    60,
      21,    -1,    65,    66,    20,    16,    21,    -1,    20,     3,
      -1,    64,     3,    15,    21,    -1,    66,    63,    -1,    63,
      -1,    67,    17,    68,    -1,    68,    -1,    69,     3,    44,
      71,    -1,    48,     8,     9,    -1,    48,     9,    -1,     8,
       9,    -1,     9,    -1,    10,    -1,    15,    72,    16,    -1,
      72,    17,    73,    -1,    73,    -1,    84,    -1,    84,    23,
      84,    -1,    84,    23,    84,    23,    84,    -1,    76,    18,
      -1,    76,    24,    84,    18,    -1,    69,     3,    24,    84,
      18,    -1,    69,     3,    45,    84,    18,    -1,    77,    24,
      84,    18,    -1,    77,    45,    84,    18,    -1,    69,     3,
      11,   100,    12,    -1,    69,     3,   102,    -1,    79,    24,
      15,    80,    16,    18,    -1,    70,     3,    11,   100,    12,
      -1,    70,     3,    -1,    80,    81,    -1,    81,    -1,    99,
      18,    -1,    69,    82,    18,    -1,    75,    -1,    61,    15,
      80,    16,    -1,    82,    17,     3,    -1,    82,    17,     3,
     102,    -1,     3,    -1,     3,   102,    -1,    84,    -1,    93,
      -1,    85,    -1,    84,    33,    84,    -1,    84,    32,    84,
      -1,    85,    30,    86,    -1,    86,    -1,    87,    -1,    87,
      31,    88,    -1,    88,    -1,    89,    -1,    88,    35,    88,
      -1,    88,    36,    88,    -1,    88,    37,    88,    -1,    88,
      38,    88,    -1,    88,    39,    88,    -1,    88,    40,    88,
      -1,    88,    55,    88,    -1,    49,    88,    -1,    50,    88,
      -1,    54,    88,    -1,    51,    88,    -1,    52,    88,    -1,
      88,    53,    88,    -1,    89,    25,    90,    -1,    89,    26,
      90,    -1,    90,    -1,    91,    -1,    90,    29,    90,    -1,
      91,    27,    92,    -1,    91,    28,    92,    -1,    92,    -1,
      46,    -1,    95,    -1,    71,    -1,    97,    -1,    99,    -1,
      11,    84,    12,    -1,    34,    92,    -1,    26,    92,    -1,
      69,    11,    84,    12,    -1,    15,    94,    18,    16,    -1,
      93,    19,    84,    -1,    84,    19,    84,    -1,    94,    18,
      92,    -1,    92,    -1,     4,    -1,     5,    -1,     6,    -1,
       7,    -1,    95,    -1,    26,    96,    -1,    34,    96,    -1,
      98,    22,     3,    -1,     3,   103,    22,     3,    -1,    98,
      22,     3,   103,    -1,     3,   103,    22,     3,   103,    -1,
       3,    -1,     3,   105,    -1,     3,   108,    -1,     3,    -1,
       3,   103,    -1,     3,   105,    -1,     3,   108,    -1,   100,
      17,   101,    -1,   101,    -1,    69,     3,    -1,    69,     3,
      24,    83,    -1,   102,    13,     3,    14,    -1,    13,     3,
      14,    -1,   103,    13,   104,    14,    -1,    13,   104,    14,
      -1,    84,    -1,    11,   106,    12,    -1,   106,    17,   107,
      -1,   107,    -1,    83,    -1,    47,    -1,    11,   109,    12,
      -1,    11,    12,    -1,   109,    17,   110,    -1,   110,    -1,
       3,    24,    83,    -1,     3,    24,    47,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    87,    87,    92,   100,   105,   110,   115,   120,   125,
     130,   135,   143,   148,   156,   164,   172,   177,   182,   187,
     192,   201,   209,   217,   222,   231,   236,   244,   252,   257,
     262,   267,   275,   283,   291,   296,   304,   309,   314,   328,
     336,   341,   346,   351,   356,   370,   378,   392,   401,   406,
     414,   419,   427,   432,   437,   442,   450,   455,   460,   465,
     480,   485,   493,   498,   503,   511,   516,   524,   532,   537,
     547,   552,   557,   562,   567,   572,   577,   582,   587,   592,
     597,   602,   607,   612,   620,   625,   630,   638,   643,   651,
     656,   661,   669,   674,   679,   684,   689,   694,   699,   704,
     709,   714,   723,   728,   737,   742,   752,   757,   762,   767,
     775,   780,   785,   800,   805,   810,   815,   823,   828,   833,
     847,   852,   857,   862,   876,   881,   889,   894,   902,   907,
     916,   921,   929,   937,   945,   950,   958,   963,   972,   977,
     985,   990,   998,  1003
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "IDENT", "BOOLCONST", "INTCONST",
  "REALCONST", "STRCONST", "MODIF", "TYPE", "FORMALISM", "LPAR", "RPAR",
  "LBRAK", "RBRAK", "LBRACE", "RBRACE", "COMMA", "SEMI", "COLON", "POUND",
  "ENDPND", "DOT", "DOTDOT", "GETS", "PLUS", "MINUS", "TIMES", "DIVIDE",
  "MOD", "OR", "AND", "SET_DIFF", "IMPLIES", "NOT", "EQUALS", "NEQUAL",
  "GT", "GE", "LT", "LE", "FOR", "END", "CONVERGE", "IN", "GUESS", "NUL",
  "DEFAULT", "PROC", "FORALL", "EXISTS", "FUTURE", "GLOBALLY", "UNTIL",
  "NEXT", "TEMPORALAND", "UMINUS", "$accept", "statements", "statement",
  "idlist", "for_header", "converge", "opt_stmt", "opt_header",
  "opt_begin", "opt_stmts", "iterators", "iterator", "type", "model",
  "set_expr", "set_elems", "set_elem", "decl_stmt", "defn_stmt",
  "func_header", "array_header", "model_decl", "model_header",
  "model_stmts", "model_stmt", "model_var_list", "expr", "arith",
  "disjunct", "doneconj", "conjunct", "logic", "summation", "doneproduct",
  "product", "term", "aggexpr", "seqexpr", "value", "const_expr",
  "model_function_call", "model_call", "function_call", "formal_params",
  "formal_param", "formal_indexes", "indexes", "index", "passed_params",
  "pos_params", "pos_param", "named_params", "named_list", "named_param", 0
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
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    57,    58,    58,    59,    59,    59,    59,    59,    59,
      59,    59,    60,    60,    61,    62,    63,    63,    63,    63,
      63,    64,    65,    66,    66,    67,    67,    68,    69,    69,
      69,    69,    70,    71,    72,    72,    73,    73,    73,    74,
      75,    75,    75,    75,    75,    76,    77,    78,    79,    79,
      80,    80,    81,    81,    81,    81,    82,    82,    82,    82,
      83,    83,    84,    84,    84,    85,    85,    86,    87,    87,
      88,    88,    88,    88,    88,    88,    88,    88,    88,    88,
      88,    88,    88,    88,    89,    89,    89,    90,    90,    91,
      91,    91,    92,    92,    92,    92,    92,    92,    92,    92,
      92,    92,    93,    93,    94,    94,    95,    95,    95,    95,
      96,    96,    96,    97,    97,    97,    97,    98,    98,    98,
      99,    99,    99,    99,   100,   100,   101,   101,   102,   102,
     103,   103,   104,   105,   106,   106,   107,   107,   108,   108,
     109,   109,   110,   110
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     1,     4,     4,     1,     1,     1,     1,
       2,     1,     3,     1,     4,     1,     3,     3,     4,     4,
       5,     2,     4,     2,     1,     3,     1,     4,     3,     2,
       2,     1,     1,     3,     3,     1,     1,     3,     5,     2,
       4,     5,     5,     4,     4,     5,     3,     6,     5,     2,
       2,     1,     2,     3,     1,     4,     3,     4,     1,     2,
       1,     1,     1,     3,     3,     3,     1,     1,     3,     1,
       1,     3,     3,     3,     3,     3,     3,     3,     2,     2,
       2,     2,     2,     3,     3,     3,     1,     1,     3,     3,
       3,     1,     1,     1,     1,     1,     1,     3,     2,     2,
       4,     4,     3,     3,     3,     1,     1,     1,     1,     1,
       1,     2,     2,     3,     4,     4,     5,     1,     2,     2,
       1,     2,     2,     2,     3,     1,     2,     4,     4,     3,
       4,     3,     1,     3,     3,     1,     1,     1,     3,     2,
       3,     1,     3,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,   120,   106,   107,   108,   109,     0,    31,    32,     0,
       0,    11,     0,     0,     0,     0,    15,    92,     0,     0,
       0,     0,     0,     0,     0,     3,     0,     0,     9,     0,
       0,     0,     0,    94,     6,     7,     0,     0,     8,     0,
       0,    62,    66,    67,    69,    70,    86,    87,    91,    93,
      95,     0,    96,     0,     0,   121,   122,   123,    30,     0,
       0,     0,    35,    36,    91,     0,    21,    99,    98,     0,
       0,    29,    78,    79,    81,    82,    80,     1,     2,     0,
       0,     0,     0,     0,     0,   110,     0,    24,     0,     0,
       0,    49,    39,     0,     0,     0,     0,    10,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   120,   139,   137,   136,
      60,    61,     0,   135,     0,   141,   132,     0,     0,     0,
      97,    33,     0,     0,     0,     0,    26,     0,    28,     0,
       0,     0,    17,    13,     0,     0,     0,   111,   112,    16,
       0,    23,     0,     0,     0,     0,    46,     0,     0,     0,
       0,     0,     0,    64,    63,    65,    68,    71,    72,    73,
      74,    75,    76,    83,    77,    84,    85,    88,    89,    90,
     113,     0,     0,     0,   133,     0,   138,     0,   131,     0,
     114,    34,    37,   101,   104,    14,     0,     0,     4,     5,
      22,     0,    18,    19,     0,     0,     0,   125,     0,     0,
       0,     0,   100,     0,    40,    43,    44,   120,     0,     0,
      54,     0,     0,    51,     0,   115,   143,   142,   103,   102,
     134,     0,   140,   130,   116,     0,    25,     0,    12,    20,
     126,    45,     0,   129,    41,    42,     0,    48,   121,   122,
     123,     0,    58,     0,     0,    50,    52,    38,     0,    27,
       0,   124,   128,     0,    46,     0,    53,    47,   127,    55,
      56,    57
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    24,    25,   144,    26,    27,    28,    29,    30,    88,
     135,   136,    59,    32,    33,    61,    62,    34,    35,    36,
      37,    38,    39,   222,   223,   253,   119,    40,    41,    42,
      43,    44,    45,    46,    47,    48,   121,    65,    49,   147,
      50,    51,    52,   206,   207,   156,    55,   127,    56,   122,
     123,    57,   124,   125
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -232
static const yytype_int16 yypact[] =
{
     332,   172,  -232,  -232,  -232,  -232,    19,  -232,  -232,   540,
     540,  -232,    91,   553,   553,    55,  -232,  -232,   145,   540,
     540,   540,   540,   540,   161,  -232,   148,   165,  -232,   577,
     122,    73,   188,  -232,  -232,  -232,    65,    20,  -232,   195,
      63,   178,  -232,   190,   220,   180,   216,   288,  -232,  -232,
    -232,   225,  -232,   384,   540,    41,   227,   242,  -232,   261,
       8,   296,  -232,   191,   263,   291,  -232,  -232,  -232,    10,
     290,  -232,   251,   251,  -232,  -232,  -232,  -232,  -232,   332,
     332,    77,   314,   420,   194,  -232,   299,  -232,   302,   173,
     540,   316,  -232,   540,   540,   540,   318,  -232,   540,   540,
     540,   540,   540,   540,   540,   540,   540,   540,   540,   540,
     553,   553,   553,   553,   553,   343,   281,  -232,  -232,  -232,
     184,   329,    96,  -232,   139,  -232,   286,   335,   540,   348,
    -232,  -232,   540,   540,    27,   249,  -232,   350,  -232,   228,
     280,   333,  -232,  -232,    97,   194,   105,  -232,  -232,  -232,
      13,  -232,    10,   352,   540,   540,   344,    45,    10,   127,
     160,   208,    14,  -232,  -232,  -232,   220,    64,    64,    88,
      88,    88,    88,   303,   251,   216,   216,  -232,  -232,  -232,
     346,   436,   540,   540,  -232,   488,  -232,   357,  -232,   347,
     346,  -232,   278,  -232,  -232,  -232,    10,   319,  -232,  -232,
    -232,   359,  -232,  -232,   349,   361,   253,  -232,   351,   235,
     275,   364,  -232,   285,  -232,  -232,  -232,   177,   353,   366,
    -232,   355,   107,  -232,   354,   358,  -232,  -232,   286,   286,
    -232,   370,  -232,  -232,   358,   540,  -232,   362,  -232,  -232,
     373,  -232,    10,  -232,  -232,  -232,   360,  -232,   358,  -232,
    -232,    14,   173,   307,   367,  -232,  -232,   286,   540,  -232,
     540,  -232,  -232,   141,   212,   395,  -232,  -232,  -232,  -232,
     363,   344
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -232,   265,   -11,   317,  -158,  -232,   -16,  -232,  -232,  -232,
    -232,   205,     0,  -232,   166,  -232,   270,  -232,  -154,  -152,
    -232,  -232,  -232,   153,  -207,  -232,  -172,    -8,  -232,   305,
    -232,    29,  -232,   -85,  -232,    -7,  -232,  -232,   -24,   -17,
    -232,  -232,  -151,   248,   167,  -231,  -143,   279,   196,  -232,
     223,   197,  -232,   224
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -120
static const yytype_int16 yytable[] =
{
      31,    60,    63,    64,   218,    85,    67,    68,   220,   227,
     221,   224,    86,    78,    87,   255,    66,   217,     6,     7,
     130,   264,     6,     7,    31,   175,   176,   177,    58,   204,
       1,     2,     3,     4,     5,     6,     7,   225,     9,   271,
      98,    99,    10,   193,    94,   120,   126,   234,    72,    73,
      74,    75,    76,    13,   128,    15,   255,   212,    18,    85,
      85,    14,    18,   129,   218,    95,    69,   148,   220,   137,
     221,   224,   151,    17,   248,    18,    89,    98,    99,    31,
      31,    97,   157,    92,    90,   159,   160,   161,   268,    93,
     163,   164,   141,   218,    66,    98,    99,   220,   142,   221,
     224,   104,   105,   106,   107,   218,   178,   179,   184,   220,
     217,   221,   224,   185,   201,     6,     7,   108,   202,   109,
     126,    85,   201,   254,    63,   192,   203,   194,    78,    78,
     166,   167,   168,   169,   170,   171,   172,   173,   174,    31,
      31,   108,    12,   109,   217,   214,   209,   210,    15,     6,
       7,   186,   205,    70,    71,    18,   187,   269,   205,    98,
      99,    77,   219,    79,     1,     2,     3,     4,     5,     6,
       7,     8,     9,   120,   228,   229,    10,   120,   215,    11,
      80,    12,    15,    53,   152,    54,   153,    13,    53,    18,
      54,    91,    98,    99,  -117,    14,   137,   154,     2,     3,
       4,     5,    15,   182,    16,   110,   111,    17,   100,    18,
      19,    20,    21,    22,   133,    23,    98,    99,   155,    96,
     145,   101,   219,    98,    99,   211,   216,   257,    84,   -59,
     -59,     1,     2,     3,     4,     5,     6,     7,     8,     9,
      98,    99,   205,    10,   198,   112,    11,   115,    12,  -118,
      63,   219,   120,   244,    13,   102,   103,   104,   105,   106,
     107,   195,    14,   219,  -119,   241,   196,    98,    99,    15,
     242,    16,    90,   108,    17,   109,    18,    19,    20,    21,
      22,  -105,    23,     1,     2,     3,     4,     5,     6,     7,
       8,     9,    53,   245,    54,    10,   199,   247,    11,   138,
      12,   235,   242,  -117,   108,   181,    13,    98,    99,   134,
      98,    99,   131,   132,    14,   113,   114,   143,    98,    99,
     149,    15,   150,    16,   265,   266,    17,   158,    18,    19,
      20,    21,    22,   162,    23,     1,     2,     3,     4,     5,
       6,     7,     8,     9,   139,   140,   180,    10,   183,   188,
      11,   190,    12,   197,   200,   208,  -120,   211,    13,    54,
     231,   233,   238,   237,   240,   243,    14,   246,   251,   252,
     239,   128,   256,    15,   262,    16,   153,   258,    17,    93,
      18,    19,    20,    21,    22,   267,    23,   116,     2,     3,
       4,     5,     6,     7,   181,     9,   117,   260,   270,    10,
     146,   236,   191,   259,   263,   165,   213,   189,   230,   261,
      13,   232,     0,   249,   250,     0,     0,     0,    14,     0,
       0,     0,     0,   143,     2,     3,     4,     5,     0,     0,
      17,   118,    18,    19,    20,    21,    22,     0,    23,     1,
       2,     3,     4,     5,     6,     7,   145,     9,     0,     0,
       0,    10,     0,     0,    84,     0,     0,     0,     0,     0,
       0,     0,    13,     0,     0,     0,     0,     0,     0,     0,
      14,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    17,   226,    18,    19,    20,    21,    22,     0,
      23,     1,     2,     3,     4,     5,     6,     7,     0,     9,
       0,     0,     0,    10,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    13,     0,     0,     0,     0,     0,
       0,     0,    14,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    17,   118,    18,    19,    20,    21,
      22,     0,    23,     1,     2,     3,     4,     5,     6,     7,
       0,     9,     0,     0,     0,    10,     1,     2,     3,     4,
       5,     6,     7,     0,     9,     0,    13,     0,    10,     0,
       0,     0,     0,     0,    14,     0,     0,     0,     0,    13,
      81,     2,     3,     4,     5,     0,    17,    14,    18,    19,
      20,    21,    22,     0,    23,     0,     0,     0,     0,    17,
       0,    18,    82,    83,     0,     0,     0,     0,     0,     0,
       0,    84
};

static const yytype_int16 yycheck[] =
{
       0,     9,    10,    10,   162,    29,    13,    14,   162,   181,
     162,   162,    29,    24,    30,   222,     3,     3,     8,     9,
      12,   252,     8,     9,    24,   110,   111,   112,     9,    16,
       3,     4,     5,     6,     7,     8,     9,   180,    11,   270,
      32,    33,    15,    16,    24,    53,    54,   190,    19,    20,
      21,    22,    23,    26,    13,    41,   263,    12,    48,    83,
      84,    34,    48,    22,   222,    45,    11,    84,   222,    69,
     222,   222,    88,    46,   217,    48,     3,    32,    33,    79,
      80,    18,    90,    18,    11,    93,    94,    95,   260,    24,
      98,    99,    15,   251,     3,    32,    33,   251,    21,   251,
     251,    37,    38,    39,    40,   263,   113,   114,    12,   263,
       3,   263,   263,    17,    17,     8,     9,    53,    21,    55,
     128,   145,    17,    16,   132,   133,    21,   134,   139,   140,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   139,
     140,    53,    20,    55,     3,    18,   154,   155,    41,     8,
       9,    12,   152,     8,     9,    48,    17,    16,   158,    32,
      33,     0,   162,    15,     3,     4,     5,     6,     7,     8,
       9,    10,    11,   181,   182,   183,    15,   185,    18,    18,
      15,    20,    41,    11,    11,    13,    13,    26,    11,    48,
      13,     3,    32,    33,    22,    34,   196,    24,     4,     5,
       6,     7,    41,    19,    43,    25,    26,    46,    30,    48,
      49,    50,    51,    52,    23,    54,    32,    33,    45,    24,
      26,    31,   222,    32,    33,    13,    18,   235,    34,    17,
      18,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      32,    33,   242,    15,    16,    29,    18,    22,    20,    22,
     258,   251,   260,    18,    26,    35,    36,    37,    38,    39,
      40,    12,    34,   263,    22,    12,    17,    32,    33,    41,
      17,    43,    11,    53,    46,    55,    48,    49,    50,    51,
      52,    18,    54,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    11,    18,    13,    15,    16,    12,    18,     9,
      20,    23,    17,    22,    53,    24,    26,    32,    33,    18,
      32,    33,    16,    17,    34,    27,    28,     3,    32,    33,
      21,    41,    20,    43,    17,    18,    46,    11,    48,    49,
      50,    51,    52,    15,    54,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    79,    80,     3,    15,    19,    14,
      18,     3,    20,     3,    21,     3,    53,    13,    26,    13,
       3,    14,     3,    44,     3,    14,    34,     3,    15,     3,
      21,    13,    18,    41,    14,    43,    13,    15,    46,    24,
      48,    49,    50,    51,    52,    18,    54,     3,     4,     5,
       6,     7,     8,     9,    24,    11,    12,    24,     3,    15,
      83,   196,   132,   237,   251,   100,   158,   128,   185,   242,
      26,   187,    -1,   217,   217,    -1,    -1,    -1,    34,    -1,
      -1,    -1,    -1,     3,     4,     5,     6,     7,    -1,    -1,
      46,    47,    48,    49,    50,    51,    52,    -1,    54,     3,
       4,     5,     6,     7,     8,     9,    26,    11,    -1,    -1,
      -1,    15,    -1,    -1,    34,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    26,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    46,    47,    48,    49,    50,    51,    52,    -1,
      54,     3,     4,     5,     6,     7,     8,     9,    -1,    11,
      -1,    -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    26,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    46,    47,    48,    49,    50,    51,
      52,    -1,    54,     3,     4,     5,     6,     7,     8,     9,
      -1,    11,    -1,    -1,    -1,    15,     3,     4,     5,     6,
       7,     8,     9,    -1,    11,    -1,    26,    -1,    15,    -1,
      -1,    -1,    -1,    -1,    34,    -1,    -1,    -1,    -1,    26,
       3,     4,     5,     6,     7,    -1,    46,    34,    48,    49,
      50,    51,    52,    -1,    54,    -1,    -1,    -1,    -1,    46,
      -1,    48,    25,    26,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    34
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      15,    18,    20,    26,    34,    41,    43,    46,    48,    49,
      50,    51,    52,    54,    58,    59,    61,    62,    63,    64,
      65,    69,    70,    71,    74,    75,    76,    77,    78,    79,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    95,
      97,    98,    99,    11,    13,   103,   105,   108,     9,    69,
      84,    72,    73,    84,    92,    94,     3,    92,    92,    11,
       8,     9,    88,    88,    88,    88,    88,     0,    59,    15,
      15,     3,    25,    26,    34,    95,    96,    63,    66,     3,
      11,     3,    18,    24,    24,    45,    24,    18,    32,    33,
      30,    31,    35,    36,    37,    38,    39,    40,    53,    55,
      25,    26,    29,    27,    28,    22,     3,    12,    47,    83,
      84,    93,   106,   107,   109,   110,    84,   104,    13,    22,
      12,    16,    17,    23,    18,    67,    68,    69,     9,    58,
      58,    15,    21,     3,    60,    26,    60,    96,    96,    21,
      20,    63,    11,    13,    24,    45,   102,    84,    11,    84,
      84,    84,    15,    84,    84,    86,    88,    88,    88,    88,
      88,    88,    88,    88,    88,    90,    90,    90,    92,    92,
       3,    24,    19,    19,    12,    17,    12,    17,    14,   104,
       3,    73,    84,    16,    92,    12,    17,     3,    16,    16,
      21,    17,    21,    21,    16,    69,   100,   101,     3,    84,
      84,    13,    12,   100,    18,    18,    18,     3,    61,    69,
      75,    76,    80,    81,    99,   103,    47,    83,    84,    84,
     107,     3,   110,    14,   103,    23,    68,    44,     3,    21,
       3,    12,    17,    14,    18,    18,     3,    12,   103,   105,
     108,    15,     3,    82,    16,    81,    18,    84,    15,    71,
      24,   101,    14,    80,   102,    17,    18,    18,    83,    16,
       3,   102
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
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
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


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
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
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
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
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      fprintf (stderr, "\n");
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

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
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



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



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
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

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
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

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

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
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
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
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
#line 88 "ParseSM/smartyacc.yy"
    {
  Reducing("statements : statements statement");
  (yyval.List) = AppendStatement((yyvsp[(1) - (2)].List), (yyvsp[(2) - (2)].Expr));
}
    break;

  case 3:
#line 93 "ParseSM/smartyacc.yy"
    {
  Reducing("statements : statement");
  (yyval.List) = AppendStatement(0, (yyvsp[(1) - (1)].Expr));
}
    break;

  case 4:
#line 101 "ParseSM/smartyacc.yy"
    {
  Reducing("statement : for_header LBRACE statements RBRACE");
  (yyval.Expr) = BuildForLoop((yyvsp[(1) - (4)].count), (yyvsp[(3) - (4)].List));
}
    break;

  case 5:
#line 106 "ParseSM/smartyacc.yy"
    { 
  Reducing("statement : converge LBRACE statements RBRACE");
  (yyval.Expr) = FinishConverge((yyvsp[(3) - (4)].List));
}
    break;

  case 6:
#line 111 "ParseSM/smartyacc.yy"
    { 
  Reducing("statement : decl_stmt");
  (yyval.Expr) = 0;
}
    break;

  case 7:
#line 116 "ParseSM/smartyacc.yy"
    { 
  Reducing("statement : defn_stmt");
  (yyval.Expr) = (yyvsp[(1) - (1)].Expr);
}
    break;

  case 8:
#line 121 "ParseSM/smartyacc.yy"
    { 
  Reducing("statement : model_decl");
  (yyval.Expr) = 0;
}
    break;

  case 9:
#line 126 "ParseSM/smartyacc.yy"
    {
  Reducing("statement : opt_stmt");
  (yyval.Expr) = (yyvsp[(1) - (1)].Expr);
}
    break;

  case 10:
#line 131 "ParseSM/smartyacc.yy"
    {
  Reducing("statement : arith SEMI");
  (yyval.Expr) = BuildExprStatement((yyvsp[(1) - (2)].Expr));
}
    break;

  case 11:
#line 136 "ParseSM/smartyacc.yy"
    {  
  Reducing("statement : SEMI");
  (yyval.Expr) = 0;
}
    break;

  case 12:
#line 144 "ParseSM/smartyacc.yy"
    {
  Reducing("idlist : idlist COMMA IDENT");
  (yyval.List) = AppendName((yyvsp[(1) - (3)].List), (yyvsp[(3) - (3)].name));
}
    break;

  case 13:
#line 149 "ParseSM/smartyacc.yy"
    {
  Reducing("idlist : IDENT");
  (yyval.List) = AppendName(0, (yyvsp[(1) - (1)].name));
}
    break;

  case 14:
#line 157 "ParseSM/smartyacc.yy"
    {
  Reducing("for_header : FOR LPAR iterators RPAR");
 (yyval.count) = (yyvsp[(3) - (4)].count);  
}
    break;

  case 15:
#line 165 "ParseSM/smartyacc.yy"
    {
  Reducing("converge : CONVERGE");
  StartConverge();
}
    break;

  case 16:
#line 173 "ParseSM/smartyacc.yy"
    {
  Reducing("opt_stmt : opt_header const_expr ENDPND");
  (yyval.Expr) = BuildOptionStatement((yyvsp[(1) - (3)].Option), (yyvsp[(2) - (3)].Expr));
}
    break;

  case 17:
#line 178 "ParseSM/smartyacc.yy"
    {
  Reducing("opt_stmt : opt_header IDENT ENDPND");
  (yyval.Expr) = BuildOptionStatement((yyvsp[(1) - (3)].Option), (yyvsp[(2) - (3)].name));
}
    break;

  case 18:
#line 183 "ParseSM/smartyacc.yy"
    {
  Reducing("opt_stmt : opt_header PLUS idlist ENDPND");
  (yyval.Expr) = BuildOptionStatement((yyvsp[(1) - (4)].Option), true, (yyvsp[(3) - (4)].List));
}
    break;

  case 19:
#line 188 "ParseSM/smartyacc.yy"
    {
  Reducing("opt_stmt : opt_header MINUS idlist ENDPND");
  (yyval.Expr) = BuildOptionStatement((yyvsp[(1) - (4)].Option), false, (yyvsp[(3) - (4)].List));
}
    break;

  case 20:
#line 193 "ParseSM/smartyacc.yy"
    {
  Reducing("opt_stmt : opt_begin opt_stmts POUND RBRACE ENDPND");
  (yyval.Expr) = FinishOptionBlock((yyvsp[(1) - (5)].Expr), (yyvsp[(2) - (5)].List));
}
    break;

  case 21:
#line 202 "ParseSM/smartyacc.yy"
    {
  Reducing("opt_header : POUND IDENT");
  (yyval.Option) = BuildOptionHeader((yyvsp[(2) - (2)].name));
}
    break;

  case 22:
#line 210 "ParseSM/smartyacc.yy"
    {
  Reducing("opt_begin : opt_header IDENT LBRACE ENDPND");
  (yyval.Expr) = StartOptionBlock((yyvsp[(1) - (4)].Option), (yyvsp[(2) - (4)].name)); 
}
    break;

  case 23:
#line 218 "ParseSM/smartyacc.yy"
    {
  Reducing("opt_stmts : opt_stmts opt_stmt");
  (yyval.List) = AppendStatement((yyvsp[(1) - (2)].List), (yyvsp[(2) - (2)].Expr));
}
    break;

  case 24:
#line 223 "ParseSM/smartyacc.yy"
    {
  Reducing("opt_stmts : opt_stmt");
  (yyval.List) = AppendStatement(0, (yyvsp[(1) - (1)].Expr));
}
    break;

  case 25:
#line 232 "ParseSM/smartyacc.yy"
    {
  Reducing("iterators : iterators COMMA iterator");
  (yyval.count) = (yyvsp[(1) - (3)].count) + AddIterator((yyvsp[(3) - (3)].Symbol));
}
    break;

  case 26:
#line 237 "ParseSM/smartyacc.yy"
    { 
  Reducing("iterators : iterator");
  (yyval.count) = AddIterator((yyvsp[(1) - (1)].Symbol));
}
    break;

  case 27:
#line 245 "ParseSM/smartyacc.yy"
    {
  Reducing("iterator : type IDENT IN set_expr");
  (yyval.Symbol) = BuildIterator((yyvsp[(1) - (4)].Type_ID), (yyvsp[(2) - (4)].name), (yyvsp[(4) - (4)].Expr));
}
    break;

  case 28:
#line 253 "ParseSM/smartyacc.yy"
    {
  Reducing("type : PROC MODIF TYPE");
  (yyval.Type_ID) = MakeType(true, (yyvsp[(2) - (3)].name), (yyvsp[(3) - (3)].Type_ID));
}
    break;

  case 29:
#line 258 "ParseSM/smartyacc.yy"
    {
  Reducing("type : PROC TYPE");
  (yyval.Type_ID) = MakeType(true, 0, (yyvsp[(2) - (2)].Type_ID));
}
    break;

  case 30:
#line 263 "ParseSM/smartyacc.yy"
    {
  Reducing("type : MODIF TYPE");
  (yyval.Type_ID) = MakeType(false, (yyvsp[(1) - (2)].name), (yyvsp[(2) - (2)].Type_ID));
}
    break;

  case 31:
#line 268 "ParseSM/smartyacc.yy"
    {
  Reducing("type : TYPE");
  (yyval.Type_ID) = MakeType(false, 0, (yyvsp[(1) - (1)].Type_ID));
}
    break;

  case 32:
#line 276 "ParseSM/smartyacc.yy"
    {
  Reducing("model : FORMALISM");
  (yyval.Type_ID) = MakeType(false, 0, (yyvsp[(1) - (1)].Type_ID));
}
    break;

  case 33:
#line 284 "ParseSM/smartyacc.yy"
    {
  Reducing("set_expr : LBRACE set_elems RBRACE");
  (yyval.Expr) = BuildAssociative(COMMA, (yyvsp[(2) - (3)].List));
}
    break;

  case 34:
#line 292 "ParseSM/smartyacc.yy"
    {
  Reducing("set_elems : set_elems COMMA set_elem");
  (yyval.List) = AppendExpression(2, (yyvsp[(1) - (3)].List), (yyvsp[(3) - (3)].Expr));
}
    break;

  case 35:
#line 297 "ParseSM/smartyacc.yy"
    {
  Reducing("set_elems : set_elem");
  (yyval.List) = AppendExpression(2, 0, (yyvsp[(1) - (1)].Expr));
}
    break;

  case 36:
#line 305 "ParseSM/smartyacc.yy"
    {
  Reducing("set_elem : arith");
  (yyval.Expr) = BuildElementSet((yyvsp[(1) - (1)].Expr));
}
    break;

  case 37:
#line 310 "ParseSM/smartyacc.yy"
    {
  Reducing("set_elem : arith DOTDOT arith");
  (yyval.Expr) = BuildInterval((yyvsp[(1) - (3)].Expr), (yyvsp[(3) - (3)].Expr)); 
}
    break;

  case 38:
#line 315 "ParseSM/smartyacc.yy"
    {
  Reducing("set_elem : arith DOTDOT arith DOTDOT arith");
  (yyval.Expr) = BuildInterval((yyvsp[(1) - (5)].Expr), (yyvsp[(3) - (5)].Expr), (yyvsp[(5) - (5)].Expr));
}
    break;

  case 39:
#line 329 "ParseSM/smartyacc.yy"
    {
  Reducing("decl_stmt : func_header SEMI");
  DoneWithFunctionHeader();
}
    break;

  case 40:
#line 337 "ParseSM/smartyacc.yy"
    {
  Reducing("defn_stmt : func_header GETS arith SEMI");
  (yyval.Expr) = BuildFuncStmt((yyvsp[(1) - (4)].Symbol), (yyvsp[(3) - (4)].Expr));
}
    break;

  case 41:
#line 342 "ParseSM/smartyacc.yy"
    {
  Reducing("defn_stmt : type IDENT GETS arith SEMI");
  (yyval.Expr) = BuildVarStmt((yyvsp[(1) - (5)].Type_ID), (yyvsp[(2) - (5)].name), (yyvsp[(4) - (5)].Expr));
}
    break;

  case 42:
#line 347 "ParseSM/smartyacc.yy"
    {
  Reducing("defn_stmt : type IDENT GUESS arith SEMI");
  (yyval.Expr) = BuildGuessStmt((yyvsp[(1) - (5)].Type_ID), (yyvsp[(2) - (5)].name), (yyvsp[(4) - (5)].Expr));
}
    break;

  case 43:
#line 352 "ParseSM/smartyacc.yy"
    {
  Reducing("defn_stmt : array_header GETS arith SEMI");
  (yyval.Expr) = BuildArrayStmt((yyvsp[(1) - (4)].Symbol), (yyvsp[(3) - (4)].Expr));
}
    break;

  case 44:
#line 357 "ParseSM/smartyacc.yy"
    {
  Reducing("defn_stmt : array_header GUESS arith SEMI");
  (yyval.Expr) = BuildArrayGuess((yyvsp[(1) - (4)].Symbol), (yyvsp[(3) - (4)].Expr));
}
    break;

  case 45:
#line 371 "ParseSM/smartyacc.yy"
    {
  Reducing("func_header : type IDENT LPAR formal_params RPAR");
  (yyval.Symbol) = BuildFunction((yyvsp[(1) - (5)].Type_ID), (yyvsp[(2) - (5)].name), (yyvsp[(4) - (5)].List));
}
    break;

  case 46:
#line 379 "ParseSM/smartyacc.yy"
    {
  Reducing("array_header : type IDENT formal_indexes");
  (yyval.Symbol) = BuildArray((yyvsp[(1) - (3)].Type_ID), (yyvsp[(2) - (3)].name), (yyvsp[(3) - (3)].List));
}
    break;

  case 47:
#line 393 "ParseSM/smartyacc.yy"
    {
  Reducing("model_decl : model_header GETS LBRACE model_stmts RBRACE SEMI");
  BuildModelStmt((yyvsp[(1) - (6)].Symbol), (yyvsp[(4) - (6)].List));
}
    break;

  case 48:
#line 402 "ParseSM/smartyacc.yy"
    {
  Reducing("model_header : model IDENT LPAR formal_params RPAR");
  (yyval.Symbol) = BuildModel((yyvsp[(1) - (5)].Type_ID), (yyvsp[(2) - (5)].name), (yyvsp[(4) - (5)].List));
}
    break;

  case 49:
#line 407 "ParseSM/smartyacc.yy"
    {
  Reducing("model_header : model IDENT");
  (yyval.Symbol) = BuildModel((yyvsp[(1) - (2)].Type_ID), (yyvsp[(2) - (2)].name), 0);
}
    break;

  case 50:
#line 415 "ParseSM/smartyacc.yy"
    {
  Reducing("model_stmts : model_stmts model_stmt");
  (yyval.List) = AppendStatement((yyvsp[(1) - (2)].List), (yyvsp[(2) - (2)].Expr));
}
    break;

  case 51:
#line 420 "ParseSM/smartyacc.yy"
    {
  Reducing("model_stmts : model_stmt");
  (yyval.List) = AppendStatement(0, (yyvsp[(1) - (1)].Expr));
}
    break;

  case 52:
#line 428 "ParseSM/smartyacc.yy"
    {
  Reducing("model_stmt : function_call SEMI");
  (yyval.Expr) = BuildExprStatement((yyvsp[(1) - (2)].Expr));
}
    break;

  case 53:
#line 433 "ParseSM/smartyacc.yy"
    {
  Reducing("model_stmt : type model_var_list SEMI");
  (yyval.Expr) = BuildModelVarStmt((yyvsp[(1) - (3)].Type_ID), (yyvsp[(2) - (3)].List));
}
    break;

  case 54:
#line 438 "ParseSM/smartyacc.yy"
    {
  Reducing("model_stmt : defn_stmt");
  (yyval.Expr) = (yyvsp[(1) - (1)].Expr);
}
    break;

  case 55:
#line 443 "ParseSM/smartyacc.yy"
    {
  Reducing("model_stmt : for_header LBRACE model_stmts RBRACE");
  (yyval.Expr) = BuildForLoop((yyvsp[(1) - (4)].count), (yyvsp[(3) - (4)].List));
}
    break;

  case 56:
#line 451 "ParseSM/smartyacc.yy"
    {
  Reducing("model_var_list : model_var_list COMMA IDENT");
  (yyval.List) = AddModelVar((yyvsp[(1) - (3)].List), (yyvsp[(3) - (3)].name));
}
    break;

  case 57:
#line 456 "ParseSM/smartyacc.yy"
    {
  Reducing("model_var_list : model_var_list COMMA IDENT formal_indexes");
  (yyval.List) = AddModelArray((yyvsp[(1) - (4)].List), (yyvsp[(3) - (4)].name), (yyvsp[(4) - (4)].List));
}
    break;

  case 58:
#line 461 "ParseSM/smartyacc.yy"
    {
  Reducing("model_var_list : IDENT");
  (yyval.List) = AddModelVar(0, (yyvsp[(1) - (1)].name));
}
    break;

  case 59:
#line 466 "ParseSM/smartyacc.yy"
    {
  Reducing("model_var_list : IDENT formal_indexes");
  (yyval.List) = AddModelArray(0, (yyvsp[(1) - (2)].name), (yyvsp[(2) - (2)].List));
}
    break;

  case 60:
#line 481 "ParseSM/smartyacc.yy"
    {
  Reducing("expr : arith");
  (yyval.Expr) = (yyvsp[(1) - (1)].Expr);
}
    break;

  case 61:
#line 486 "ParseSM/smartyacc.yy"
    {
  Reducing("expr : aggexpr");
  (yyval.Expr) = BuildAssociative(COLON, (yyvsp[(1) - (1)].List));
}
    break;

  case 62:
#line 494 "ParseSM/smartyacc.yy"
    {
  Reducing("arith : disjunct");
  (yyval.Expr) = BuildSummation((yyvsp[(1) - (1)].List));
}
    break;

  case 63:
#line 499 "ParseSM/smartyacc.yy"
    {
  Reducing("arith : arith IMPLIES arith");
  (yyval.Expr) = BuildBinary((yyvsp[(1) - (3)].Expr), IMPLIES, (yyvsp[(3) - (3)].Expr));
}
    break;

  case 64:
#line 504 "ParseSM/smartyacc.yy"
    {
  Reducing("arith : arith SET_DIFF arith");
  (yyval.Expr) = BuildBinary((yyvsp[(1) - (3)].Expr), SET_DIFF, (yyvsp[(3) - (3)].Expr));
}
    break;

  case 65:
#line 512 "ParseSM/smartyacc.yy"
    {
  Reducing("disjunct : disjunct OR doneconj");
  (yyval.List) = AppendTerm((yyvsp[(1) - (3)].List), OR, (yyvsp[(3) - (3)].Expr));
}
    break;

  case 66:
#line 517 "ParseSM/smartyacc.yy"
    {
  Reducing("disjunct : doneconj");
  (yyval.List) = AppendTerm(0, 0, (yyvsp[(1) - (1)].Expr));
}
    break;

  case 67:
#line 525 "ParseSM/smartyacc.yy"
    {
  Reducing("doneconj : conjunct");
  (yyval.Expr) = BuildProduct((yyvsp[(1) - (1)].List));
}
    break;

  case 68:
#line 533 "ParseSM/smartyacc.yy"
    {
  Reducing("conjunct : conjunct AND logic");
  (yyval.List) = AppendTerm((yyvsp[(1) - (3)].List), AND, (yyvsp[(3) - (3)].Expr));
}
    break;

  case 69:
#line 538 "ParseSM/smartyacc.yy"
    {
  Reducing("conjunct : logic");
  (yyval.List) = AppendTerm(0, 0, (yyvsp[(1) - (1)].Expr));
}
    break;

  case 70:
#line 548 "ParseSM/smartyacc.yy"
    {
  Reducing("logic : summation");
  (yyval.Expr) = BuildSummation((yyvsp[(1) - (1)].List));
}
    break;

  case 71:
#line 553 "ParseSM/smartyacc.yy"
    {
  Reducing("logic : logic EQUALS logic");
  (yyval.Expr) = BuildBinary((yyvsp[(1) - (3)].Expr), EQUALS, (yyvsp[(3) - (3)].Expr));
}
    break;

  case 72:
#line 558 "ParseSM/smartyacc.yy"
    {
  Reducing("logic : logic NEQUAL logic");
  (yyval.Expr) = BuildBinary((yyvsp[(1) - (3)].Expr), NEQUAL, (yyvsp[(3) - (3)].Expr));
}
    break;

  case 73:
#line 563 "ParseSM/smartyacc.yy"
    {
  Reducing("logic : logic GT logic");
  (yyval.Expr) = BuildBinary((yyvsp[(1) - (3)].Expr), GT, (yyvsp[(3) - (3)].Expr));
}
    break;

  case 74:
#line 568 "ParseSM/smartyacc.yy"
    {
  Reducing("logic : logic GE logic");
  (yyval.Expr) = BuildBinary((yyvsp[(1) - (3)].Expr), GE, (yyvsp[(3) - (3)].Expr));
}
    break;

  case 75:
#line 573 "ParseSM/smartyacc.yy"
    {
  Reducing("logic : logic LT logic");
  (yyval.Expr) = BuildBinary((yyvsp[(1) - (3)].Expr), LT, (yyvsp[(3) - (3)].Expr));
}
    break;

  case 76:
#line 578 "ParseSM/smartyacc.yy"
    {
  Reducing("logic : logic LE logic");
  (yyval.Expr) = BuildBinary((yyvsp[(1) - (3)].Expr), LE, (yyvsp[(3) - (3)].Expr));
}
    break;

  case 77:
#line 583 "ParseSM/smartyacc.yy"
    {
  Reducing("logic : logic TEMPORALAND logic");
  (yyval.Expr) = BuildBinary((yyvsp[(1) - (3)].Expr), TEMPORALAND, (yyvsp[(3) - (3)].Expr));
}
    break;

  case 78:
#line 588 "ParseSM/smartyacc.yy"
    {
  Reducing("logic : FORALL logic");
  (yyval.Expr) = BuildUnary(FORALL, (yyvsp[(2) - (2)].Expr));
}
    break;

  case 79:
#line 593 "ParseSM/smartyacc.yy"
    {
  Reducing("logic : EXISTS logic");
  (yyval.Expr) = BuildUnary(EXISTS, (yyvsp[(2) - (2)].Expr));
}
    break;

  case 80:
#line 598 "ParseSM/smartyacc.yy"
    {
  Reducing("logic : NEXT logic");
  (yyval.Expr) = BuildUnary(NEXT, (yyvsp[(2) - (2)].Expr));
}
    break;

  case 81:
#line 603 "ParseSM/smartyacc.yy"
    {
  Reducing("logic : FUTURE Logic");
  (yyval.Expr) = BuildUnary(FUTURE, (yyvsp[(2) - (2)].Expr));
}
    break;

  case 82:
#line 608 "ParseSM/smartyacc.yy"
    {
  Reducing("logic : GLOBALLY logic");
  (yyval.Expr) = BuildUnary(GLOBALLY, (yyvsp[(2) - (2)].Expr));
}
    break;

  case 83:
#line 613 "ParseSM/smartyacc.yy"
    {
  Reducing("logic : logic UNTIL logic");
  (yyval.Expr) = BuildBinary((yyvsp[(1) - (3)].Expr), UNTIL, (yyvsp[(3) - (3)].Expr));
}
    break;

  case 84:
#line 621 "ParseSM/smartyacc.yy"
    {
  Reducing("summation : summation PLUS doneproduct");
  (yyval.List) = AppendTerm((yyvsp[(1) - (3)].List), PLUS, (yyvsp[(3) - (3)].Expr));
}
    break;

  case 85:
#line 626 "ParseSM/smartyacc.yy"
    {
  Reducing("summation : summation MINUS doneproduct");
  (yyval.List) = AppendTerm((yyvsp[(1) - (3)].List), MINUS, (yyvsp[(3) - (3)].Expr));
}
    break;

  case 86:
#line 631 "ParseSM/smartyacc.yy"
    {
  Reducing("summation : doneproduct");
  (yyval.List) = AppendTerm(0, 0, (yyvsp[(1) - (1)].Expr));
}
    break;

  case 87:
#line 639 "ParseSM/smartyacc.yy"
    {
  Reducing("doneproduct : product");
  (yyval.Expr) = BuildProduct((yyvsp[(1) - (1)].List));
}
    break;

  case 88:
#line 644 "ParseSM/smartyacc.yy"
    {
  Reducing("doneproduct : doneproduct MOD doneproduct");
  (yyval.Expr) = BuildBinary((yyvsp[(1) - (3)].Expr), MOD, (yyvsp[(3) - (3)].Expr));
}
    break;

  case 89:
#line 652 "ParseSM/smartyacc.yy"
    {
  Reducing("product : product TIMES term");
  (yyval.List) = AppendTerm((yyvsp[(1) - (3)].List), TIMES, (yyvsp[(3) - (3)].Expr));
}
    break;

  case 90:
#line 657 "ParseSM/smartyacc.yy"
    {
  Reducing("product : product DIVIDE term");
  (yyval.List) = AppendTerm((yyvsp[(1) - (3)].List), DIVIDE, (yyvsp[(3) - (3)].Expr));
}
    break;

  case 91:
#line 662 "ParseSM/smartyacc.yy"
    {
  Reducing("product : term");
  (yyval.List) = AppendTerm(0, 0, (yyvsp[(1) - (1)].Expr));
}
    break;

  case 92:
#line 670 "ParseSM/smartyacc.yy"
    {  
  Reducing("term : NUL");
  (yyval.Expr) = 0;
}
    break;

  case 93:
#line 675 "ParseSM/smartyacc.yy"
    {
  Reducing("term : value");
  (yyval.Expr) = (yyvsp[(1) - (1)].Expr);
}
    break;

  case 94:
#line 680 "ParseSM/smartyacc.yy"
    {
  Reducing("term : set_expr");
  (yyval.Expr) = (yyvsp[(1) - (1)].Expr);
}
    break;

  case 95:
#line 685 "ParseSM/smartyacc.yy"
    {
  Reducing("term : model_function_call");
  (yyval.Expr) = (yyvsp[(1) - (1)].Expr);
}
    break;

  case 96:
#line 690 "ParseSM/smartyacc.yy"
    {
  Reducing("term : function_call");
  (yyval.Expr) = (yyvsp[(1) - (1)].Expr);
}
    break;

  case 97:
#line 695 "ParseSM/smartyacc.yy"
    {
  Reducing("term : LPAR arith RPAR");
  (yyval.Expr) = (yyvsp[(2) - (3)].Expr);
}
    break;

  case 98:
#line 700 "ParseSM/smartyacc.yy"
    {
  Reducing("term : NOT term");
  (yyval.Expr) = BuildUnary(NOT, (yyvsp[(2) - (2)].Expr));
}
    break;

  case 99:
#line 705 "ParseSM/smartyacc.yy"
    {
  Reducing("term : MINUS term");
  (yyval.Expr) = BuildUnary(MINUS, (yyvsp[(2) - (2)].Expr));
}
    break;

  case 100:
#line 710 "ParseSM/smartyacc.yy"
    {
  Reducing("term : type LPAR arith RPAR");
  (yyval.Expr) = BuildTypecast((yyvsp[(1) - (4)].Type_ID), (yyvsp[(3) - (4)].Expr));
}
    break;

  case 101:
#line 715 "ParseSM/smartyacc.yy"
    {
  Reducing("term : LBRACE seqexpr SEMI RBRACE");
  (yyval.Expr) = BuildAssociative(SEMI, (yyvsp[(2) - (4)].List));
}
    break;

  case 102:
#line 724 "ParseSM/smartyacc.yy"
    {
  Reducing("aggexpr : aggexpr COLON arith");
  (yyval.List) = AppendExpression(0, (yyvsp[(1) - (3)].List), (yyvsp[(3) - (3)].Expr));
}
    break;

  case 103:
#line 729 "ParseSM/smartyacc.yy"
    {
  Reducing("aggexpr : arith COLON arith");
  (yyval.List) = AppendExpression(0, AppendExpression(2, 0, (yyvsp[(1) - (3)].Expr)), (yyvsp[(3) - (3)].Expr));
}
    break;

  case 104:
#line 738 "ParseSM/smartyacc.yy"
    {
  Reducing("seqexpr : seqexpr SEMI term");
  (yyval.List) = AppendExpression(1, (yyvsp[(1) - (3)].List), (yyvsp[(3) - (3)].Expr));
}
    break;

  case 105:
#line 743 "ParseSM/smartyacc.yy"
    {
  Reducing("seqexpr : term");
  (yyval.List) = AppendExpression(1, 0, (yyvsp[(1) - (1)].Expr));
}
    break;

  case 106:
#line 753 "ParseSM/smartyacc.yy"
    {
  Reducing("value : BOOLCONST");
  (yyval.Expr) = MakeBoolConst((yyvsp[(1) - (1)].name));
}
    break;

  case 107:
#line 758 "ParseSM/smartyacc.yy"
    {
  Reducing("value : INTCONST");
  (yyval.Expr) = MakeIntConst((yyvsp[(1) - (1)].name));
}
    break;

  case 108:
#line 763 "ParseSM/smartyacc.yy"
    {
  Reducing("value : REALCONST");
  (yyval.Expr) = MakeRealConst((yyvsp[(1) - (1)].name));
}
    break;

  case 109:
#line 768 "ParseSM/smartyacc.yy"
    {
  Reducing("value : STRCONST");
  (yyval.Expr) = MakeStringConst((yyvsp[(1) - (1)].name));
}
    break;

  case 110:
#line 776 "ParseSM/smartyacc.yy"
    {
  Reducing("const_expr : value");
  (yyval.Expr) = (yyvsp[(1) - (1)].Expr);
}
    break;

  case 111:
#line 781 "ParseSM/smartyacc.yy"
    {
  Reducing("const_expr : MINUS const_expr");
  (yyval.Expr) = BuildUnary(MINUS, (yyvsp[(2) - (2)].Expr));
}
    break;

  case 112:
#line 786 "ParseSM/smartyacc.yy"
    {
  Reducing("const_expr : NOT const_expr");
  (yyval.Expr) = BuildUnary(NOT, (yyvsp[(2) - (2)].Expr));
}
    break;

  case 113:
#line 801 "ParseSM/smartyacc.yy"
    {  
  Reducing("model_function_call : model_call DOT IDENT");
  (yyval.Expr) = MakeMCall((yyvsp[(1) - (3)].other), (yyvsp[(3) - (3)].name));
}
    break;

  case 114:
#line 806 "ParseSM/smartyacc.yy"
    {
  Reducing("model_function_call : IDENT indexes DOT IDENT");
  (yyval.Expr) = MakeAMCall((yyvsp[(1) - (4)].name), (yyvsp[(2) - (4)].List), (yyvsp[(4) - (4)].name));
}
    break;

  case 115:
#line 811 "ParseSM/smartyacc.yy"
    {
  Reducing("model_function_call : model_call DOT IDENT indexes");
  (yyval.Expr) = MakeMACall((yyvsp[(1) - (4)].other), (yyvsp[(3) - (4)].name), (yyvsp[(4) - (4)].List));
}
    break;

  case 116:
#line 816 "ParseSM/smartyacc.yy"
    {
  Reducing("model_function_call : IDENT indexes DOT IDENT indexes");
  (yyval.Expr) = MakeAMACall((yyvsp[(1) - (5)].name), (yyvsp[(2) - (5)].List), (yyvsp[(4) - (5)].name), (yyvsp[(5) - (5)].List));
}
    break;

  case 117:
#line 824 "ParseSM/smartyacc.yy"
    { 
  Reducing("model_call : IDENT");
  (yyval.other) = MakeModelCallPP((yyvsp[(1) - (1)].name), 0);
}
    break;

  case 118:
#line 829 "ParseSM/smartyacc.yy"
    { 
  Reducing("model_call : IDENT passed_params");
  (yyval.other) = MakeModelCallPP((yyvsp[(1) - (2)].name), (yyvsp[(2) - (2)].List));
}
    break;

  case 119:
#line 834 "ParseSM/smartyacc.yy"
    { 
  Reducing("model_call : IDENT named_params");
  (yyval.other) = MakeModelCallNP((yyvsp[(1) - (2)].name), (yyvsp[(2) - (2)].List));
}
    break;

  case 120:
#line 848 "ParseSM/smartyacc.yy"
    {
  Reducing("function_call : IDENT");
  (yyval.Expr) = FindIdent((yyvsp[(1) - (1)].name));
}
    break;

  case 121:
#line 853 "ParseSM/smartyacc.yy"
    {
  Reducing("function_call : IDENT indexes");
  (yyval.Expr) = BuildArrayCall((yyvsp[(1) - (2)].name), (yyvsp[(2) - (2)].List));
}
    break;

  case 122:
#line 858 "ParseSM/smartyacc.yy"
    {
  Reducing("function_call : IDENT passed_params");
  (yyval.Expr) = BuildFuncCallPP((yyvsp[(1) - (2)].name), (yyvsp[(2) - (2)].List));
}
    break;

  case 123:
#line 863 "ParseSM/smartyacc.yy"
    {
  Reducing("function_call : IDENT named_params");
  (yyval.Expr) = BuildFuncCallNP((yyvsp[(1) - (2)].name), (yyvsp[(2) - (2)].List));
}
    break;

  case 124:
#line 877 "ParseSM/smartyacc.yy"
    {
  Reducing("formal_params : formal_params COMMA formal_param");
  (yyval.List) = AppendSymbol((yyvsp[(1) - (3)].List), (yyvsp[(3) - (3)].Symbol), "formal parameter");
}
    break;

  case 125:
#line 882 "ParseSM/smartyacc.yy"
    {
  Reducing("formal_params : formal_param");
  (yyval.List) = AppendSymbol(0, (yyvsp[(1) - (1)].Symbol), "formal parameter");
}
    break;

  case 126:
#line 890 "ParseSM/smartyacc.yy"
    {
  Reducing("formal_param : type IDENT");
  (yyval.Symbol) = BuildFormal((yyvsp[(1) - (2)].Type_ID), (yyvsp[(2) - (2)].name));
}
    break;

  case 127:
#line 895 "ParseSM/smartyacc.yy"
    {
  Reducing("formal_param : type IDENT GETS expr");
  (yyval.Symbol) = BuildFormal((yyvsp[(1) - (4)].Type_ID), (yyvsp[(2) - (4)].name), (yyvsp[(4) - (4)].Expr));
}
    break;

  case 128:
#line 903 "ParseSM/smartyacc.yy"
    {
  Reducing("formal_indexes : formal_indexes LBRAK IDENT RBRAK");
  (yyval.List) = AppendName((yyvsp[(1) - (4)].List), (yyvsp[(3) - (4)].name));
}
    break;

  case 129:
#line 908 "ParseSM/smartyacc.yy"
    {
  Reducing("formal_indexes : LBRAK IDENT RBRAK");
  (yyval.List) = AppendName(0, (yyvsp[(2) - (3)].name));
}
    break;

  case 130:
#line 917 "ParseSM/smartyacc.yy"
    {
  Reducing("indexes : indexes LBRAK index RBRAK");
  (yyval.List) = AppendExpression(0, (yyvsp[(1) - (4)].List), (yyvsp[(3) - (4)].Expr));
}
    break;

  case 131:
#line 922 "ParseSM/smartyacc.yy"
    {
  Reducing("indexes : LBRAK index RBRAK");
  (yyval.List) = AppendExpression(0, 0, (yyvsp[(2) - (3)].Expr));
}
    break;

  case 132:
#line 930 "ParseSM/smartyacc.yy"
    {
  Reducing("index : arith");
  (yyval.Expr) = (yyvsp[(1) - (1)].Expr);
}
    break;

  case 133:
#line 938 "ParseSM/smartyacc.yy"
    {
  Reducing("passed_params : LPAR pos_params RPAR");
  (yyval.List) = (yyvsp[(2) - (3)].List);
}
    break;

  case 134:
#line 946 "ParseSM/smartyacc.yy"
    {
  Reducing("pos_params : pos_params COMMA pos_param");
  (yyval.List) = AppendExpression(0, (yyvsp[(1) - (3)].List), (yyvsp[(3) - (3)].Expr));
}
    break;

  case 135:
#line 951 "ParseSM/smartyacc.yy"
    {
  Reducing("pos_params : pos_param");
  (yyval.List) = AppendExpression(0, 0, (yyvsp[(1) - (1)].Expr));
}
    break;

  case 136:
#line 959 "ParseSM/smartyacc.yy"
    {
  Reducing("pos_param : expr");
  (yyval.Expr) = (yyvsp[(1) - (1)].Expr);
}
    break;

  case 137:
#line 964 "ParseSM/smartyacc.yy"
    {
  Reducing("pos_param : DEFAULT");
  (yyval.Expr) = Default();
}
    break;

  case 138:
#line 973 "ParseSM/smartyacc.yy"
    {
  Reducing("named_params : LPAR named_list RPAR");
  (yyval.List) = (yyvsp[(2) - (3)].List);
}
    break;

  case 139:
#line 978 "ParseSM/smartyacc.yy"
    {
  Reducing("named_params : LPAR RPAR");
  (yyval.List) = 0;
}
    break;

  case 140:
#line 986 "ParseSM/smartyacc.yy"
    {
  Reducing("named_list : named_list COMMA named_param");
  (yyval.List) = AppendSymbol((yyvsp[(1) - (3)].List), (yyvsp[(3) - (3)].Symbol), "named parameter");
}
    break;

  case 141:
#line 991 "ParseSM/smartyacc.yy"
    {
  Reducing("named_list : named_param");
  (yyval.List) = AppendSymbol(0, (yyvsp[(1) - (1)].Symbol), "named parameter");
}
    break;

  case 142:
#line 999 "ParseSM/smartyacc.yy"
    {
  Reducing("named_param : IDENT GETS expr");
  (yyval.Symbol) = BuildNamed((yyvsp[(1) - (3)].name), (yyvsp[(3) - (3)].Expr));
}
    break;

  case 143:
#line 1004 "ParseSM/smartyacc.yy"
    {
  Reducing("named_param : IDENT GETS DEFAULT");
  (yyval.Symbol) = BuildNamed((yyvsp[(1) - (3)].name), Default());
}
    break;


/* Line 1267 of yacc.c.  */
#line 2897 "ParseSM/smartyacc.cc"
      default: break;
    }
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
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
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

  /* Else will try to reuse look-ahead token after shifting the error
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
      if (yyn != YYPACT_NINF)
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

  if (yyn == YYFINAL)
    YYACCEPT;

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

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
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


#line 1010 "ParseSM/smartyacc.yy"

/*-----------------------------------------------------------------*/




