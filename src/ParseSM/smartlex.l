
/*
 *
 * $Id$
 *
 *
 * Lex file used to generate tokens for smart.
 *
 */


%{

#include "lexer.h"

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
dec           ("."{digit}+)
exp           ([eE][-+]?{digit}+)
alphanum      [A-Za-z0-9_]
qstring       (\"[^\"\n]*[\"\n])
notendl       [^\n]

%%

<OPTION>\n                            { BEGIN 0; return ProcessEndpnd(); }
"//"{notendl}*                        { /* C++ comment, ignored */ }
{comstart}{notcomclose}*{comclose}    { /* C comment, ignored */ }
{comstart}{notcomclose}*              { UnclosedComment(); }
\n                                    { /* Ignored */ }
{white}                               { /* Ignored */ }
"#"{white}"include"{white}{qstring}   { Include(); }
"converge"                            { return ProcessConverge(); }
"default"                             { return ProcessDefault(); }
"for"                                 { return ProcessFor(); }
"guess"                               { return ProcessGuess(); }
"in"                                  { return ProcessIn(); }
"null"                                { return ProcessNull(); }
"false"                               { return ProcessBool(); }
"true"                                { return ProcessBool(); }
{qstring}                             { return ProcessString(); }
{letter}{alphanum}*                   { return ProcessID(); }
{digit}+                              { return ProcessInt(); }
{digit}+{dec}?{exp}?                  { return ProcessReal(); } 
"("                                   { return ProcessLpar(); }
")"                                   { return ProcessRpar(); }
"["                                   { return ProcessLbrak(); }
"]"                                   { return ProcessRbrak(); }
"{"                                   { return ProcessLbrace(); }
"}"                                   { return ProcessRbrace(); }
","                                   { return ProcessComma(); }
";"                                   { return ProcessSemi(); }
":"                                   { return ProcessColon(); }
"."                                   { return ProcessDot(); }
".."                                  { return ProcessDotdot(); }
":="                                  { return ProcessGets(); }
"+"                                   { return ProcessPlus(); }
"-"                                   { return ProcessMinus(); }
"*"                                   { return ProcessTimes(); }
"/"                                   { return ProcessDivide(); }
"%"                                   { return ProcessMod(); }
"|"                                   { return ProcessOr(); }
"&"                                   { return ProcessAnd(); }
"\\"                                  { return ProcessSetDiff(); }
"->"                                  { return ProcessImplies(); }
"!"                                   { return ProcessNot(); }
"=="                                  { return ProcessEquals(); }
"!="                                  { return ProcessNequal(); }
">"                                   { return ProcessGt(); }
">="                                  { return ProcessGe(); }
"<"                                   { return ProcessLt(); }
"<="                                  { return ProcessLe(); }
"#"                                   { BEGIN OPTION;  return ProcessPound(); }
.                                     { IllegalToken(); }

%%

