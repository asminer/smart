
/*
 *
 * $Id$
 *
 *
 * Yacc file (actually, bison) for smart.
 *
 * The main program is here.
 *
 *
 */

%{

#include "../../Source/Language/api.h"  // previous layer

#include <string.h>
#include <stdio.h>
#include <stdlib.h>  // keeps compiler happy
#include <new>

// #define PARSE_TRACE

%}

%union {
  char *name;
  type Type_ID;
  expr *Expr;
  function *Func;
  /*
  option *Option;
  expr_set *setexpr;
  List <local_iterator> *itrs;
  List <statement> *stmts;
  List <formal_param> *fpl;
  List <array_index> *apl;
  List <expr> *ppl;
  List <named_param> *npl;
  List <function> *varlist;
  List <char *> *tuple_ids;
  */
}

%token <name> IDENT BOOLCONST INTCONST REALCONST STRCONST 
LPAR RPAR LBRAK RBRAK LBRACE RBRACE 
COMMA SEMI COLON POUND DOT DOTDOT GETS PLUS MINUS TIMES DIVIDE OR AND NOT 
EQUALS NEQUAL GT GE LT LE ENDPND FOR END CONVERGE IN GUESS
NUL DEFAULT TYPE MODIF MODEL 

%type <Type_ID> type
%type <Expr> expr term const_expr function_call model_call model_function_call
%type <Func> header array_header
/*
%type <Option> opt_header
%type <setexpr> set_expr set_elems set_elem 
%type <itrs> iterator iterators for_header 
%type <stmts> statement statements decl_stmt defn_stmt model_stmt model_stmts
%type <fpl> formal_params formal_param
%type <ppl> pos_params pos_param indexes index
%type <npl> named_params named_param 
%type <varlist> model_var_decl
%type <tuple_ids> tupleidlist
*/


%left COMMA
%left COLON
%nonassoc DOTDOT
%left OR
%left AND
%left EQUALS NEQUAL
%left GT GE LT LE
%left PLUS MINUS
%left TIMES DIVIDE
%right NOT UMINUS
%nonassoc ARROW
%right NEGSIGN
%nonassoc LPAR RPAR

/*-----------------------------------------------------------------*/
%%

/*==================================================================\
|                                                                   |
|                       Top-level  statements                       |        
|                                                                   |
\==================================================================*/

statements 
	: 	statements statement
{
#ifdef PARSE_TRACE
  cout << "Reducing statements : statements statement\n";
#endif
}
	|	statement
{
#ifdef PARSE_TRACE
  cout << "Reducing statements : statement\n";
#endif
}
	;

statement 
        :       for_header LBRACE statements RBRACE
{
#ifdef PARSE_TRACE
  cout << "Reducing statement : for_header LBRACE statements RBRACE\n";
#endif
}
	|	converge LBRACE statements RBRACE
{ 
#ifdef PARSE_TRACE
  cout << "Reducing statement : converge LBRACE statements RBRACE\n";
#endif
}
	|	decl_stmt
{ 
#ifdef PARSE_TRACE
  cout << "Reducing statement : decl_stmt\n";
#endif
}
	|	defn_stmt
{ 
#ifdef PARSE_TRACE
  cout << "Reducing statement : defn_stmt\n";
#endif
}
        |       model_decl
{ 
#ifdef PARSE_TRACE
  cout << "Reducing statement : model_decl\n";
#endif
} 
        |       opt_header const_expr ENDPND 
{
#ifdef PARSE_TRACE
  cout << "Reducing statement : opt_header const_expr ENDPND\n";
#endif
}
        |       opt_header IDENT ENDPND
{
#ifdef PARSE_TRACE
  cout << "Reducing statement : opt_header IDENT ENDPND\n";
#endif
}       
        |       opt_header LBRACE tupleidlist RBRACE ENDPND
{
#ifdef PARSE_TRACE
  cout << "Reducing statement : opt_header LBRACE tipleidlist RBRACE ENDPND\n";
#endif
}       
        |       expr SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing statement : expr SEMI\n";
#endif
}
        |       SEMI
{  
#ifdef PARSE_TRACE
  cout << "Reducing statement : SEMI\n";
#endif
}
	;

tupleidlist
        :       tupleidlist COMMA IDENT
{
#ifdef PARSE_TRACE
  cout << "Reducing tupleidlist : tupleidlist COMMA IDENT\n";
#endif
}
        |       IDENT
{
#ifdef PARSE_TRACE
  cout << "Reducing tupleidlist : IDENT\n";
#endif
}
	;

for_header
        : 	FOR LPAR iterators RPAR
{
#ifdef PARSE_TRACE
  cout << "Reducing for_header : FOR LPAR iterators RPAR\n";
#endif
}
	;

converge
        :       CONVERGE
{
#ifdef PARSE_TRACE
  cout << "Reducing converge : CONVERGE\n";
#endif
}
	;

opt_header
        :       POUND IDENT
{
#ifdef PARSE_TRACE
  cout << "Reducing opt_header : POUND IDENT\n";
#endif
}
	;

iterators
	:	iterators COMMA iterator 
{
#ifdef PARSE_TRACE
  cout << "Reducing iterators : iterators COMMA iterator\n";
#endif
}
	|	iterator
{ 
#ifdef PARSE_TRACE
  cout << "Reducing iterators : iterator\n";
#endif
}
	;

iterator
	:	type IDENT IN set_expr
{
#ifdef PARSE_TRACE
  cout << "Reducing iterator : type IDENT IN set_expr\n";
#endif
}
	;

type
        :       MODIF TYPE
{
#ifdef PARSE_TRACE
  cout << "Reducing type : MODIF TYPE\n";
#endif
}
        |       TYPE
{
#ifdef PARSE_TRACE
  cout << "Reducing type : TYPE\n";
#endif
}
;

set_expr
	:	LBRACE set_elems RBRACE
{
#ifdef PARSE_TRACE
  cout << "Reducing set_expr : LBRACE set_elems RBRACE\n";
#endif
}
	;

set_elems
	:	set_elems COMMA set_elem
{
#ifdef PARSE_TRACE
  cout << "Reducing set_elems : set_elems COMMA set_elem\n";
#endif
}
	|	set_elem
{
#ifdef PARSE_TRACE
  cout << "Reducing set_elems : set_elem\n";
#endif
}
	;

set_elem
	:	expr
{
#ifdef PARSE_TRACE
  cout << "Reducing set_elem : expr\n";
#endif
}
	|	expr DOTDOT expr
{
#ifdef PARSE_TRACE
  cout << "Reducing set_elem : expr DOTDOT expr\n";
#endif
}
	|	expr DOTDOT expr DOTDOT expr
{
#ifdef PARSE_TRACE
  cout << "Reducing set_elem : expr DOTDOT expr DOTDOT expr\n";
#endif
}
	;

/*==================================================================\
|                                                                   |
|                      Top-level  declarations                      |        
|                                                                   |
\==================================================================*/

decl_stmt 
        :       header SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing decl_stmt : header SEMI\n";
#endif
}
	|	array_header SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing decl_stmt : array_header SEMI\n";
#endif
}
	;

defn_stmt
	:	header GETS expr SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing defn_stmt : header GETS expr SEMI\n";
#endif
}
	|	type IDENT GETS expr SEMI 
{
#ifdef PARSE_TRACE
  cout << "Reducing defn_stmt : type IDENT GETS expr SEMI\n";
#endif
}
	|	type IDENT GUESS expr SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing defn_stmt : type IDENT GUESS expr SEMI\n";
#endif
}
	|	array_header GETS expr SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing defn_stmt : array_header GETS expr SEMI\n";
#endif
}
	|	array_header GUESS expr SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing defn_stmt : array_header GUESS expr SEMI\n";
#endif
}
;

/*==================================================================\
|                                                                   |
|                      Function/Array  Headers                      |        
|                                                                   |
\==================================================================*/

header
	:	type IDENT LPAR formal_params RPAR
{
#ifdef PARSE_TRACE
  cout << "Reducing header : type IDENT LPAR formal_params RPAR\n";
#endif
}
        ;

array_header
        :       type IDENT indexes 
{
#ifdef PARSE_TRACE
  cout << "Reducing array_header : type IDENT indexes\n";
#endif
}
        ;

/*==================================================================\
|                                                                   |
|                        Model  declarations                        |        
|                                                                   |
\==================================================================*/

model_decl
	:	model_header GETS LBRACE model_stmts RBRACE SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing model_decl : model_header GETS LBRACE model_stmts RBRACE SEMI\n";
#endif
}
	; 


model_header
	:	MODEL IDENT LPAR formal_params RPAR
{
#ifdef PARSE_TRACE
  cout << "Reducing model_header : MODEL IDENT LPAR formal_params RPAR\n";
#endif
}
        |       MODEL IDENT
{
#ifdef PARSE_TRACE
  cout << "Reducing model_header : MODEL IDENT\n";
#endif
}
        ;

model_stmts
        :       model_stmts model_stmt
{
#ifdef PARSE_TRACE
  cout << "Reducing model_stmts : model_stmts model_stmt\n";
#endif
}
        |       model_stmt
{
#ifdef PARSE_TRACE
  cout << "Reducing model_stmts : model_stmt\n";
#endif
}
        ;

model_stmt
        :       function_call SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing model_stmt : function_call SEMI\n";
#endif
}
	|	model_var_decl SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing model_stmt : model_var_decl SEMI\n";
#endif
}
	|	defn_stmt 
{
#ifdef PARSE_TRACE
  cout << "Reducing model_stmt : defn_stmt\n";
#endif
}
        |       for_header LBRACE model_stmts RBRACE
{
#ifdef PARSE_TRACE
  cout << "Reducing model_stmt : for_header LBRACE model_stmts RBRACE\n";
#endif
}
        ;

model_var_decl
        :       model_var_decl COMMA IDENT    
{
#ifdef PARSE_TRACE
  cout << "Reducing model_var_decl : model_var_decl COMMA IDENT\n";
#endif
}
        |       model_var_decl COMMA IDENT indexes
{
#ifdef PARSE_TRACE
  cout << "Reducing model_var_decl : model_var_decl COMMA IDENT indexes\n";
#endif
}
        |       type IDENT    
{
#ifdef PARSE_TRACE
  cout << "Reducing model_var_decl : type IDENT\n";
#endif
}
	|	type IDENT indexes
{
#ifdef PARSE_TRACE
  cout << "Reducing model_var_decl : type IDENT indexes\n";
#endif
}
        ;


/*==================================================================\
|                                                                   |
|                            Expressions                            |        
|                                                                   |
\==================================================================*/

expr 
	:	NUL
{  
#ifdef PARSE_TRACE
  cout << "Reducing expr : NUL\n";
#endif
  $$ = NULL;
}
        |       term      
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : term\n";
#endif
}
        |       model_function_call
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : model_function_call\n";
#endif
}
	|	function_call
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : function_call\n";
#endif
}
	|	LPAR expr RPAR
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : LPAR expr RPAR\n";
#endif
   $$ = $2;
}
	|	expr PLUS expr
{  
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr PLUS expr\n";
#endif
}
	| 	expr MINUS expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr MINUS expr\n";
#endif
}
	|	expr TIMES expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr TIMES expr\n";
#endif
}
	|	expr DIVIDE expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr DIVIDE expr\n";
#endif
}
	|	expr OR expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr OR expr\n";
#endif
}
	|	expr AND expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr AND expr\n";
#endif
}
	|	NOT expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : NOT expr\n";
#endif
}
	|	MINUS expr %prec UMINUS
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : MINUS expr\n";
#endif
}
	|	expr EQUALS expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr EQUALS expr\n";
#endif
}
	|	expr NEQUAL expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr NEQUAL expr\n";
#endif
}
	|	expr GT expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr GT expr\n";
#endif
}
	|	expr GE	expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr GE expr\n";
#endif
}
	|	expr LT expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr LT expr\n";
#endif
}
	|	expr LE expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr LE expr\n";
#endif
}
	|	type LPAR expr RPAR
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : type LPAR expr RPAR\n";
#endif
}
        |       expr COLON expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr COLON expr\n";
#endif
}
        ;



term      
        :       BOOLCONST
{
#ifdef PARSE_TRACE
  cout << "Reducing term : BOOLCONST\n";
#endif
} 
	|	INTCONST
{
#ifdef PARSE_TRACE
  cout << "Reducing term : INTCONST\n";
#endif
}
        |       REALCONST
{
#ifdef PARSE_TRACE
  cout << "Reducing term : REALCONST\n";
#endif
}
        |       STRCONST
{
#ifdef PARSE_TRACE
  cout << "Reducing term : STRCONST\n";
#endif
}
        ;

const_expr
	:	term
{
#ifdef PARSE_TRACE
  cout << "Reducing const_expr : term\n";
#endif
}
	|	MINUS const_expr %prec UMINUS
{
#ifdef PARSE_TRACE
  cout << "Reducing const_expr : MINUS const_expr\n";
#endif
}
	|	NOT const_expr
{
#ifdef PARSE_TRACE
  cout << "Reducing const_expr : NOT const_expr\n";
#endif
}
	;

/*==================================================================\
|                                                                   |
|                            Model calls                            |        
|                                                                   |
\==================================================================*/

model_function_call
        :       model_call DOT IDENT
{  
#ifdef PARSE_TRACE
  cout << "Reducing model_function_call : model_call DOT IDENT\n";
#endif
}
	|	model_call DOT IDENT indexes
{
#ifdef PARSE_TRACE
  cout << "Reducing model_function_call : model_call DOT IDENT indexes\n";
#endif
}
        ;

model_call
        :       IDENT
{ 
#ifdef PARSE_TRACE
  cout << "Reducing model_call : IDENT\n";
#endif
}
        |       IDENT LPAR pos_params RPAR
{ 
#ifdef PARSE_TRACE
  cout << "Reducing model_call : IDENT LPAR pos_params RPAR\n";
#endif
}
        |       IDENT LPAR named_params RPAR
{
#ifdef PARSE_TRACE
  cout << "Reducing model_call : IDENT LPAR named_params RPAR\n";
#endif
}
	;

/*==================================================================\
|                                                                   |
|                          Function  calls                          |        
|                                                                   |
\==================================================================*/

function_call 
	: 	IDENT 
{
#ifdef PARSE_TRACE
  cout << "Reducing function_call : IDENT\n";
#endif
}
	|	IDENT indexes	 
{
#ifdef PARSE_TRACE
  cout << "Reducing function_call : IDENT indexes\n";
#endif
}
	|	IDENT LPAR pos_params RPAR
{
#ifdef PARSE_TRACE
  cout << "Reducing function_call : IDENT LPAR pos_params RPAR\n";
#endif
}
        |       IDENT LPAR named_params RPAR
{
#ifdef PARSE_TRACE
  cout << "Reducing function_call : IDENT LPAR named_params RPAR\n";
#endif
}
	;

/*==================================================================\
|                                                                   |
|                          Parameter lists                          |        
|                                                                   |
\==================================================================*/

formal_params 
	: 	formal_params COMMA formal_param
{
#ifdef PARSE_TRACE
  cout << "Reducing formal_params : formal_params COMMA formal_param\n";
#endif
}
	|	formal_param
{
#ifdef PARSE_TRACE
  cout << "Reducing formal_params : formal_param\n";
#endif
}
	;

formal_param 
	:	type IDENT
{
#ifdef PARSE_TRACE
  cout << "Reducing formal_param : type IDENT\n";
#endif
}
	|	type IDENT GETS expr
{
#ifdef PARSE_TRACE
  cout << "Reducing formal_param : type IDENT GETS expr\n";
#endif
}
	;

indexes
	:	indexes LBRAK index RBRAK
{
#ifdef PARSE_TRACE
  cout << "Reducing indexes : indexes LBRAK index RBRAK\n";
#endif
}
	|	LBRAK index RBRAK
{
#ifdef PARSE_TRACE
  cout << "Reducing indexes : LBRAK index RBRAK\n";
#endif
}
	;

index
        :       expr
{
#ifdef PARSE_TRACE
  cout << "Reducing index : expr\n";
#endif
}
        ;


pos_params 
	:	pos_params COMMA pos_param
{
#ifdef PARSE_TRACE
  cout << "Reducing pos_params : pos_params COMMA pos_param\n";
#endif
}
	|	pos_param
{
#ifdef PARSE_TRACE
  cout << "Reducing pos_params : pos_param\n";
#endif
}
	;

pos_param 
	:	expr  
{
#ifdef PARSE_TRACE
  cout << "Reducing pos_param : expr\n";
#endif
}
	|	DEFAULT
{
#ifdef PARSE_TRACE
  cout << "Reducing pos_param : DEFAULT\n";
#endif
}
	;

named_params 
	:	named_params COMMA named_param
{
#ifdef PARSE_TRACE
  cout << "Reducing named_params : named_params COMMA named_param\n";
#endif
}
	|	named_param
{
#ifdef PARSE_TRACE
  cout << "Reducing named_params : named_param\n";
#endif
}
	;

named_param 
	:	IDENT GETS expr
{
#ifdef PARSE_TRACE
  cout << "Reducing named_param : IDENT GETS expr\n";
#endif
}
	|	IDENT GETS DEFAULT
{
#ifdef PARSE_TRACE
  cout << "Reducing named_param : IDENT GETS DEFAULT\n";
#endif
}
	;



%%
/*-----------------------------------------------------------------*/



