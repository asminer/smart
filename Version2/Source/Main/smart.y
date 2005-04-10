
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

//#define PARSE_TRACE

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
  option* Option;
  model* Model;
  func_call* Mcall;
}

%token <name> IDENT BOOLCONST INTCONST REALCONST STRCONST 
LPAR RPAR LBRAK RBRAK LBRACE RBRACE 
COMMA SEMI COLON POUND DOT DOTDOT GETS PLUS MINUS TIMES DIVIDE OR AND NOT 
EQUALS NEQUAL GT GE LT LE ENDPND FOR END CONVERGE IN GUESS
NUL DEFAULT TYPE MODIF MODEL 

%type <Type_ID> type model
%type <Expr> topexpr expr term const_expr function_call
model_function_call set_expr set_elems set_elem pos_param index
%type <list> aggexpr seqexpr statements model_stmts model_var_list
formal_params formal_indexes pos_params named_params indexes
%type <index> iterator
%type <fparam> formal_param
%type <nparam> named_param
%type <count> for_header iterators
%type <stmt> statement defn_stmt model_stmt
%type <Array> array_header
%type <Func> func_header
%type <Model> model_header
%type <Option> opt_header
%type <Mcall> model_call

%left SEMI
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
  Output << "Reducing statements : statements statement\n";
  Output.flush();
#endif
  $$ = AppendStatement($1, $2);
}
	|	statement
{
#ifdef PARSE_TRACE
  Output << "Reducing statements : statement\n";
  Output.flush();
#endif
  $$ = AppendStatement(NULL, $1);
}
	;

statement 
        :       for_header LBRACE statements RBRACE
{
#ifdef PARSE_TRACE
  Output << "Reducing statement : for_header LBRACE statements RBRACE\n";
  Output.flush();
#endif
  $$ = BuildForLoop($1, $3);
}
	|	converge LBRACE statements RBRACE
{ 
#ifdef PARSE_TRACE
  Output << "Reducing statement : converge LBRACE statements RBRACE\n";
  Output.flush();
#endif
  $$ = FinishConverge($3);
}
	|	decl_stmt
{ 
#ifdef PARSE_TRACE
  Output << "Reducing statement : decl_stmt\n";
  Output.flush();
#endif
  $$ = NULL;
}
	|	defn_stmt
{ 
#ifdef PARSE_TRACE
  Output << "Reducing statement : defn_stmt\n";
  Output.flush();
#endif
  $$ = $1;
}
        |       model_decl
{ 
#ifdef PARSE_TRACE
  Output << "Reducing statement : model_decl\n";
  Output.flush();
#endif
  $$ = NULL;
} 
        |       opt_header const_expr ENDPND 
{
#ifdef PARSE_TRACE
  Output << "Reducing statement : opt_header const_expr ENDPND\n";
  Output.flush();
#endif
  $$ = BuildOptionStatement($1, $2);
}
        |       opt_header IDENT ENDPND
{
#ifdef PARSE_TRACE
  Output << "Reducing statement : opt_header IDENT ENDPND\n";
  Output.flush();
#endif
  $$ = BuildOptionStatement($1, $2);
}       
        |       opt_header LBRACE tupleidlist RBRACE ENDPND
{
#ifdef PARSE_TRACE
  Output << "Reducing statement : opt_header LBRACE tipleidlist RBRACE ENDPND\n";
  Output.flush();
#endif
  $$ = NULL;
}       
        |       expr SEMI
{
#ifdef PARSE_TRACE
  Output << "Reducing statement : expr SEMI\n";
  Output.flush();
#endif
  $$ = BuildExprStatement($1);
}
        |       SEMI
{  
#ifdef PARSE_TRACE
  Output << "Reducing statement : SEMI\n";
  Output.flush();
#endif
  $$ = NULL;
}
	;

tupleidlist
        :       tupleidlist COMMA IDENT
{
#ifdef PARSE_TRACE
  Output << "Reducing tupleidlist : tupleidlist COMMA IDENT\n";
  Output.flush();
#endif
}
        |       IDENT
{
#ifdef PARSE_TRACE
  Output << "Reducing tupleidlist : IDENT\n";
  Output.flush();
#endif
}
	;

for_header
        : 	FOR LPAR iterators RPAR
{
#ifdef PARSE_TRACE
  Output << "Reducing for_header : FOR LPAR iterators RPAR\n";
  Output.flush();
#endif
 $$ = $3;  
}
	;

converge
        :       CONVERGE
{
#ifdef PARSE_TRACE
  Output << "Reducing converge : CONVERGE\n";
  Output.flush();
#endif
  StartConverge();
}
	;

opt_header
        :       POUND IDENT
{
#ifdef PARSE_TRACE
  Output << "Reducing opt_header : POUND IDENT\n";
  Output.flush();
#endif
  $$ = BuildOptionHeader($2);
}
	;

iterators
	:	iterators COMMA iterator 
{
#ifdef PARSE_TRACE
  Output << "Reducing iterators : iterators COMMA iterator\n";
  Output.flush();
#endif
  $$ = $1 + AddIterator($3);
}
	|	iterator
{ 
#ifdef PARSE_TRACE
  Output << "Reducing iterators : iterator\n";
  Output.flush();
#endif
  $$ = AddIterator($1);
}
	;

iterator
	:	type IDENT IN set_expr
{
#ifdef PARSE_TRACE
  Output << "Reducing iterator : type IDENT IN set_expr\n";
  Output.flush();
#endif
  $$ = BuildIterator($1, $2, $4);
}
	;

type
        :       MODIF TYPE
{
#ifdef PARSE_TRACE
  Output << "Reducing type : MODIF TYPE\n";
  Output.flush();
#endif
  $$ = MakeType($1, $2);
}
        |       TYPE
{
#ifdef PARSE_TRACE
  Output << "Reducing type : TYPE\n";
  Output.flush();
#endif
  $$ = MakeType(NULL, $1);
}
;

model
	:	MODEL
{
#ifdef PARSE_TRACE
  Output << "Reducing model : MODEL\n";
  Output.flush();
#endif
  $$ = MakeType(NULL, $1);
}
	;

set_expr
	:	LBRACE set_elems RBRACE
{
#ifdef PARSE_TRACE
  Output << "Reducing set_expr : LBRACE set_elems RBRACE\n";
  Output.flush();
#endif
  $$ = $2;
}
	;

set_elems
	:	set_elems COMMA set_elem
{
#ifdef PARSE_TRACE
  Output << "Reducing set_elems : set_elems COMMA set_elem\n";
  Output.flush();
#endif
  $$ = AppendSetElem($1, $3);
}
	|	set_elem
{
#ifdef PARSE_TRACE
  Output << "Reducing set_elems : set_elem\n";
  Output.flush();
#endif
  $$ = $1;
}
	;

set_elem
	:	expr
{
#ifdef PARSE_TRACE
  Output << "Reducing set_elem : expr\n";
  Output.flush();
#endif
  $$ = BuildElementSet($1);
}
	|	expr DOTDOT expr
{
#ifdef PARSE_TRACE
  Output << "Reducing set_elem : expr DOTDOT expr\n";
  Output.flush();
#endif
  $$ = BuildInterval($1, $3); 
}
	|	expr DOTDOT expr DOTDOT expr
{
#ifdef PARSE_TRACE
  Output << "Reducing set_elem : expr DOTDOT expr DOTDOT expr\n";
  Output.flush();
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
  Output << "Reducing decl_stmt : func_header SEMI\n";
  Output.flush();
#endif
  DoneWithFunctionHeader($1);
}
	|	array_header SEMI
{
#ifdef PARSE_TRACE
  Output << "Reducing decl_stmt : array_header SEMI\n";
  Output.flush();
#endif
}
	;

defn_stmt
	:	func_header GETS expr SEMI
{
#ifdef PARSE_TRACE
  Output << "Reducing defn_stmt : func_header GETS expr SEMI\n";
  Output.flush();
#endif
  $$ = BuildFuncStmt($1, $3);
}
	|	type IDENT GETS expr SEMI 
{
#ifdef PARSE_TRACE
  Output << "Reducing defn_stmt : type IDENT GETS expr SEMI\n";
  Output.flush();
#endif
  $$ = BuildVarStmt($1, $2, $4);
}
	|	type IDENT GUESS expr SEMI
{
#ifdef PARSE_TRACE
  Output << "Reducing defn_stmt : type IDENT GUESS expr SEMI\n";
  Output.flush();
#endif
  $$ = BuildGuessStmt($1, $2, $4);
}
	|	array_header GETS expr SEMI
{
#ifdef PARSE_TRACE
  Output << "Reducing defn_stmt : array_header GETS expr SEMI\n";
  Output.flush();
#endif
  $$ = BuildArrayStmt($1, $3);
}
	|	array_header GUESS expr SEMI
{
#ifdef PARSE_TRACE
  Output << "Reducing defn_stmt : array_header GUESS expr SEMI\n";
  Output.flush();
#endif
  $$ = BuildArrayGuess($1, $3);
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
  Output << "Reducing func_header : type IDENT LPAR formal_params RPAR\n";
  Output.flush();
#endif
  $$ = BuildFunction($1, $2, $4);
}
        ;

array_header
        :       type IDENT formal_indexes 
{
#ifdef PARSE_TRACE
  Output << "Reducing array_header : type IDENT formal_indexes\n";
  Output.flush();
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
  Output << "Reducing model_decl : model_header GETS LBRACE model_stmts RBRACE SEMI\n";
  Output.flush();
#endif
  BuildModelStmt($1, $4);
}
	; 


model_header
	:	model IDENT LPAR formal_params RPAR
{
#ifdef PARSE_TRACE
  Output << "Reducing model_header : model IDENT LPAR formal_params RPAR\n";
  Output.flush();
#endif
  $$ = BuildModel($1, $2, $4);
}
        |       model IDENT
{
#ifdef PARSE_TRACE
  Output << "Reducing model_header : model IDENT\n";
  Output.flush();
#endif
  $$ = BuildModel($1, $2, NULL);
}
        ;

model_stmts
        :       model_stmts model_stmt
{
#ifdef PARSE_TRACE
  Output << "Reducing model_stmts : model_stmts model_stmt\n";
  Output.flush();
#endif
  $$ = AppendStatement($1, $2);
}
        |       model_stmt
{
#ifdef PARSE_TRACE
  Output << "Reducing model_stmts : model_stmt\n";
  Output.flush();
#endif
  $$ = AppendStatement(NULL, $1);
}
        ;

model_stmt
        :       function_call SEMI
{
#ifdef PARSE_TRACE
  Output << "Reducing model_stmt : function_call SEMI\n";
  Output.flush();
#endif
  $$ = BuildExprStatement($1);
}
	|	type model_var_list SEMI
{
#ifdef PARSE_TRACE
  Output << "Reducing model_stmt : type model_var_list SEMI\n";
  Output.flush();
#endif
  $$ = BuildModelVarStmt($1, $2);
}
	|	defn_stmt 
{
#ifdef PARSE_TRACE
  Output << "Reducing model_stmt : defn_stmt\n";
  Output.flush();
#endif
  $$ = $1;
}
        |       for_header LBRACE model_stmts RBRACE
{
#ifdef PARSE_TRACE
  Output << "Reducing model_stmt : for_header LBRACE model_stmts RBRACE\n";
  Output.flush();
#endif
  $$ = BuildForLoop($1, $3);
}
        ;

model_var_list
        :       model_var_list COMMA IDENT    
{
#ifdef PARSE_TRACE
  Output << "Reducing model_var_list : model_var_list COMMA IDENT\n";
  Output.flush();
#endif
  $$ = AddModelVariable($1, $3);
}
        |       model_var_list COMMA IDENT formal_indexes
{
#ifdef PARSE_TRACE
  Output << "Reducing model_var_list : model_var_list COMMA IDENT formal_indexes\n";
  Output.flush();
#endif
  $$ = AddModelArray($1, $3, $4);
}
        |       IDENT    
{
#ifdef PARSE_TRACE
  Output << "Reducing model_var_list : IDENT\n";
  Output.flush();
#endif
  $$ = AddModelVariable(NULL, $1);
}
	|	IDENT formal_indexes
{
#ifdef PARSE_TRACE
  Output << "Reducing model_var_list : IDENT formal_indexes\n";
  Output.flush();
#endif
  $$ = AddModelArray(NULL, $1, $2);
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
  Output << "Reducing topexpr : expr\n";
  Output.flush();
#endif
  Optimize(0, $1);
  $$ = $1;
}
	|	aggexpr
{
#ifdef PARSE_TRACE
  Output << "Reducing topexpr : aggexpr\n";
  Output.flush();
#endif
  $$ = BuildAggregate($1);
}
	|	seqexpr
{
#ifdef PARSE_TRACE
  Output << "Reducing topexpr : seqexpr\n";
  Output.flush();
#endif
  $$ = BuildSequence($1);
}
	;


expr 
	:	NUL
{  
#ifdef PARSE_TRACE
  Output << "Reducing expr : NUL\n";
  Output.flush();
#endif
  $$ = NULL;
}
        |       term      
{
#ifdef PARSE_TRACE
  Output << "Reducing expr : term\n";
  Output.flush();
#endif
  $$ = $1;
}
	|	set_expr
{
#ifdef PARSE_TRACE
  Output << "Reducing expr : set_expr\n";
  Output.flush();
#endif
  $$ = $1;
}
        |       model_function_call
{
#ifdef PARSE_TRACE
  Output << "Reducing expr : model_function_call\n";
  Output.flush();
#endif
  $$ = $1;
}
	|	function_call
{
#ifdef PARSE_TRACE
  Output << "Reducing expr : function_call\n";
  Output.flush();
#endif
  $$ = $1;
}
	|	LPAR expr RPAR
{
#ifdef PARSE_TRACE
  Output << "Reducing expr : LPAR expr RPAR\n";
  Output.flush();
#endif
  $$ = $2;
}
	|	expr PLUS expr
{  
#ifdef PARSE_TRACE
  Output << "Reducing expr : expr PLUS expr\n";
  Output.flush();
#endif
  $$ = BuildBinary($1, PLUS, $3);
}
	| 	expr MINUS expr
{
#ifdef PARSE_TRACE
  Output << "Reducing expr : expr MINUS expr\n";
  Output.flush();
#endif
  $$ = BuildBinary($1, MINUS, $3);
}
	|	expr TIMES expr
{
#ifdef PARSE_TRACE
  Output << "Reducing expr : expr TIMES expr\n";
  Output.flush();
#endif
  $$ = BuildBinary($1, TIMES, $3);
}
	|	expr DIVIDE expr
{
#ifdef PARSE_TRACE
  Output << "Reducing expr : expr DIVIDE expr\n";
  Output.flush();
#endif
  $$ = BuildBinary($1, DIVIDE, $3);
}
	|	expr OR expr
{
#ifdef PARSE_TRACE
  Output << "Reducing expr : expr OR expr\n";
  Output.flush();
#endif
  $$ = BuildBinary($1, OR, $3);
}
	|	expr AND expr
{
#ifdef PARSE_TRACE
  Output << "Reducing expr : expr AND expr\n";
  Output.flush();
#endif
  $$ = BuildBinary($1, AND, $3);
}
	|	expr EQUALS expr
{
#ifdef PARSE_TRACE
  Output << "Reducing expr : expr EQUALS expr\n";
  Output.flush();
#endif
  $$ = BuildBinary($1, EQUALS, $3);
}
	|	expr NEQUAL expr
{
#ifdef PARSE_TRACE
  Output << "Reducing expr : expr NEQUAL expr\n";
  Output.flush();
#endif
  $$ = BuildBinary($1, NEQUAL, $3);
}
	|	expr GT expr
{
#ifdef PARSE_TRACE
  Output << "Reducing expr : expr GT expr\n";
  Output.flush();
#endif
  $$ = BuildBinary($1, GT, $3);
}
	|	expr GE	expr
{
#ifdef PARSE_TRACE
  Output << "Reducing expr : expr GE expr\n";
  Output.flush();
#endif
  $$ = BuildBinary($1, GE, $3);
}
	|	expr LT expr
{
#ifdef PARSE_TRACE
  Output << "Reducing expr : expr LT expr\n";
  Output.flush();
#endif
  $$ = BuildBinary($1, LT, $3);
}
	|	expr LE expr
{
#ifdef PARSE_TRACE
  Output << "Reducing expr : expr LE expr\n";
  Output.flush();
#endif
  $$ = BuildBinary($1, LE, $3);
}
	|	NOT expr
{
#ifdef PARSE_TRACE
  Output << "Reducing expr : NOT expr\n";
  Output.flush();
#endif
  $$ = BuildUnary(NOT, $2);
}
	|	MINUS expr %prec UMINUS
{
#ifdef PARSE_TRACE
  Output << "Reducing expr : MINUS expr\n";
  Output.flush();
#endif
  $$ = BuildUnary(MINUS, $2);
}
	|	type LPAR expr RPAR
{
#ifdef PARSE_TRACE
  Output << "Reducing expr : type LPAR expr RPAR\n";
  Output.flush();
#endif
  $$ = BuildTypecast($3, $1);
}
	;


aggexpr 
	:	aggexpr COLON expr
{
#ifdef PARSE_TRACE
  Output << "Reducing aggexpr : aggexpr COLON expr\n";
  Output.flush();
#endif
  $$ = AddAggregate($1, $3);
}
        |       expr COLON expr
{
#ifdef PARSE_TRACE
  Output << "Reducing aggexpr : expr COLON expr\n";
  Output.flush();
#endif
  $$ = StartAggregate($1, $3);
}
        ;


seqexpr 
	:	seqexpr SEMI expr
{
#ifdef PARSE_TRACE
  Output << "Reducing seqexpr : seqexpr SEMI expr\n";
  Output.flush();
#endif
  $$ = AddToSequence($1, $3);
}
        |       expr SEMI expr
{
#ifdef PARSE_TRACE
  Output << "Reducing seqexpr : expr SEMI expr\n";
  Output.flush();
#endif
  $$ = StartSequence($1, $3);
}
        ;



term      
        :       BOOLCONST
{
#ifdef PARSE_TRACE
  Output << "Reducing term : BOOLCONST\n";
  Output.flush();
#endif
  $$ = MakeBoolConst($1);
} 
	|	INTCONST
{
#ifdef PARSE_TRACE
  Output << "Reducing term : INTCONST\n";
  Output.flush();
#endif
  $$ = MakeIntConst($1);
}
        |       REALCONST
{
#ifdef PARSE_TRACE
  Output << "Reducing term : REALCONST\n";
  Output.flush();
#endif
  $$ = MakeRealConst($1);
}
        |       STRCONST
{
#ifdef PARSE_TRACE
  Output << "Reducing term : STRCONST\n";
  Output.flush();
#endif
  $$ = MakeStringConst($1);
}
        ;

const_expr
	:	term
{
#ifdef PARSE_TRACE
  Output << "Reducing const_expr : term\n";
  Output.flush();
#endif
  $$ = $1;
}
	|	MINUS const_expr %prec UMINUS
{
#ifdef PARSE_TRACE
  Output << "Reducing const_expr : MINUS const_expr\n";
  Output.flush();
#endif
  $$ = BuildUnary(MINUS, $2);
}
	|	NOT const_expr
{
#ifdef PARSE_TRACE
  Output << "Reducing const_expr : NOT const_expr\n";
  Output.flush();
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
  Output << "Reducing model_function_call : model_call DOT IDENT\n";
  Output.flush();
#endif
  $$ = MakeMCall($1, $3);
}
	|	model_call DOT IDENT indexes
{
#ifdef PARSE_TRACE
  Output << "Reducing model_function_call : model_call DOT IDENT indexes\n";
  Output.flush();
#endif
  $$ = MakeMACall($1, $3, $4);
}
        ;

model_call
        :       IDENT
{ 
#ifdef PARSE_TRACE
  Output << "Reducing model_call : IDENT\n";
  Output.flush();
#endif
  $$ = MakeModelCall($1);
}
        |       IDENT LPAR pos_params RPAR
{ 
#ifdef PARSE_TRACE
  Output << "Reducing model_call : IDENT LPAR pos_params RPAR\n";
  Output.flush();
#endif
  $$ = MakeModelCallPos($1, $3);
}
        |       IDENT LPAR named_params RPAR
{
#ifdef PARSE_TRACE
  Output << "Reducing model_call : IDENT LPAR named_params RPAR\n";
  Output.flush();
#endif
  $$ = MakeModelCallNamed($1, $3);
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
  Output << "Reducing function_call : IDENT\n";
  Output.flush();
#endif
  $$ = FindIdent($1);
}
	|	IDENT indexes	 
{
#ifdef PARSE_TRACE
  Output << "Reducing function_call : IDENT indexes\n";
  Output.flush();
#endif
  $$ = BuildArrayCall($1, $2);
}
	|	IDENT LPAR pos_params RPAR
{
#ifdef PARSE_TRACE
  Output << "Reducing function_call : IDENT LPAR pos_params RPAR\n";
  Output.flush();
#endif
  $$ = BuildFunctionCall($1, $3);
}
        |       IDENT LPAR named_params RPAR
{
#ifdef PARSE_TRACE
  Output << "Reducing function_call : IDENT LPAR named_params RPAR\n";
  Output.flush();
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
  Output << "Reducing formal_params : formal_params COMMA formal_param\n";
  Output.flush();
#endif
  $$ = AddParameter($1, $3);
}
	|	formal_param
{
#ifdef PARSE_TRACE
  Output << "Reducing formal_params : formal_param\n";
  Output.flush();
#endif
  $$ = AddParameter(NULL, $1);
}
	;

formal_param 
	:	type IDENT
{
#ifdef PARSE_TRACE
  Output << "Reducing formal_param : type IDENT\n";
  Output.flush();
#endif
  $$ = BuildFormal($1, $2);
}
	|	type IDENT GETS expr
{
#ifdef PARSE_TRACE
  Output << "Reducing formal_param : type IDENT GETS expr\n";
  Output.flush();
#endif
  $$ = BuildFormal($1, $2, $4);
}
	;

formal_indexes
	:	formal_indexes LBRAK IDENT RBRAK
{
#ifdef PARSE_TRACE
  Output << "Reducing formal_indexes : formal_indexes LBRAK IDENT RBRAK\n";
  Output.flush();
#endif
  $$ = AddFormalIndex($1, $3);
}
	|	LBRAK IDENT RBRAK
{
#ifdef PARSE_TRACE
  Output << "Reducing formal_indexes : LBRAK IDENT RBRAK\n";
  Output.flush();
#endif
  $$ = AddFormalIndex(NULL, $2);
}
	;


indexes
	:	indexes LBRAK index RBRAK
{
#ifdef PARSE_TRACE
  Output << "Reducing indexes : indexes LBRAK index RBRAK\n";
  Output.flush();
#endif
  $$ = AddParameter($1, $3);
}
	|	LBRAK index RBRAK
{
#ifdef PARSE_TRACE
  Output << "Reducing indexes : LBRAK index RBRAK\n";
  Output.flush();
#endif
  $$ = AddParameter(NULL, $2);
}
	;

index
        :       expr
{
#ifdef PARSE_TRACE
  Output << "Reducing index : expr\n";
  Output.flush();
#endif
  $$ = $1;
}
        ;


pos_params 
	:	pos_params COMMA pos_param
{
#ifdef PARSE_TRACE
  Output << "Reducing pos_params : pos_params COMMA pos_param\n";
  Output.flush();
#endif
  $$ = AddParameter($1, $3);
}
	|	pos_param
{
#ifdef PARSE_TRACE
  Output << "Reducing pos_params : pos_param\n";
  Output.flush();
#endif
  $$ = AddParameter(NULL, $1);
}
	;

pos_param 
	:	topexpr  
{
#ifdef PARSE_TRACE
  Output << "Reducing pos_param : expr\n";
  Output.flush();
#endif
  $$ = $1;
}
	|	DEFAULT
{
#ifdef PARSE_TRACE
  Output << "Reducing pos_param : DEFAULT\n";
  Output.flush();
#endif
  $$ = DEFLT;
}
	;

named_params 
	:	named_params COMMA named_param
{
#ifdef PARSE_TRACE
  Output << "Reducing named_params : named_params COMMA named_param\n";
  Output.flush();
#endif
  $$ = AddParameter($1, $3);
}
	|	named_param
{
#ifdef PARSE_TRACE
  Output << "Reducing named_params : named_param\n";
  Output.flush();
#endif
  $$ = AddParameter(NULL, $1);
}
	;

named_param 
	:	IDENT GETS topexpr
{
#ifdef PARSE_TRACE
  Output << "Reducing named_param : IDENT GETS expr\n";
  Output.flush();
#endif
  $$ = BuildNamedParam($1, $3);
}
	|	IDENT GETS DEFAULT
{
#ifdef PARSE_TRACE
  Output << "Reducing named_param : IDENT GETS DEFAULT\n";
  Output.flush();
#endif
  $$ = BuildNamedDefault($1);
}
	;



%%
/*-----------------------------------------------------------------*/



