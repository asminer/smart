
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

#include "../Language/api.h"  // previous layer

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
  expr_set *setexpr;
  List <local_iterator> *itrs;
  */
  List <statement> *stmts;
  List <formal_param> *fpl;
  List <pos_param> *ppl;
  List <named_param> *npl;
  List <function> *varlist;
  /*
  option *Option;
  List <char *> *tuple_ids;
  */
}

%type <Expr> expr term const_expr function_call model_call model_function_call
%type <Func> header array_header
%type <fpl> formal_params formal_param
%type <ppl> pos_params pos_param indexes index
%type <npl> named_params named_param 
/*
%type <setexpr> set_expr set_elems set_elem 
*/
%type <stmts> statement statements decl_stmt defn_stmt model_stmt model_stmts
/*
%type <itrs> iterator iterators for_header 
*/
%type <varlist> model_var_decl
%type <Type_ID> type
/*
%type <Option> opt_header
%type <tuple_ids> tupleidlist
*/

%token <name> IDENT BOOLCONST INTCONST REALCONST STRCONST 
LPAR RPAR LBRAK RBRAK LBRACE RBRACE 
COMMA SEMI COLON POUND DOT DOTDOT GETS PLUS MINUS TIMES DIVIDE OR AND NOT 
EQUALS NEQUAL GT GE LT LE ENDPND FOR END CONVERGE IN GUESS
NUL DEFAULT TYPE MODIF MODEL 

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
  if ($1) { $1->Append($2); $$ = $1; } 
  else { $$ = $2; }
}
	|	statement
{
#ifdef PARSE_TRACE
  cout << "Reducing statements : statement\n";
#endif
  $$ = $1;
}
	;

statement 
        :       for_header LBRACE statements RBRACE
{
#ifdef PARSE_TRACE
  cout << "Reducing statement : for_header LBRACE statements RBRACE\n";
#endif
  /*
  $$ = BuildForStmt($1, $3);
  */
}
	|	converge LBRACE statements RBRACE
{ 
#ifdef PARSE_TRACE
  cout << "Reducing statement : converge LBRACE statements RBRACE\n";
#endif
  /*
  $$ = BuildConvergeStmt($3);
  */
}
	|	decl_stmt
{ 
#ifdef PARSE_TRACE
  cout << "Reducing statement : decl_stmt\n";
#endif
  $$ = $1; 
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
  cout << "Reducing statement : opt_header const_expr ENDPND\n";
#endif
  /*
  $$ = BuildOptionStmt($1, $2);
  */
}
        |       opt_header IDENT ENDPND
{
#ifdef PARSE_TRACE
  cout << "Reducing statement : opt_header IDENT ENDPND\n";
#endif
  /*
  $$ = BuildOptionStmt($1, $2);
  */
}       
        |       opt_header LBRACE tupleidlist RBRACE ENDPND
{
#ifdef PARSE_TRACE
  cout << "Reducing statement : opt_header LBRACE tipleidlist RBRACE ENDPND\n";
#endif
  /*
  $$ = BuildOptionStmt($1, $3);
  */
}       
        |       expr SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing statement : expr SEMI\n";
#endif
  /*
  $$ = BuildExprStmt($1);
  */
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
  /*
  $1->Append(new List<char *>(new char*($3)));
  $$ = $1;
  */
}
        |       IDENT
{
#ifdef PARSE_TRACE
  cout << "Reducing tupleidlist : IDENT\n";
#endif
  /*
  $$ = new List<char *>(new char*($1));
  */
}
	;

for_header
        : 	FOR LPAR iterators RPAR
{
#ifdef PARSE_TRACE
  cout << "Reducing for_header : FOR LPAR iterators RPAR\n";
#endif
  block++;
  $$ = $3;
}
	;

converge
        :       CONVERGE
{
#ifdef PARSE_TRACE
  cout << "Reducing converge : CONVERGE\n";
#endif
  converges++;
}
	;

opt_header
        :       POUND IDENT
{
#ifdef PARSE_TRACE
  cout << "Reducing opt_header : POUND IDENT\n";
#endif
  /*
  option *o = GetOptionPtr($2);
  if (o==NULL) {
    Error.Start(filename, lexer.lineno);
    Error << "Unknown option: " << $2;
    Error.Stop();
    $$ = NULL;
  } else $$ = o;
  */
}
	;

iterators
	:	iterators COMMA iterator 
{
#ifdef PARSE_TRACE
  cout << "Reducing iterators : iterators COMMA iterator\n";
#endif
   /*
   $1->Append($3);
   $$ = $1;
   */
}
	|	iterator
{ 
#ifdef PARSE_TRACE
  cout << "Reducing iterators : iterator\n";
#endif
   $$ = $1;
}
	;

iterator
	:	type IDENT IN set_expr
{
#ifdef PARSE_TRACE
  cout << "Reducing iterator : type IDENT IN set_expr\n";
#endif
  /*
  $$ = BuildIterator($1, $2, $4);
  */
}
	;

type
        :       MODIF TYPE
{
#ifdef PARSE_TRACE
  cout << "Reducing type : MODIF TYPE\n";
#endif
  /*
   int m = FindModif($1);
   int t = FindType($2);
   if (CompatibleModif(m, t)) {
     $$ = new type_list((m+1)*MAXTYPES + t);
   } else {
     char *err = Cat("Modifier ", $1);
     err = Cat(err, " has no effect on type ", true);
     err = Cat(err, $2, true);
     yywarn(err);
     delete[] err;
     $$ = new type_list(t);
   }
   */
}
        |       TYPE
{
#ifdef PARSE_TRACE
  cout << "Reducing type : TYPE\n";
#endif
  /*
   $$ = new type_list(FindType($1));
   */
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
  /*
   $$ = JoinSets($1, $3);
   */
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
  /*
  $$ = BuildSet($1);
  */
}
	|	expr DOTDOT expr
{
#ifdef PARSE_TRACE
  cout << "Reducing set_elem : expr DOTDOT expr\n";
#endif
  /*
  $$ = BuildInterval($1, $3);
  */
}
	|	expr DOTDOT expr DOTDOT expr
{
#ifdef PARSE_TRACE
  cout << "Reducing set_elem : expr DOTDOT expr DOTDOT expr\n";
#endif
  /*
  $$ = BuildRange($1, $3, $5);
  */
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
   /*
   LocalTable.KillBlock(block);
   */
   $$ = NULL;
}
	|	array_header SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing decl_stmt : array_header SEMI\n";
#endif
 $$ = NULL;
}
	;

defn_stmt
	:	header GETS expr SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing defn_stmt : header GETS expr SEMI\n";
#endif
  /*
  $$ = FinishFunction($1, $3);
  LocalTable.KillBlock(block);
  */
}
	|	type IDENT GETS expr SEMI 
{
#ifdef PARSE_TRACE
  cout << "Reducing defn_stmt : type IDENT GETS expr SEMI\n";
#endif
  /*
  $$ = BuildFunction($1, $2, $4);
  */
}
	|	type IDENT GUESS expr SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing defn_stmt : type IDENT GUESS expr SEMI\n";
#endif
  /*
  $$ = GuessFunction($1, $2, $4);
  */
}
	|	array_header GETS expr SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing defn_stmt : array_header GETS expr SEMI\n";
#endif
  /*
  $$ = FinishFunction($1, $3);
  */
}
	|	array_header GUESS expr SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing defn_stmt : array_header GUESS expr SEMI\n";
#endif
  /*
  $$ = GuessArray($1, $3);
  */
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
  /*
  $$ = BuildHeader($1, $2, $4);
  */
}
        ;

array_header
        :       type IDENT indexes 
{
#ifdef PARSE_TRACE
  cout << "Reducing array_header : type IDENT indexes\n";
#endif
  /*
  $$ = BuildHeader($1, $2, $3);
  */
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
  /*
  block--;
  if (block>1) {
    yyerror(10, "Models must be declared at the top level.");
  } else if (converges>0) {
    yyerror(10, "Models cannot be declared in a converge.");
  } else {
    if ($4!=NULL) Model->SetStmts($4->Compress(), $4->Length());
  }
  // Remove the model vars from the local table
  LocalTable.KillBlock(block+1); 
  // Remove the model parameters from the local table
  LocalTable.KillBlock(block);   
  if ($4!=NULL) delete $4;
  InModel = false;
  Model = NULL; 
  */
}
	; 


model_header
	:	MODEL IDENT LPAR formal_params RPAR
{
#ifdef PARSE_TRACE
  cout << "Reducing model_header : MODEL IDENT LPAR formal_params RPAR\n";
#endif
  /*
  if (block<2) {
    int mt = FindModel($1);
    Model = NewModel(mt, $2, $4->Compress(), $4->Length());
    FTable.Insert(Model, block);
  } else {
    cout << "Sorry, models must be declared at the top level\n";
    Model = NULL;
  }
  block++;
  delete $4;
  InModel = true;
  */
}
        |       MODEL IDENT
{
#ifdef PARSE_TRACE
  cout << "Reducing model_header : MODEL IDENT\n";
#endif
  /*
  if (block<2) {
    int mt = FindModel($1);
    Model = NewModel(mt, $2, NULL, 0);
    FTable.Insert(Model, block);
  }
  block++;
  InModel = true;
  */
}
        ;

model_stmts
        :       model_stmts model_stmt
{
#ifdef PARSE_TRACE
  cout << "Reducing model_stmts : model_stmts model_stmt\n";
  statement *s = NULL;
  if ($2) s = $2->data;
  cout << "\nDone statement:\n";
  if (s) s->Show(); else cout << "NULL statement\n";
  cout << "\n";
#endif
  if ($1==NULL) $$ = $2; else {
    $1->Append($2);
    $$ = $1;
  }
}
        |       model_stmt
{
#ifdef PARSE_TRACE
  cout << "Reducing model_stmts : model_stmt\n";
  statement *s = $1->data;
  cout << "\nDone statement:\n";
  s->Show();
  cout << "\n";
#endif
  $$ = $1;
}
        ;

model_stmt
        :       function_call SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing model_stmt : function_call SEMI\n";
#endif
   /*
   statement *es = new expr_statement($1);
   $$ = new List <statement> (es);
   */
}
	|	model_var_decl SEMI
{
#ifdef PARSE_TRACE
  cout << "Reducing model_stmt : model_var_decl SEMI\n";
#endif
  /*
  if ($1) {
    statement *s = new model_var_dec_stmt(Model, $1);
    $$ = new List <statement> (s);
  } else $$ = NULL;
  */
}
	|	defn_stmt 
{
#ifdef PARSE_TRACE
  cout << "Reducing model_stmt : defn_stmt\n";
#endif
  $$ = $1; 
}
        |       for_header LBRACE model_stmts RBRACE
{
#ifdef PARSE_TRACE
  cout << "Reducing model_stmt : for_header LBRACE model_stmts RBRACE\n";
#endif
  $$ = BuildForStmt($1, $3);
}
        ;

model_var_decl
        :       model_var_decl COMMA IDENT    
{
#ifdef PARSE_TRACE
  cout << "Reducing model_var_decl : model_var_decl COMMA IDENT\n";
#endif
    type_list *t = $1->data->Type();
    function *var = new model_variable(t, $3);
    var->SetParent(Model);
    $$ = $1;
    if (AddToModelTable(Model, var)) $$->Append(var);
}
        |       model_var_decl COMMA IDENT indexes
{
#ifdef PARSE_TRACE
  cout << "Reducing model_var_decl : model_var_decl COMMA IDENT indexes\n";
#endif
  type_list *t = $1->data->Type();
  int dim = Descriptor.Length(); 
  int parms = $4->Length();              
  if (dim!=parms) {
      char *err = Cat("Incorrect dimension for array ", $3);
      yyerror(13, err);
      delete err;
      delete t;
      $$ = NULL;
  } else if (!IteratorCheck(&Descriptor, $4)) {
      char *err = Cat("Bad index for array ", $3);
      yyerror(14, err);
      delete err;
      delete t;
      $$ = NULL;
  } else {
      // indices are fine
      local_iterator **itlist = new local_iterator*[dim];
      // Make a shallow copy
      for (int i=0; i<dim; i++) itlist[i] = Descriptor.Item(i);  
      function *var = new model_array(t, $3, itlist, dim);

      // insert the variable in the model symbol table
      $$ = $1;
      if (AddToModelTable(Model, var)) $$->Append(var);
  }
}
        |       type IDENT    
{
#ifdef PARSE_TRACE
  cout << "Reducing model_var_decl : type IDENT\n";
#endif
  if (Model->LegalType($1)) {
    function *var = new model_variable($1, $2);
    var->SetParent(Model);
    if (AddToModelTable(Model, var)) $$ = new List <function>(var);
  } else {
    type_list *t = Model->Type();
    char *err = Cat(GetType(t->Data()), " model does not support ");
    err = Cat(err, GetType($1->Data()), true);
    err = Cat(err, " declarations.", true);
    yyerror(15, err);
    delete err;
    delete t;
    $$ = NULL;
  }
}
	|	type IDENT indexes
{
#ifdef PARSE_TRACE
  cout << "Reducing model_var_decl : type IDENT indexes\n";
#endif
  if (Model->LegalType($1)) {
    // Legal type for this model, make the array
    int dim = Descriptor.Length(); 
    int parms = $3->Length();              
    if (dim!=parms) {
      char *err = Cat("Incorrect dimension for array ", $2);
      yyerror(13, err);
      delete err;
      delete $1;
      $$ = NULL;
    } else if (!IteratorCheck(&Descriptor, $3)) {
      char *err = Cat("Bad index for array ", $2);
      yyerror(14, err);
      delete err;
      delete $1;
      $$ = NULL;
    } else {
      // indices are fine
      local_iterator **itlist = new local_iterator*[dim];
      // Make a shallow copy
      for (int i=0; i<dim; i++) itlist[i] = Descriptor.Item(i);  
      function *var = new model_array($1, $2, itlist, dim);

      // insert the variable in the model symbol table
      if (AddToModelTable(Model, var)) $$ = new List <function>(var);
    }
    // done making array
  } else {
    // Illegal type
    type_list *t = Model->Type();
    char *err = Cat(GetType(t->Data()), " model does not support ");
    err = Cat(err, GetType($1->Data()), true);
    err = Cat(err, " declarations.", true);
    yyerror(15, err);
    delete err;
    delete t;
    $$ = NULL;
  }
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
  $$ = BuildExpr($1, PLUS, $3);
}
	| 	expr MINUS expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr MINUS expr\n";
#endif
  $$ = BuildExpr($1, MINUS, $3);
}
	|	expr TIMES expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr TIMES expr\n";
#endif
  $$ = BuildExpr($1, TIMES, $3);
}
	|	expr DIVIDE expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr DIVIDE expr\n";
#endif
  $$ = BuildExpr($1, DIVIDE, $3);
}
	|	expr OR expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr OR expr\n";
#endif
  $$ = BuildExpr($1, OR, $3);
}
	|	expr AND expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr AND expr\n";
#endif
  $$ = BuildExpr($1, AND, $3);
}
	|	NOT expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : NOT expr\n";
#endif
  if (!LegalOp(NOT, $2)) yyerror(22, "Illegal Operand type on NOT");
  $$ = MakeExpr('!', $2);
}
	|	MINUS expr %prec UMINUS
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : MINUS expr\n";
#endif
  if (!LegalOp(MINUS, $2)) yyerror(23, "Illegal Operand type on Unary Minus");
  $$ = MakeExpr('!', $2);
}
	|	expr EQUALS expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr EQUALS expr\n";
#endif
  $$ = BuildExpr($1, EQUALS, $3);
}
	|	expr NEQUAL expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr NEQUAL expr\n";
#endif
   if (!LegalOp($1, NEQUAL, $3)) yyerror(25, "Illegal Operand Type on !=");
   $$ = MakeExpr('!', MakeExpr($1, '=', $3));
}
	|	expr GT expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr GT expr\n";
#endif
  $$ = BuildExpr($1, GT, $3);
}
	|	expr GE	expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr GE expr\n";
#endif
   if (!LegalOp($1, GE, $3)) yyerror(27, "Illegal Operand Type on >=");
   $$ = MakeExpr('!', MakeExpr($1, '<', $3));
}
	|	expr LT expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr LT expr\n";
#endif
  $$ = BuildExpr($1, LT, $3);
}
	|	expr LE expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr LE expr\n";
#endif
   if (!LegalOp($1, LE, $3)) yyerror(29, "Illegal Operand Type on <=");
   $$ = MakeExpr('!', MakeExpr($1, '>', $3));
}
	|	type LPAR expr RPAR
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : type LPAR expr RPAR\n";
#endif
  type_list *t = $3->Type();
  if (Castable(t, $1)) {
    $$ = ChangeType($3, $1);
  } else {
    char *err = Cat("Cannot convert expression to ", GetType($1->Data()));
    yyerror(30, err);
    delete err;
    $$ = NULL;
    delete $1;
    delete $3;
  }
  delete t;
}
        |       expr COLON expr
{
#ifdef PARSE_TRACE
  cout << "Reducing expr : expr COLON expr\n";
#endif
  $$ = MakeExpr($1, ':', $3);  
}
        ;



term      
        :       BOOLCONST
{
#ifdef PARSE_TRACE
  cout << "Reducing term : BOOLCONST\n";
#endif
  $$ = MakeExpr(strcmp($1, "true")==0);
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
  result *r = new string_result($1);
  $$ = MakeExpr(r);
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
   if (!LegalOp(MINUS, $2)) yyerror(23, "Illegal Operand type on Unary Minus");
   $$ = MakeExpr('!', $2);
}
	|	NOT const_expr
{
#ifdef PARSE_TRACE
  cout << "Reducing const_expr : NOT const_expr\n";
#endif
   if (!LegalOp(NOT, $2)) yyerror(22, "Illegal Operand type on NOT");
   $$ = MakeExpr('!', $2);
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
  function *m = FindMeasure(CallModel, $3, NULL, 0);
  if (m) $$ = new mcall($1, m);
  else   $$ = NULL;
  CallModel = NULL;
}
	|	model_call DOT IDENT indexes
{
  int numparms = $4->Length();
  pos_param **pp = $4->Compress();
  delete $4;
  function *m = FindMeasure(CallModel, $3, pp, numparms);
  if (m) $$ = new mcall($1, m, pp, numparms);
  else   $$ = NULL;
  CallModel = NULL;
}
        ;

model_call
        :       IDENT
{ 
#ifdef PARSE_TRACE
  cout << "Reducing model_call : IDENT\n";
#endif
  $$ = NULL;
  CallModel = NULL;
  char *err = NULL;
  expr *f = Pick($1, NULL, NULL, NULL, &FTable); 
  if (f) {
    if (IsModelType(f->Type())) {
      result *r = f->Compute();
      if (r!=NULL) {
        CallModel = (model *) r->Pointer();
        delete r; 
        $$ = f;
      } else {
        err = Cat("Cannot create instance of model ", $1);    
        // Problems if this occurs 
	delete f;
      }  
    } else {
      err = Cat($1, " is not a model type");
      delete f;
    }
  } 
  if (err != NULL) {
    yyerror(31, err);
    delete err;
  }
}
        |       IDENT LPAR pos_params RPAR
{ 
#ifdef PARSE_TRACE
  cout << "Reducing model_call : IDENT LPAR pos_params RPAR\n";
#endif
  $$ = NULL;
  CallModel = NULL;
  char *err = NULL;
  int numparms = $3->Length();
  pos_param **pp = $3->Compress();
  delete $3;
  bool isover;
  function *find = Pick($1, pp, numparms, false, NULL, NULL, &FTable, isover);
  if (find) {
    if (find->IsModel()) {
      CallModel = (model *) find;
      $$ = new fcall(find, pp, numparms);
    } else err = Cat($1, " is not a model type");
  } 
  if (err) {
    yyerror(32, err);
    delete err;
  }
}
        |       IDENT LPAR named_params RPAR
{
#ifdef PARSE_TRACE
  cout << "Reducing model_call : IDENT LPAR named_params RPAR\n";
#endif
  $$ = NULL;
  CallModel = NULL;
  char *err = NULL;
  int numparms = $3->Length();
  named_param **np = $3->Compress();
  delete $3;
  bool isover;
  function *find = Pick($1, np, numparms, NULL, NULL, &FTable, isover);
  if (find) {
    if (find->IsModel()) {
      CallModel = (model *) find;
      $$ = new fcall(find, np, numparms);
    } else err = Cat($1, " is not a model type");
  } 
  if (err) {
    yyerror(33, err);
    delete err;
  }
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
  $$ = Pick($1, &LocalTable, Model, &MTable, &FTable);
}
	|	IDENT indexes	 
{
#ifdef PARSE_TRACE
  cout << "Reducing function_call : IDENT indexes\n";
#endif
   int numparms = $2->Length();
   pos_param **pp = $2->Compress();
   delete $2;
   function *find = NULL;
   bool isoverloaded;
   find = Pick($1, pp, numparms, true, Model, &MTable, &FTable, isoverloaded);
   if (find) {
     PrintParameterWarning(find->LinkParams(pp, numparms), isoverloaded);
     $$ = new fcall(find, pp, numparms); 
   } else $$ = NULL;
}
	|	IDENT LPAR pos_params RPAR
{
#ifdef PARSE_TRACE
  cout << "Reducing function_call : IDENT LPAR pos_params RPAR\n";
#endif
   int numparms = $3->Length();
   pos_param **pp = $3->Compress();
   delete $3;
   function *find;
   bool isoverloaded;
   find = Pick($1, pp, numparms, false, Model, &MTable, &FTable, isoverloaded);
   if (find) {
     PrintParameterWarning(find->LinkParams(pp, numparms), isoverloaded);
     $$ = new fcall(find, pp, numparms); 
   } else $$ = NULL;
}
        |       IDENT LPAR named_params RPAR
{
#ifdef PARSE_TRACE
  cout << "Reducing function_call : IDENT LPAR named_params RPAR\n";
#endif
   int numparms;
   named_param **np;
   if ($3) {
     numparms = $3->Length();
     np = $3->Compress();
     delete $3;
   } else {
     numparms = 0;
     np = NULL;
   }
   function *find;
   bool isoverloaded;
   find = Pick($1, np, numparms, Model, &MTable, &FTable, isoverloaded);
   if (find!=NULL) {
     PrintParameterWarning(find->LinkParams(np, numparms), isoverloaded);
     $$ = new fcall(find, np, numparms); 
   } else $$ = NULL;
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
   $1->Append($3);
   $$ = $1;
}
	|	formal_param
{
#ifdef PARSE_TRACE
  cout << "Reducing formal_params : formal_param\n";
#endif
   $$ = $1; 
}
	;

formal_param 
	:	type IDENT
{
#ifdef PARSE_TRACE
  cout << "Reducing formal_param : type IDENT\n";
#endif
   formal_param *fp = new formal_param($1, $2);
   ident *I = new ident(fp);
   LocalTable.Insert(I,block); 
   $$ = new List <formal_param> (fp);
}
	|	type IDENT GETS expr
{
#ifdef PARSE_TRACE
  cout << "Reducing formal_param : type IDENT GETS expr\n";
#endif
   formal_param *fp = new formal_param($1, $2, $4);
   ident *I = new ident(fp);
   LocalTable.Insert(I,block); 
   $$ = new List <formal_param> (fp);
}
	;

indexes
	:	indexes LBRAK index RBRAK
{
#ifdef PARSE_TRACE
  cout << "Reducing indexes : indexes LBRAK index RBRAK\n";
#endif
   $1->Append($3);
   $$ = $1;
}
	|	LBRAK index RBRAK
{
#ifdef PARSE_TRACE
  cout << "Reducing indexes : LBRAK index RBRAK\n";
#endif
  $$ = $2;
}
	;

index
        :       expr
{
#ifdef PARSE_TRACE
  cout << "Reducing index : expr\n";
#endif
  if (InModel) {
    ASSERT(Model);
    if (Model->LegalIteratorExpr($1)) {
      $$ = new List <pos_param> (new pos_param($1));
    } else {
      yyerror(42, "Illegal index expression within a model");
      $$ = new List <pos_param> (new pos_param(NULL));
    }
  } else {
    $$ = new List <pos_param> (new pos_param($1));
  }
}
        ;


pos_params 
	:	pos_params COMMA pos_param
{
#ifdef PARSE_TRACE
  cout << "Reducing pos_params : pos_params COMMA pos_param\n";
#endif
   $1->Append($3);
   $$ = $1;
}
	|	pos_param
{
#ifdef PARSE_TRACE
  cout << "Reducing pos_params : pos_param\n";
#endif
  $$ = $1;
}
	;

pos_param 
	:	expr  
{
#ifdef PARSE_TRACE
  cout << "Reducing pos_param : expr\n";
#endif
   $$ = new List <pos_param> (new pos_param($1));
}
	|	DEFAULT
{
#ifdef PARSE_TRACE
  cout << "Reducing pos_param : DEFAULT\n";
#endif
   $$ = new List <pos_param> (new pos_param());
}
	;

named_params 
	:	named_params COMMA named_param
{
#ifdef PARSE_TRACE
  cout << "Reducing named_params : named_params COMMA named_param\n";
#endif
   if ($1) {
     $1->Append($3);
     $$ = $1;
   } else {
     $$ = $3;
   }
}
	|	named_param
{
#ifdef PARSE_TRACE
  cout << "Reducing named_params : named_param\n";
#endif
  $$ = $1;
}
	;

named_param 
	:	IDENT GETS expr
{
#ifdef PARSE_TRACE
  cout << "Reducing named_param : IDENT GETS expr\n";
#endif
   $$ = new List <named_param> (new named_param($1, $3));
}
	|	IDENT GETS DEFAULT
{
#ifdef PARSE_TRACE
  cout << "Reducing named_param : IDENT GETS DEFAULT\n";
#endif
   $$ = NULL;
}
	;



%%
/*-----------------------------------------------------------------*/



