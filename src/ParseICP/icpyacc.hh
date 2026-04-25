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
     TYPE = 261,
     LPAR = 262,
     RPAR = 263,
     LBRACE = 264,
     RBRACE = 265,
     COMMA = 266,
     SEMI = 267,
     DOTDOT = 268,
     GETS = 269,
     PLUS = 270,
     MINUS = 271,
     TIMES = 272,
     DIVIDE = 273,
     MOD = 274,
     OR = 275,
     AND = 276,
     IMPLIES = 277,
     NOT = 278,
     EQUALS = 279,
     NEQUAL = 280,
     GT = 281,
     GE = 282,
     LT = 283,
     LE = 284,
     MAXIMIZE = 285,
     MINIMIZE = 286,
     SATISFIABLE = 287,
     IN = 288,
     POUND = 289,
     ENDPND = 290,
     COLON = 291,
     UMINUS = 292,
     RBRAK = 293,
     LBRAK = 294
   };
#endif
/* Tokens.  */
#define IDENT 258
#define BOOLCONST 259
#define INTCONST 260
#define TYPE 261
#define LPAR 262
#define RPAR 263
#define LBRACE 264
#define RBRACE 265
#define COMMA 266
#define SEMI 267
#define DOTDOT 268
#define GETS 269
#define PLUS 270
#define MINUS 271
#define TIMES 272
#define DIVIDE 273
#define MOD 274
#define OR 275
#define AND 276
#define IMPLIES 277
#define NOT 278
#define EQUALS 279
#define NEQUAL 280
#define GT 281
#define GE 282
#define LT 283
#define LE 284
#define MAXIMIZE 285
#define MINIMIZE 286
#define SATISFIABLE 287
#define IN 288
#define POUND 289
#define ENDPND 290
#define COLON 291
#define UMINUS 292
#define RBRAK 293
#define LBRAK 294




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 16 "ParseICP/icpyacc.yy"
{
  char* name;
  expr* Expr;
  parser_list* List;
  option* Option;
}
/* Line 1529 of yacc.c.  */
#line 134 "ParseICP/icpyacc.hh"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

