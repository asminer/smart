
/*
 *
 * $Id$
 *
 *
 * Yacc file (actually, bison) for smart.
 *
 *
 *
 */

%{

#include "compile.h" // compile-time functionality 

%}

%union {
  char* name;
  const type* Type_ID;
  int count;
  expr* Expr;
  parser_list* List;
  option* Option;
  symbol* Symbol;
  shared_object* other;
};

%token <name> IDENT BOOLCONST INTCONST REALCONST STRCONST MODIF

%token <Type_ID> TYPE FORMALISM

%token LPAR RPAR LBRAK RBRAK LBRACE RBRACE 
COMMA SEMI COLON POUND ENDPND DOT DOTDOT GETS 
PLUS MINUS TIMES DIVIDE MOD
OR AND SET_DIFF IMPLIES NOT 
EQUALS NEQUAL GT GE LT LE 
FOR END CONVERGE IN GUESS
NUL DEFAULT PROC

%type <Type_ID> type model
%type <count> for_header iterators

%type <Expr> statement defn_stmt opt_stmt model_stmt opt_begin
expr term value const_expr 
function_call model_function_call set_expr set_elem pos_param index
doneproduct doneconj arith logic

%type <List> aggexpr seqexpr statements opt_stmts model_stmts model_var_list
idlist formal_params formal_indexes passed_params pos_params indexes set_elems
summation product conjunct disjunct

%type <Symbol> iterator func_header array_header model_header formal_param
%type <Option> opt_header
%type <other> model_call

%left SEMI
%left COMMA
%left COLON
%nonassoc DOTDOT
%left OR
%left AND
%left IMPLIES MOD SET_DIFF
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
  $$ = AppendStatement($1, $2);
}
      |    statement
{
  Reducing("statements : statement");
  $$ = AppendStatement(0, $1);
}
      ;

statement 
      :   for_header LBRACE statements RBRACE
{
  Reducing("statement : for_header LBRACE statements RBRACE");
  $$ = BuildForLoop($1, $3);
}
      |    converge LBRACE statements RBRACE
{ 
  Reducing("statement : converge LBRACE statements RBRACE");
  $$ = FinishConverge($3);
}
      |    decl_stmt
{ 
  Reducing("statement : decl_stmt");
  $$ = 0;
}
      |    defn_stmt
{ 
  Reducing("statement : defn_stmt");
  $$ = $1;
}
      |   model_decl
{ 
  Reducing("statement : model_decl");
  $$ = 0;
} 
      |   opt_stmt
{
  Reducing("statement : opt_stmt");
  $$ = $1;
}
      |   arith SEMI
{
  Reducing("statement : arith SEMI");
  $$ = BuildExprStatement($1);
}
      |   SEMI
{  
  Reducing("statement : SEMI");
  $$ = 0;
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

for_header
      :   FOR LPAR iterators RPAR
{
  Reducing("for_header : FOR LPAR iterators RPAR");
 $$ = $3;  
}
      ;

converge
      :   CONVERGE
{
  Reducing("converge : CONVERGE");
  StartConverge();
}
      ;

opt_stmt
      :   opt_header const_expr ENDPND 
{
  Reducing("opt_stmt : opt_header const_expr ENDPND");
  $$ = BuildOptionStatement($1, $2);
}
      |   opt_header IDENT ENDPND
{
  Reducing("opt_stmt : opt_header IDENT ENDPND");
  $$ = BuildOptionStatement($1, $2);
}       
      |   opt_header PLUS idlist ENDPND
{
  Reducing("opt_stmt : opt_header PLUS idlist ENDPND");
  $$ = BuildOptionStatement($1, true, $3);
}       
      |   opt_header MINUS idlist ENDPND
{
  Reducing("opt_stmt : opt_header MINUS idlist ENDPND");
  $$ = BuildOptionStatement($1, false, $3);
}
      |   opt_begin opt_stmts POUND RBRACE ENDPND
{
  Reducing("opt_stmt : opt_begin opt_stmts POUND RBRACE ENDPND");
  $$ = FinishOptionBlock($1, $2);
}
      ;


opt_header
      :   POUND IDENT
{
  Reducing("opt_header : POUND IDENT");
  $$ = BuildOptionHeader($2);
}
      ;

opt_begin
      :   opt_header IDENT LBRACE ENDPND
{
  Reducing("opt_begin : opt_header IDENT LBRACE ENDPND");
  $$ = StartOptionBlock($1, $2); 
}
      ;

opt_stmts 
      :   opt_stmts opt_stmt
{
  Reducing("opt_stmts : opt_stmts opt_stmt");
  $$ = AppendStatement($1, $2);
}
      |    opt_stmt
{
  Reducing("opt_stmts : opt_stmt");
  $$ = AppendStatement(0, $1);
}
      ;


iterators
      :    iterators COMMA iterator 
{
  Reducing("iterators : iterators COMMA iterator");
  $$ = $1 + AddIterator($3);
}
      |    iterator
{ 
  Reducing("iterators : iterator");
  $$ = AddIterator($1);
}
      ;

iterator
      :    type IDENT IN set_expr
{
  Reducing("iterator : type IDENT IN set_expr");
  $$ = BuildIterator($1, $2, $4);
}
      ;

type
      :   PROC MODIF TYPE
{
  Reducing("type : PROC MODIF TYPE");
  $$ = MakeType(true, $2, $3);
}
      |   PROC TYPE
{
  Reducing("type : PROC TYPE");
  $$ = MakeType(true, 0, $2);
}
      |   MODIF TYPE
{
  Reducing("type : MODIF TYPE");
  $$ = MakeType(false, $1, $2);
}
      |   TYPE
{
  Reducing("type : TYPE");
  $$ = MakeType(false, 0, $1);
}
      ;

model
      :    FORMALISM
{
  Reducing("model : FORMALISM");
  $$ = MakeType(false, 0, $1);
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
      :    arith
{
  Reducing("set_elem : arith");
  $$ = BuildElementSet($1);
}
      |    arith DOTDOT arith
{
  Reducing("set_elem : arith DOTDOT arith");
  $$ = BuildInterval($1, $3); 
}
      |    arith DOTDOT arith DOTDOT arith
{
  Reducing("set_elem : arith DOTDOT arith DOTDOT arith");
  $$ = BuildInterval($1, $3, $5);
}
      ;

/*==================================================================\
|                                                                   |
|                      Top-level  declarations                      |        
|                                                                   |
\==================================================================*/

decl_stmt 
      :   func_header SEMI
{
  Reducing("decl_stmt : func_header SEMI");
  DoneWithFunctionHeader();
}
      ;

defn_stmt
      :    func_header GETS arith SEMI
{
  Reducing("defn_stmt : func_header GETS arith SEMI");
  $$ = BuildFuncStmt($1, $3);
}
      |    type IDENT GETS arith SEMI 
{
  Reducing("defn_stmt : type IDENT GETS arith SEMI");
  $$ = BuildVarStmt($1, $2, $4);
}
      |    type IDENT GUESS arith SEMI
{
  Reducing("defn_stmt : type IDENT GUESS arith SEMI");
  $$ = BuildGuessStmt($1, $2, $4);
}
      |    array_header GETS arith SEMI
{
  Reducing("defn_stmt : array_header GETS arith SEMI");
  $$ = BuildArrayStmt($1, $3);
}
      |    array_header GUESS arith SEMI
{
  Reducing("defn_stmt : array_header GUESS arith SEMI");
  $$ = BuildArrayGuess($1, $3);
}
      ;

/*==================================================================\
|                                                                   |
|                      Function/Array  Headers                      |        
|                                                                   |
\==================================================================*/

func_header
      :    type IDENT LPAR formal_params RPAR
{
  Reducing("func_header : type IDENT LPAR formal_params RPAR");
  $$ = BuildFunction($1, $2, $4);
}
      ;

array_header
      :   type IDENT formal_indexes 
{
  Reducing("array_header : type IDENT formal_indexes");
  $$ = BuildArray($1, $2, $3);
}
      ;

/*==================================================================\
|                                                                   |
|                        Model  declarations                        |        
|                                                                   |
\==================================================================*/

model_decl
      :    model_header GETS LBRACE model_stmts RBRACE SEMI
{
  Reducing("model_decl : model_header GETS LBRACE model_stmts RBRACE SEMI");
  BuildModelStmt($1, $4);
}
      ; 


model_header
      :    model IDENT LPAR formal_params RPAR
{
  Reducing("model_header : model IDENT LPAR formal_params RPAR");
  $$ = BuildModel($1, $2, $4);
}
      |   model IDENT
{
  Reducing("model_header : model IDENT");
  $$ = BuildModel($1, $2, 0);
}
      ;

model_stmts
      :   model_stmts model_stmt
{
  Reducing("model_stmts : model_stmts model_stmt");
  $$ = AppendStatement($1, $2);
}
      |   model_stmt
{
  Reducing("model_stmts : model_stmt");
  $$ = AppendStatement(0, $1);
}
      ;

model_stmt
      :   function_call SEMI
{
  Reducing("model_stmt : function_call SEMI");
  $$ = BuildExprStatement($1);
}
      |    type model_var_list SEMI
{
  Reducing("model_stmt : type model_var_list SEMI");
  $$ = BuildModelVarStmt($1, $2);
}
      |    defn_stmt 
{
  Reducing("model_stmt : defn_stmt");
  $$ = $1;
}
      |   for_header LBRACE model_stmts RBRACE
{
  Reducing("model_stmt : for_header LBRACE model_stmts RBRACE");
  $$ = BuildForLoop($1, $3);
}
      ;

model_var_list
      :   model_var_list COMMA IDENT    
{
  Reducing("model_var_list : model_var_list COMMA IDENT");
  $$ = AddModelVar($1, $3);
}
      |   model_var_list COMMA IDENT formal_indexes
{
  Reducing("model_var_list : model_var_list COMMA IDENT formal_indexes");
  $$ = AddModelArray($1, $3, $4);
}
      |   IDENT    
{
  Reducing("model_var_list : IDENT");
  $$ = AddModelVar(0, $1);
}
      |    IDENT formal_indexes
{
  Reducing("model_var_list : IDENT formal_indexes");
  $$ = AddModelArray(0, $1, $2);
}
      ;


/*==================================================================\
|                                                                   |
|                            Expressions                            |        
|                                                                   |
\==================================================================*/

expr
      :    arith
{
  Reducing("expr : arith");
  $$ = $1;
}
      |    aggexpr
{
  Reducing("expr : aggexpr");
  $$ = BuildAssociative(COLON, $1);
}
      ;

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
      |    arith SET_DIFF arith
{
  Reducing("arith : arith SET_DIFF arith");
  $$ = BuildBinary($1, SET_DIFF, $3);
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
      :    NUL
{  
  Reducing("term : NUL");
  $$ = 0;
}
      |   value      
{
  Reducing("term : value");
  $$ = $1;
}
      |    set_expr
{
  Reducing("term : set_expr");
  $$ = $1;
}
      |   model_function_call
{
  Reducing("term : model_function_call");
  $$ = $1;
}
      |    function_call
{
  Reducing("term : function_call");
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
      |    type LPAR arith RPAR
{
  Reducing("term : type LPAR arith RPAR");
  $$ = BuildTypecast($1, $3);
}
      |   LBRACE seqexpr SEMI RBRACE
{
  Reducing("term : LBRACE seqexpr SEMI RBRACE");
  $$ = BuildAssociative(SEMI, $2);
}
      ;


aggexpr 
      :    aggexpr COLON arith
{
  Reducing("aggexpr : aggexpr COLON arith");
  $$ = AppendExpression(0, $1, $3);
}
      |   arith COLON arith
{
  Reducing("aggexpr : arith COLON arith");
  $$ = AppendExpression(0, AppendExpression(2, 0, $1), $3);
}
      ;


seqexpr 
      :   seqexpr SEMI term
{
  Reducing("seqexpr : seqexpr SEMI term");
  $$ = AppendExpression(1, $1, $3);
}
      |   term
{
  Reducing("seqexpr : term");
  $$ = AppendExpression(1, 0, $1);
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
      |   REALCONST
{
  Reducing("value : REALCONST");
  $$ = MakeRealConst($1);
}
      |   STRCONST
{
  Reducing("value : STRCONST");
  $$ = MakeStringConst($1);
}
      ;

const_expr
      :    value
{
  Reducing("const_expr : value");
  $$ = $1;
}
      |    MINUS const_expr %prec UMINUS
{
  Reducing("const_expr : MINUS const_expr");
  $$ = BuildUnary(MINUS, $2);
}
      |    NOT const_expr
{
  Reducing("const_expr : NOT const_expr");
  $$ = BuildUnary(NOT, $2);
}
      ;

/*==================================================================\
|                                                                   |
|                            Model calls                            |        
|                                                                   |
\==================================================================*/

model_function_call
      :   model_call DOT IDENT
{  
  Reducing("model_function_call : model_call DOT IDENT");
  $$ = MakeMCall($1, $3);
}
      |    IDENT indexes DOT IDENT
{
  Reducing("model_function_call : IDENT indexes DOT IDENT");
  $$ = MakeAMCall($1, $2, $4);
}
      |    model_call DOT IDENT indexes
{
  Reducing("model_function_call : model_call DOT IDENT indexes");
  $$ = MakeMACall($1, $3, $4);
}
      |    IDENT indexes DOT IDENT indexes
{
  Reducing("model_function_call : IDENT indexes DOT IDENT indexes");
  $$ = MakeAMACall($1, $2, $4, $5);
}
      ;

model_call
      :   IDENT
{ 
  Reducing("model_call : IDENT");
  $$ = MakeModelCall($1, 0);
}
      |   IDENT passed_params
{ 
  Reducing("model_call : IDENT passed_params");
  $$ = MakeModelCall($1, $2);
}
      ;

/*==================================================================\
|                                                                   |
|                          Function  calls                          |        
|                                                                   |
\==================================================================*/

function_call 
      :   IDENT 
{
  Reducing("function_call : IDENT");
  $$ = FindIdent($1);
}
      |    IDENT indexes   
{
  Reducing("function_call : IDENT indexes");
  $$ = BuildArrayCall($1, $2);
}
      |    IDENT passed_params
{
  Reducing("function_call : IDENT passed_params");
  $$ = BuildFunctionCall($1, $2);
}
      ;

/*==================================================================\
|                                                                   |
|                          Parameter lists                          |        
|                                                                   |
\==================================================================*/

formal_params 
      :   formal_params COMMA formal_param
{
  Reducing("formal_params : formal_params COMMA formal_param");
  $$ = AppendFormal($1, $3);
}
      |    formal_param
{
  Reducing("formal_params : formal_param");
  $$ = AppendFormal(0, $1);
}
      ;

formal_param 
      :    type IDENT
{
  Reducing("formal_param : type IDENT");
  $$ = BuildFormal($1, $2);
}
      |    type IDENT GETS expr
{
  Reducing("formal_param : type IDENT GETS expr");
  $$ = BuildFormal($1, $2, $4);
}
      ;

formal_indexes
      :    formal_indexes LBRAK IDENT RBRAK
{
  Reducing("formal_indexes : formal_indexes LBRAK IDENT RBRAK");
  $$ = AppendName($1, $3);
}
      |    LBRAK IDENT RBRAK
{
  Reducing("formal_indexes : LBRAK IDENT RBRAK");
  $$ = AppendName(0, $2);
}
      ;


indexes
      :    indexes LBRAK index RBRAK
{
  Reducing("indexes : indexes LBRAK index RBRAK");
  $$ = AppendExpression(0, $1, $3);
}
      |    LBRAK index RBRAK
{
  Reducing("indexes : LBRAK index RBRAK");
  $$ = AppendExpression(0, 0, $2);
}
      ;

index
      :   arith
{
  Reducing("index : arith");
  $$ = $1;
}
      ;

passed_params
      :   LPAR pos_params RPAR
{
  Reducing("passed_params : LPAR pos_params RPAR");
  $$ = $2;
}
      |   LPAR RPAR
{
  Reducing("passed_params : LPAR RPAR");
  $$ = 0;
}
      ;

pos_params 
      :   pos_params COMMA pos_param
{
  Reducing("pos_params : pos_params COMMA pos_param");
  $$ = AppendExpression(0, $1, $3);
}
      |   pos_param
{
  Reducing("pos_params : pos_param");
  $$ = AppendExpression(0, 0, $1);
}
      ;

pos_param 
      :   expr  
{
  Reducing("pos_param : expr");
  $$ = $1;
}
      |   DEFAULT
{
  Reducing("pos_param : DEFAULT");
  $$ = Default();
}
      ;



%%
/*-----------------------------------------------------------------*/



