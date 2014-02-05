
/*
 *
 * $Id$
 *
 *
 * Yacc file (actually, bison) for Integer Constraint Programming.
 *
 *
 */

%{

#include "compile.h" // compile-time functionality 

%}

%union {
  char* name;
  expr* Expr;
  parser_list* List;
  option* Option;
};

%token <name> IDENT BOOLCONST INTCONST TYPE

%token LPAR RPAR LBRACE RBRACE 
COMMA SEMI DOTDOT GETS 
PLUS MINUS TIMES DIVIDE MOD
OR AND IMPLIES NOT 
EQUALS NEQUAL GT GE LT LE 
MAXIMIZE MINIMIZE SATISFIABLE IN
POUND ENDPND

%type <Expr> statement declaration measure term value 
set_expr set_elem doneproduct doneconj arith logic function_call

%type <List> summation product conjunct disjunct set_elems namelist idlist pos_params

%type <Option> opt_header

%left SEMI
%left COMMA
%left COLON
%nonassoc DOTDOT
%left OR
%left AND
%left IMPLIES MOD
%left EQUALS NEQUAL
%left GT GE LT LE
%left PLUS MINUS
%left TIMES DIVIDE
%right NOT UMINUS
%nonassoc LPAR RPAR LBRAK RBRAK

/*-----------------------------------------------------------------*/
%%

/*==================================================================\
|                                                                   |
|                       Top-level  statements                       |        
|                                                                   |
\==================================================================*/

statements 
      :   statements statement
{
  Reducing("statements : statements statement");
  AppendStatement($2);
}
      |   statement
{
  Reducing("statements : statement");
  AppendStatement($1);
}
      ;

statement 
      :   opt_header value ENDPND 
{
  Reducing("statement : opt_header value ENDPND");
  $$ = BuildOptionStatement($1, $2);
}
      |   opt_header IDENT ENDPND
{
  Reducing("statement : opt_header IDENT ENDPND");
  $$ = BuildOptionStatement($1, $2);
}       
      |   opt_header PLUS idlist ENDPND
{
  Reducing("statement : opt_header PLUS idlist ENDPND");
  $$ = BuildOptionStatement($1, true, $3);
}       
      |   opt_header MINUS idlist ENDPND
{
  Reducing("statement : opt_header MINUS idlist ENDPND");
  $$ = BuildOptionStatement($1, false, $3);
}       
      |   declaration 
{ 
  Reducing("statement : declaration");
  $$ = $1;
}
      |   function_call SEMI
{
  Reducing("state,emt : function_call SEMI");
  $$ = BuildExprStatement($1);
}
      |    arith SEMI
{ 
  Reducing("statement : arith SEMI");
  $$ = MakeConstraint($1);
}
      |   measure SEMI
{ 
  Reducing("statement : model_decl SEMI");
  $$ = $1;
} 
      |   SEMI
{  
  Reducing("statement : SEMI");
  $$ = 0;
}
      ;


opt_header
      :   POUND IDENT
{
  Reducing("opt_header : POUND IDENT");
  $$ = BuildOptionHeader($2);
}
      ;


idlist
      :   idlist COMMA IDENT
{
  Reducing("idlist : idlist COMMA IDENT");
  $$ = AppendName($1, $3);
}
      |   IDENT
{
  Reducing("idlist : IDENT");
  $$ = AppendName(0, $1);
}
      ;


declaration
      :    TYPE namelist IN set_expr SEMI
{
  Reducing("declaration : TYPE namelist IN set_expr SEMI");
  $$ = BuildIntegers($1, $2, $4);
}
      |    TYPE namelist SEMI
{
  Reducing("declaration : TYPE namelist SEMI");
  $$ = BuildBools($1, $2);
}
      ;

namelist
      :    namelist COMMA IDENT
{
  Reducing("namelist : namelist COMMA IDENT");
  $$ = AddModelVar($1, $3);
}
      |    IDENT
{
  Reducing("namelist : IDENT");
  $$ = AddModelVar(0, $1);
}
      ;

set_expr
      :    LBRACE set_elems RBRACE
{
  Reducing("set_expr : LBRACE set_elems RBRACE");
  $$ = BuildAssociative(COMMA, $2);
}
      ;

set_elems
      :    set_elems COMMA set_elem
{
  Reducing("set_elems : set_elems COMMA set_elem");
  $$ = AppendExpression(2, $1, $3);
}
      |    set_elem
{
  Reducing("set_elems : set_elem");
  $$ = AppendExpression(2, 0, $1);
}
      ;

set_elem
      :    value
{
  Reducing("set_elem : value");
  $$ = BuildElementSet($1);
}
      |    value DOTDOT value
{
  Reducing("set_elem : value DOTDOT value");
  $$ = BuildInterval($1, $3); 
}
      |    value DOTDOT value DOTDOT value
{
  Reducing("set_elem : value DOTDOT value DOTDOT value");
  $$ = BuildInterval($1, $3, $5);
}
      ;

measure
      :    MAXIMIZE IDENT GETS arith
{
  Reducing("measure : MAXIMIZE IDENT GETS arith");
  $$ = BuildMaximize($2, $4);
}
      |    MINIMIZE IDENT GETS arith
{
  Reducing("measure : MINIMIZE IDENT GETS arith");
  $$ = BuildMinimize($2, $4);
}
      |    SATISFIABLE IDENT GETS arith
{
  Reducing("measure : SATISFIABLE IDENT GETS arith");
  $$ = BuildSatisfiable($2, $4);
}
      ;


/*==================================================================\
|                                                                   |
|                            Expressions                            |        
|                                                                   |
\==================================================================*/

arith
      :    disjunct
{
  Reducing("arith : disjunct");
  $$ = BuildSummation($1);
}
      |    arith IMPLIES arith
{
  Reducing("arith : arith IMPLIES arith");
  $$ = BuildBinary($1, IMPLIES, $3);
}
      ;

disjunct
      :    disjunct OR doneconj
{
  Reducing("disjunct : disjunct OR doneconj");
  $$ = AppendTerm($1, OR, $3);
}
      |    doneconj
{
  Reducing("disjunct : doneconj");
  $$ = AppendTerm(0, 0, $1);
}
      ;

doneconj
      :    conjunct
{
  Reducing("doneconj : conjunct");
  $$ = BuildProduct($1);
}
      ;

conjunct
      :    conjunct AND logic
{
  Reducing("conjunct : conjunct AND logic");
  $$ = AppendTerm($1, AND, $3);
}
      |    logic
{
  Reducing("conjunct : logic");
  $$ = AppendTerm(0, 0, $1);
}
      ;

logic
      :    summation
{
  Reducing("logic : summation");
  $$ = BuildSummation($1);
}
      |    logic EQUALS logic
{
  Reducing("logic : logic EQUALS logic");
  $$ = BuildBinary($1, EQUALS, $3);
}
      |    logic NEQUAL logic
{
  Reducing("logic : logic NEQUAL logic");
  $$ = BuildBinary($1, NEQUAL, $3);
}
      |    logic GT logic
{
  Reducing("logic : logic GT logic");
  $$ = BuildBinary($1, GT, $3);
}
      |    logic GE logic
{
  Reducing("logic : logic GE logic");
  $$ = BuildBinary($1, GE, $3);
}
      |    logic LT logic
{
  Reducing("logic : logic LT logic");
  $$ = BuildBinary($1, LT, $3);
}
      |    logic LE logic
{
  Reducing("logic : logic LE logic");
  $$ = BuildBinary($1, LE, $3);
}
      ;

summation
      :    summation PLUS doneproduct
{
  Reducing("summation : summation PLUS doneproduct");
  $$ = AppendTerm($1, PLUS, $3);
}
      |    summation MINUS doneproduct
{
  Reducing("summation : summation MINUS doneproduct");
  $$ = AppendTerm($1, MINUS, $3);
}
      |    doneproduct
{
  Reducing("summation : doneproduct");
  $$ = AppendTerm(0, 0, $1);
}
      ;

doneproduct
      :    product
{
  Reducing("doneproduct : product");
  $$ = BuildProduct($1);
}
      |    doneproduct MOD doneproduct
{
  Reducing("doneproduct : doneproduct MOD doneproduct");
  $$ = BuildBinary($1, MOD, $3);
}
      ;

product
      :    product TIMES term
{
  Reducing("product : product TIMES term");
  $$ = AppendTerm($1, TIMES, $3);
}
      |    product DIVIDE term
{
  Reducing("product : product DIVIDE term");
  $$ = AppendTerm($1, DIVIDE, $3);
}
      |    term
{
  Reducing("product : term");
  $$ = AppendTerm(0, 0, $1);
}
      ;

term 
      :   value      
{
  Reducing("term : value");
  $$ = $1;
}
      |   IDENT 
{
  Reducing("term : IDENT");
  $$ = FindIdent($1);
}
      |    set_expr
{
  Reducing("term : set_expr");
  $$ = $1;
}
      |    LPAR arith RPAR
{
  Reducing("term : LPAR arith RPAR");
  $$ = $2;
}
      |    NOT term
{
  Reducing("term : NOT term");
  $$ = BuildUnary(NOT, $2);
}
      |    MINUS term %prec UMINUS
{
  Reducing("term : MINUS term");
  $$ = BuildUnary(MINUS, $2);
}
      ;


value      
      :   BOOLCONST
{
  Reducing("value : BOOLCONST");
  $$ = MakeBoolConst($1);
} 
      |    INTCONST
{
  Reducing("value : INTCONST");
  $$ = MakeIntConst($1);
}
      ;


/*==================================================================\
|                                                                   |
|                          Function  calls                          |        
|                                                                   |
\==================================================================*/

function_call 
      :    IDENT LPAR pos_params RPAR
{
  Reducing("function_call : IDENT LPAR pos_params RPAR");
  $$ = BuildFunctionCall($1, $3);
}
      ;

pos_params 
      :    pos_params COMMA arith
{
  Reducing("pos_params : pos_params COMMA arith");
  $$ = AppendExpression(0, $1, $3);
}
      |    arith
{
  Reducing("pos_params : arith");
  $$ = AppendExpression(0, 0, $1);
}
      ;





%%
/*-----------------------------------------------------------------*/



