
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

#include "../../Source/Main/compile.h" // compile-time functionality 

#include <string.h>
#include <stdio.h>
#include <stdlib.h>  // keeps compiler happy

// #define PARSE_TRACE

%}

%union {
  char* name;
  type Type_ID;
  expr* Expr;
  void* list;   
  array_index* index;
  formal_param* fparam;
  named_param* nparam;
  int count;
  statement* stmt;
  array* Array;
  user_func* Func;
  option *Option;
  /*
  expr_set *setexpr;
  List <local_iterator> *itrs;
  List <statement> *stmts;
  function *Func;
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

%type <Type_ID> type model_var_decl
%type <Expr> topexpr expr term const_expr function_call model_call
model_function_call set_expr set_elems set_elem pos_param index
%type <list> aggexpr statements model_stmt model_stmts
formal_params formal_indexes pos_params named_params indexes
%type <index> iterator
%type <fparam> formal_param
%type <nparam> named_param
%type <count> for_header iterators
%type <stmt> statement defn_stmt
%type <Array> array_header
%type <Func> func_header
%type <Option> opt_header
/*
%type <setexpr> set_expr set_elems set_elem 
%type <itrs> iterator iterators for_header 
%type <stmts> statement statements decl_stmt defn_stmt model_stmt model_stmts
%type <npl> named_params named_param 
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
  $$ = AppendStatement($1, $2);
}
	|	statement
{
#ifdef PARSE_TRACE
  cout << "Reducing statements : statement\n";
#endif
  $$ = AppendStatement(NULL, $1);
}
	;

statement 
        :       for_header LBRACE statements RBRACE
{
#ifdef PARSE_TRACE
  cout << "Reducing statement : for_header LBRACE statements RBRACE\n";
#endif
  $$ = BuildForLoop($1, $3);
}
	|	converge LBRACE statements RBRACE
{ 
#ifdef PARSE_TRACE
  cout << "Reducing statement : converge LBRACE statements RBRACE\n";
#endif
  $$ = NULL;
}
	|	decl_stmt
{ 
#ifdef PARSE_TRACE
  cout << "Reducing statement : decl_stmt\n";
#endif
  $$ = NULL;
}
	|	defn_stmt
{ 
#ifdef PARSE_TRACE
  cout << "Reducing statement : defn_stmt\n";
#endif
  $$ = $1;
}
        |       model_decl
{ 
#ifdef PARSE_TRACE
  cout << "Reducing statement : model_decl\n";
#endif
  $$ = NULL;
} 
        |       opt_header const_expr ENDPND 
{
#ifdef PARSE_TRACE
  cout << "Reducing statement : opt_header topexpr ENDPND\n";
#endif
  $$ = BuildOptionStatement($1, $2);
}
        |       opt_header IDENT ENDPND
{
#ifdef PARSE_TRACE
  cout << "Reducing statement : opt_header IDENT ENDPND\n";
#endif
  $$ = BuildOptionStatement($1, $2);
}       
        |       opt_header LBRACE tupleidlist RBRACE ENDPND
{
#ifdef PARSE_TRACE
  cout << "Reducing statement : opt_header LBRACE tipleidlist RBRACE ENDPND\n";
#endif
  $$ = NULL;
}       
        |       expr SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing statement : expr SEMI\n";
#endif
  $$ = BuildExprStatement($1);
}
        |       SEMI
{  
#ifdef PARSE_TRACE
  cout << "Reducing statement : SEMI\n";
#endif
  $$ = NULL;
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
 $$ = $3;  
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
  $$ = BuildOptionHeader($2);
}
	;

iterators
	:	iterators COMMA iterator 
{
#ifdef PARSE_TRACE
  cout << "Reducing iterators : iterators COMMA iterator\n";
#endif
  $$ = $1 + AddIterator($3);
}
	|	iterator
{ 
#ifdef PARSE_TRACE
  cout << "Reducing iterators : iterator\n";
#endif
  $$ = AddIterator($1);
}
	;

iterator
	:	type IDENT IN set_expr
{
#ifdef PARSE_TRACE
  cout << "Reducing iterator : type IDENT IN set_expr\n";
#endif
  $$ = BuildIterator($1, $2, $4);
}
	;

type
        :       MODIF TYPE
{
#ifdef PARSE_TRACE
  cout << "Reducing type : MODIF TYPE\n";
#endif
  $$ = MakeType($1, $2);
}
        |       TYPE
{
#ifdef PARSE_TRACE
  cout << "Reducing type : TYPE\n";
#endif
  $$ = MakeType(NULL, $1);
}
;

set_expr
	:	LBRACE set_elems RBRACE
{
#ifdef PARSE_TRACE
  cout << "Reducing set_expr : LBRACE set_elems RBRACE\n";
#endif
  $$ = $2;
}
	;

set_elems
	:	set_elems COMMA set_elem
{
#ifdef PARSE_TRACE
  cout << "Reducing set_elems : set_elems COMMA set_elem\n";
#endif
  $$ = AppendSetElem($1, $3);
}
	|	set_elem
{
#ifdef PARSE_TRACE
  cout << "Reducing set_elems : set_elem\n";
#endif
  $$ = $1;
}
	;

set_elem
	:	expr
{
#ifdef PARSE_TRACE
  cout << "Reducing set_elem : expr\n";
#endif
  $$ = MakeElementSet(Filename(), LineNumber(), $1);
}
	|	expr DOTDOT expr
{
#ifdef PARSE_TRACE
  cout << "Reducing set_elem : expr DOTDOT expr\n";
#endif
  $$ = BuildInterval($1, $3); 
}
	|	expr DOTDOT expr DOTDOT expr
{
#ifdef PARSE_TRACE
  cout << "Reducing set_elem : expr DOTDOT expr DOTDOT expr\n";
#endif
  $$ = BuildInterval($1, $3, $5);
}
	;

/*==================================================================\
|                                                                   |
|                      Top-level  declarations                      |        
|                                                                   |
\==================================================================*/

decl_stmt 
        :       func_header SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing decl_stmt : func_header SEMI\n";
#endif
  DoneWithFunctionHeader($1);
}
	|	array_header SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing decl_stmt : array_header SEMI\n";
#endif
}
	;

defn_stmt
	:	func_header GETS expr SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing defn_stmt : func_header GETS expr SEMI\n";
#endif
  $$ = BuildFuncStmt($1, $3);
}
	|	type IDENT GETS expr SEMI 
{
#ifdef PARSE_TRACE
  cout << "Reducing defn_stmt : type IDENT GETS expr SEMI\n";
#endif
  $$ = BuildVarStmt($1, $2, $4);
}
	|	type IDENT GUESS expr SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing defn_stmt : type IDENT GUESS expr SEMI\n";
#endif
  $$ = NULL;
}
	|	array_header GETS expr SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing defn_stmt : array_header GETS expr SEMI\n";
#endif
  $$ = BuildArrayStmt($1, $3);
}
	|	array_header GUESS expr SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing defn_stmt : array_header GUESS expr SEMI\n";
#endif
  $$ = NULL;
}
;

/*==================================================================\
|                                                                   |
|                      Function/Array  Headers                      |        
|                                                                   |
\==================================================================*/

func_header
	:	type IDENT LPAR formal_params RPAR
{
#ifdef PARSE_TRACE
  cout << "Reducing func_header : type IDENT LPAR formal_params RPAR\n";
#endif
  $$ = BuildFunction($1, $2, $4);
}
        ;

array_header
        :       type IDENT formal_indexes 
{
#ifdef PARSE_TRACE
  cout << "Reducing array_header : type IDENT formal_indexes\n";
#endif
  $$ = BuildArray($1, $2, $3);
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
  $$ = $1;
}
        |       model_var_decl COMMA IDENT formal_indexes
{
#ifdef PARSE_TRACE
  cout << "Reducing model_var_decl : model_var_decl COMMA IDENT formal_indexes\n";
#endif
  $$ = $1;
}
        |       type IDENT    
{
#ifdef PARSE_TRACE
  cout << "Reducing model_var_decl : type IDENT\n";
#endif
  $$ = $1;
}
	|	type IDENT formal_indexes
{
#ifdef PARSE_TRACE
  cout << "Reducing model_var_decl : type IDENT formal_indexes\n";
#endif
  $$ = $1;
}
        ;


/*==================================================================\
|                                                                   |
|                            Expressions                            |        
|                                                                   |
\==================================================================*/

topexpr
	:	expr
{
#ifdef PARSE_TRACE
  cout << "Reducing topexpr : expr\n";
#endif
  Optimize(0, $1);
  $$ = $1;
}
	|	aggexpr
{
#ifdef PARSE_TRACE
  cout << "Reducing topexpr : aggexpr\n";
#endif
  $$ = BuildAggregate($1);
}
	;


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
  $$ = $1;
}
        |       model_function_call
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : model_function_call\n";
#endif
  $$ = $1;
}
	|	function_call
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : function_call\n";
#endif
  $$ = $1;
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
  $$ = BuildBinary($1, PLUS, $3);
}
	| 	expr MINUS expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr MINUS expr\n";
#endif
  $$ = BuildBinary($1, MINUS, $3);
}
	|	expr TIMES expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr TIMES expr\n";
#endif
  $$ = BuildBinary($1, TIMES, $3);
}
	|	expr DIVIDE expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr DIVIDE expr\n";
#endif
  $$ = BuildBinary($1, DIVIDE, $3);
}
	|	expr OR expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr OR expr\n";
#endif
  $$ = BuildBinary($1, OR, $3);
}
	|	expr AND expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr AND expr\n";
#endif
  $$ = BuildBinary($1, AND, $3);
}
	|	expr EQUALS expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr EQUALS expr\n";
#endif
  $$ = BuildBinary($1, EQUALS, $3);
}
	|	expr NEQUAL expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr NEQUAL expr\n";
#endif
  $$ = BuildBinary($1, NEQUAL, $3);
}
	|	expr GT expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr GT expr\n";
#endif
  $$ = BuildBinary($1, GT, $3);
}
	|	expr GE	expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr GE expr\n";
#endif
  $$ = BuildBinary($1, GE, $3);
}
	|	expr LT expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr LT expr\n";
#endif
  $$ = BuildBinary($1, LT, $3);
}
	|	expr LE expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr LE expr\n";
#endif
  $$ = BuildBinary($1, LE, $3);
}
	|	NOT expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : NOT expr\n";
#endif
  $$ = BuildUnary(NOT, $2);
}
	|	MINUS expr %prec UMINUS
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : MINUS expr\n";
#endif
  $$ = BuildUnary(MINUS, $2);
}
	|	type LPAR expr RPAR
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : type LPAR expr RPAR\n";
#endif
  $$ = BuildTypecast($3, $1);
}
	;


aggexpr 
	:	aggexpr COLON expr
{
#ifdef PARSE_TRACE
  cout << "Reducing aggexpr : aggexpr COLON expr\n";
#endif
  $$ = AddAggregate($1, $3);
}
        |       expr COLON expr
{
#ifdef PARSE_TRACE
  cout << "Reducing aggexpr : expr COLON expr\n";
#endif
  $$ = StartAggregate($1, $3);
}
        ;



term      
        :       BOOLCONST
{
#ifdef PARSE_TRACE
  cout << "Reducing term : BOOLCONST\n";
#endif
  $$ = MakeBoolConst($1);
} 
	|	INTCONST
{
#ifdef PARSE_TRACE
  cout << "Reducing term : INTCONST\n";
#endif
  $$ = MakeIntConst($1);
}
        |       REALCONST
{
#ifdef PARSE_TRACE
  cout << "Reducing term : REALCONST\n";
#endif
  $$ = MakeRealConst($1);
}
        |       STRCONST
{
#ifdef PARSE_TRACE
  cout << "Reducing term : STRCONST\n";
#endif
  $$ = MakeStringConst($1);
}
        ;

const_expr
	:	term
{
#ifdef PARSE_TRACE
  cout << "Reducing const_expr : term\n";
#endif
  $$ = $1;
}
	|	MINUS const_expr %prec UMINUS
{
#ifdef PARSE_TRACE
  cout << "Reducing const_expr : MINUS const_expr\n";
#endif
  $$ = BuildUnary(MINUS, $2);
}
	|	NOT const_expr
{
#ifdef PARSE_TRACE
  cout << "Reducing const_expr : NOT const_expr\n";
#endif
  $$ = BuildUnary(NOT, $2);
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
  $$ = NULL;
}
	|	model_call DOT IDENT indexes
{
#ifdef PARSE_TRACE
  cout << "Reducing model_function_call : model_call DOT IDENT indexes\n";
#endif
  $$ = NULL;
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
  $$ = FindIdent($1);
}
	|	IDENT indexes	 
{
#ifdef PARSE_TRACE
  cout << "Reducing function_call : IDENT indexes\n";
#endif
  $$ = BuildArrayCall($1, $2);
}
	|	IDENT LPAR pos_params RPAR
{
#ifdef PARSE_TRACE
  cout << "Reducing function_call : IDENT LPAR pos_params RPAR\n";
#endif
  $$ = BuildFunctionCall($1, $3);
}
        |       IDENT LPAR named_params RPAR
{
#ifdef PARSE_TRACE
  cout << "Reducing function_call : IDENT LPAR named_params RPAR\n";
#endif
  $$ = BuildNamedFunctionCall($1, $3);
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
  $$ = AddParameter($1, $3);
}
	|	formal_param
{
#ifdef PARSE_TRACE
  cout << "Reducing formal_params : formal_param\n";
#endif
  $$ = AddParameter(NULL, $1);
}
	;

formal_param 
	:	type IDENT
{
#ifdef PARSE_TRACE
  cout << "Reducing formal_param : type IDENT\n";
#endif
  $$ = BuildFormal($1, $2);
}
	|	type IDENT GETS expr
{
#ifdef PARSE_TRACE
  cout << "Reducing formal_param : type IDENT GETS expr\n";
#endif
  $$ = BuildFormal($1, $2, $4);
}
	;

formal_indexes
	:	formal_indexes LBRAK IDENT RBRAK
{
#ifdef PARSE_TRACE
  cout << "Reducing formal_indexes : formal_indexes LBRAK IDENT RBRAK\n";
#endif
  $$ = AddFormalIndex($1, $3);
}
	|	LBRAK IDENT RBRAK
{
#ifdef PARSE_TRACE
  cout << "Reducing formal_indexes : LBRAK IDENT RBRAK\n";
#endif
  $$ = AddFormalIndex(NULL, $2);
}
	;


indexes
	:	indexes LBRAK index RBRAK
{
#ifdef PARSE_TRACE
  cout << "Reducing indexes : indexes LBRAK index RBRAK\n";
#endif
  $$ = AddParameter($1, $3);
}
	|	LBRAK index RBRAK
{
#ifdef PARSE_TRACE
  cout << "Reducing indexes : LBRAK index RBRAK\n";
#endif
  $$ = AddParameter(NULL, $2);
}
	;

index
        :       expr
{
#ifdef PARSE_TRACE
  cout << "Reducing index : expr\n";
#endif
  $$ = $1;
}
        ;


pos_params 
	:	pos_params COMMA pos_param
{
#ifdef PARSE_TRACE
  cout << "Reducing pos_params : pos_params COMMA pos_param\n";
#endif
  $$ = AddParameter($1, $3);
}
	|	pos_param
{
#ifdef PARSE_TRACE
  cout << "Reducing pos_params : pos_param\n";
#endif
  $$ = AddParameter(NULL, $1);
}
	;

pos_param 
	:	topexpr  
{
#ifdef PARSE_TRACE
  cout << "Reducing pos_param : expr\n";
#endif
  $$ = $1;
}
	|	DEFAULT
{
#ifdef PARSE_TRACE
  cout << "Reducing pos_param : DEFAULT\n";
#endif
  $$ = DEFLT;
}
	;

named_params 
	:	named_params COMMA named_param
{
#ifdef PARSE_TRACE
  cout << "Reducing named_params : named_params COMMA named_param\n";
#endif
  $$ = AddParameter($1, $3);
}
	|	named_param
{
#ifdef PARSE_TRACE
  cout << "Reducing named_params : named_param\n";
#endif
  $$ = AddParameter(NULL, $1);
}
	;

named_param 
	:	IDENT GETS topexpr
{
#ifdef PARSE_TRACE
  cout << "Reducing named_param : IDENT GETS expr\n";
#endif
  $$ = BuildNamedParam($1, $3);
}
	|	IDENT GETS DEFAULT
{
#ifdef PARSE_TRACE
  cout << "Reducing named_param : IDENT GETS DEFAULT\n";
#endif
  $$ = BuildNamedDefault($1);
}
	;



%%
/*-----------------------------------------------------------------*/



