
/*
 *
 *
 * Lex file used to generate tokens for smart.
 *
 */


%{

#include "ParseSM/lexer.h"

#define YY_NO_INPUT

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
letter        [A-Za-z_]
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
{letter}{alphanum}*                   { 
      /* 
          A little heavy, but we grab temporal operator
          characters out of identifiers "by hand".

          First, if we're checking for temporal operators,
          scan the current token text (yytext) for the first
          temporal operator and remember its position.
      */
      long op = -1; 
      if (isTemporalActive()) {
        for (long i=0; i<yyleng; i++) {
          if (isTemporalOperator(yytext[i])) {
            op = i;
            break;
          }
        }
      } /* isTemporalActive() */
      /*
          If the temporal operator is first,
          then that's the token we return;
          push remaining characters back onto input stream
          and process them later.
      */
      if (0==op) {
        yyless(1);
        return ProcessTemporalOperator();
      }
      /*
          If the temporal operator is in the middle
          or not present, then the next token is an
          identifier; any characters from the temporal
          operator onward are pushed back to process later.
      */
      if (op>0) {
        yyless(op);
      } 
      return ProcessID(); 
                                      }
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
"&&"                                  { return ProcessTemporalAnd(); }
.                                     { IllegalToken(); }

%%

