
/*
 *
 *
 * Lex file used to generate tokens for ICP
 *
 */


%{

#include "ParseICP/lexer.h"

%}

%s OPTION

%option yylineno
%option nounput

ws            [\r\t ]
white         ({ws}*)
comstart      "/*"
comclose      "*"+"/"
notspecial    ([^/*])
notcomclose   ([^*]|"*"+{notspecial})
letter        [A-Za-z]
digit         [0-9]
alphanum      [A-Za-z0-9_]
qstring       (\"[^\"\n]*[\"\n])
notendl       [^\n]

%%

<OPTION>\n                          { BEGIN 0; return ProcessEndpnd(); }
"//"{notendl}*\n                    { /* C++ comment, ignored */ }
{comstart}{notcomclose}*{comclose}  { /* C comment, ignored */ }
{comstart}{notcomclose}*            { UnclosedComment(); }
\n                                  { /* Ignored */ }
{white}                             { /* Ignored */ }
"#"{white}"include"{white}{qstring} { Include(); }
"maximize"                          { return ProcessMaximize(); }
"minimize"                          { return ProcessMinimize(); }
"satisfiable"                       { return ProcessSatisfiable(); }
"in"                                { return ProcessIn(); }
"false"                             { return ProcessBool(); }
"true"                              { return ProcessBool(); }
{letter}{alphanum}*                 { return ProcessID(); }
{digit}+                            { return ProcessInt(); }
"("                                 { return ProcessLpar(); }
")"                                 { return ProcessRpar(); }
"{"                                 { return ProcessLbrace(); }
"}"                                 { return ProcessRbrace(); }
","                                 { return ProcessComma(); }
";"                                 { return ProcessSemi(); }
".."                                { return ProcessDotdot(); }
":="                                { return ProcessGets(); }
"+"                                 { return ProcessPlus(); }
"-"                                 { return ProcessMinus(); }
"*"                                 { return ProcessTimes(); }
"/"                                 { return ProcessDivide(); }
"%"                                 { return ProcessMod(); }
"|"                                 { return ProcessOr(); }
"&"                                 { return ProcessAnd(); }
"->"                                { return ProcessImplies(); }
"!"                                 { return ProcessNot(); }
"=="                                { return ProcessEquals(); }
"!="                                { return ProcessNequal(); }
">"                                 { return ProcessGt(); }
">="                                { return ProcessGe(); }
"<"                                 { return ProcessLt(); }
"<="                                { return ProcessLe(); }
"#"                                 { BEGIN OPTION;  return ProcessPound(); }
.                                   { IllegalToken(); }

%%

