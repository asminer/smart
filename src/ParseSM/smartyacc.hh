/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

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
/* Line 1529 of yacc.c.  */
#line 172 "ParseSM/smartyacc.hh"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

