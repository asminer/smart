
/*
 *
 * $Id$
 *
 *
 * Lex file used to generate tokens for smart.
 *
 * Now we assume that this is going to generate a c++ lexer, by flex
 *
 *
 */


%{

#include "../../Source/Base/api.h"
#include "../../Source/Language/api.h"
#include "../../Source/Templates/stack.h"
//#include "smart.tab.h"
#include <string.h>
#include <fstream>
#include <FlexLexer.h>  // For the new & improved lexer class
//#define DEBUG_LEX

/// Set of input files on the command line
char **inputFiles;

/// Number of input files on the command line
int numInputFiles;

/// Which cmdline input file we're currently processing
int whichInputFile;

/// Name of current input file
char* filename;

yyFlexLexer lexer;

struct fileinfo {
  char* filename;  
  bool canDeleteFilename;
  YY_BUFFER_STATE buffer;
  fileinfo(char *f, bool kill) {
    filename = f;
    canDeleteFilename = kill;
    buffer = (YY_BUFFER_STATE) NULL;
  }
  void SetBuffer(YY_BUFFER_STATE b) {
    buffer = b;
  }
  void Clear() {
    if (canDeleteFilename) free(filename);  
    filename = NULL;
    if (buffer) lexer.yy_delete_buffer(buffer); 
  }
  int operator == (fileinfo f) {
    return 0==strcmp(filename, f.filename);
  }
};

Stack<fileinfo> FileStack(256);

/** Implements file inclusion.
    Now handled completely by the lexer.
    
    We have two possibilities here.
    \begin{enumerate}
    \item  
    
      n is not NULL :
        Push a new file onto the stack and start reading from it.
        If there were any errors (such as a circular dependency or
	file not found) then we return NULL;
	otherwise we return the name of the file.

    \item 

      n is NULL :
        Trash the current file and pop the next one off the stack.
	We return NULL if the stack is empty, otherwise the current file.

    \end{enumerate}

    @param	name	The file name to start reading, or NULL to
    			pop from the stack.

    @param	kill	Can we trash the name when we're done

    @return	The current file we're reading from or NULL if
    		further switching is required.

*/

char* IncludeFile(char *name, bool kill)
{
  if (name) {
    // Start a new file
#ifdef DEBUG_LEX
    cout << "Lexer: opening file " << name << "\n";
#endif
    fileinfo fi(name, kill);
    // First: check for circular dependency
    if (FileStack.Contains(fi)) {
	Warning.Start(filename, lexer.lineno());
	Warning << "circular dependency caused by #include "; 
	Warning << name << ", ignoring";
	Warning.Stop();
	return filename;
    } 
    // Second: try to open the file
    ifstream *infile = new ifstream(name);
    if (!infile->good()) {
        delete infile;
        Error.Start(filename, lexer.lineno());	
	Error << "couldn't open file " << name << ", ignoring";
	Error.Stop();
        return NULL;
    }
    // We have a good file, set up the buffer
    YY_BUFFER_STATE b = lexer.yy_create_buffer(infile, YY_BUF_SIZE);
    fi.SetBuffer(b);
    FileStack.Push(fi);
    lexer.yy_switch_to_buffer(b);
    return name;
  } 

  // NULL name, pop next file off stack
  fileinfo fi = FileStack.Pop();
#ifdef DEBUG_LEX
  cout << "Lexer: done with file " << fi.filename << "\n";
#endif
  fi.Clear();
  if (FileStack.Empty()) {
    // empty stack
    return NULL;
  }

  fi = FileStack.Top();
  lexer.yy_switch_to_buffer(fi.buffer);
#ifdef DEBUG_LEX
  cout << "Lexer: reverting to file " << filename << "\n";
#endif
  return fi.filename;
}

/*  Start reading the next file from the command line
*/
void NextFile()
{
  filename = ">";
#ifdef DEBUG_LEX
  cout << "Lexer: next cmdline file " << inputFiles[whichInputFile] << endl;
#endif
  if (strcmp(inputFiles[whichInputFile], "-")==0) {
    istream *thing = &cin;
    YY_BUFFER_STATE b = lexer.yy_create_buffer(thing, YY_BUF_SIZE);
    lexer.yy_switch_to_buffer(b);
    fileinfo fi("-", false);
    fi.SetBuffer(b);
    FileStack.Push(fi);
    filename = "-";
  } else {
    filename = IncludeFile(inputFiles[whichInputFile], false);
  }
  whichInputFile++;
}

int ProcessID()
{
  const char *yytext = lexer.YYText();
  yylval.name = strdup(yytext);
  if (FindSimpleType(yytext)!=NO_SUCH_TYPE) return TYPE;
  if (FindModif(yytext)!=NO_SUCH_MODIF) return MODIF;
  if (FindModel(yytext)!=NO_SUCH_TYPE) return MODEL;
  if (FindVoidType(yytext)!=NO_SUCH_TYPE) return TYPE;
  if (strcmp(yytext, "proc")==0) return PROC;
  return IDENT;
}

int ProcessNum(int type)
{
  yylval.name = strdup(lexer.YYText());
  return type; 
}

int ProcessString(int type)
{
/*  This is copied from the lex&yacc book, p84 */

  yylval.name = strdup(lexer.YYText()+1);  /* skip open quote */
  if (yylval.name[lexer.YYLeng()-2] != '"') {
    Error.Start(filename, lexer.lineno());
    Error << "Unclosed quote";
    Error.Stop();
  } else 
    yylval.name[lexer.YYLeng()-2] = '\0'; /* erase close quote */
  return type;
}

void UnclosedComment()
{
  Error.Start(filename, lexer.lineno());
  Error << "Unclosed comment";
  Error.Stop();
}

void IllegalToken()
{
  Error.Start(filename, lexer.lineno());
  Error << "Illegal syntactical element: '" << lexer.YYText() << "'";
  Error.Stop();
}

void yyerror(char *msg)
{
  Error.Start(filename, lexer.lineno());
  Error << msg;
  Error.Stop();
}

void Include()
{
  // Get the filename.  Ignore everything until first quote
  const char *token = lexer.YYText();
  int tokenlen = strlen(token);
  int start;
  int stop;
  for(start=0; start<tokenlen; start++) 
    if (token[start]=='"') break;
  if (start>=tokenlen) {
    Internal.Start(__FILE__, __LINE__, filename, lexer.lineno());
    Internal << "Include() error, missing quote?";
    Internal.Stop();
    return;
  }
  // stop should be the last char, but who knows?
  for (stop=tokenlen-1; stop>start; stop--)
    if (token[stop]=='"') break;
  if (stop<=start) {
    Internal.Start(__FILE__, __LINE__, filename, lexer.lineno());
    Internal << "Include() error, missing final quote?";
    Internal.Stop();
    return;
  }

  // Bail out for empty strings
  if (stop-1==start) {
    Warning.Start(filename, lexer.lineno());
    Warning << "Empty filename for include, ignoring";
    Warning.Stop();
    return;
  }

  char* fn = (char*) malloc(stop-start);
  fn[stop-start-1] = 0;
  int p;
  for (p=start+1; p<stop; p++) fn[p-start-1] = token[p];

  filename = IncludeFile(fn, true);
}

%}

%s OPTION

%option c++ yylineno

ws     		[\t ]
white		({ws}*)
comstart  	"/*"
comclose  	"*"+"/"
notspecial 	([^/*])
notcomclose 	([^*]|"*"+{notspecial})
letter    	[A-Za-z]
digit     	[0-9]
dec       	("."{digit}+)
exp       	([eE][-+]?{digit}+)
alphanum  	[A-Za-z0-9_]
qstring    	(\"[^\"\n]*[\"\n])
notendl   	[^\n]

%%

<OPTION>\n  			{ BEGIN 0; return ENDPND; }
"//"{notendl}*\n	          	{ /* C++ comment, ignored */ }
{comstart}{notcomclose}*{comclose} 	{ /* C comment, ignored */ }
{comstart}{notcomclose}*           	{ UnclosedComment(); }
\n                                	{ /* Ignored */ }
{white}                          	{ /* Ignored */ }
"#"{white}"include"{white}{qstring}   	{ Include(); }
"converge"			{ return CONVERGE; }
"default"                      	{ return DEFAULT; }
"end"                          	{ return END; }
"for"                          	{ return FOR; }
"guess"                        	{ return GUESS; }
"in"                           	{ return IN; }
"null"                         	{ return NUL; }
"false"                        	{ return ProcessNum(BOOLCONST); }
"true"                         	{ return ProcessNum(BOOLCONST); }
{letter}{alphanum}*            	{ return ProcessID(); }
{digit}+                       	{ return ProcessNum(INTCONST); }
{digit}+{dec}?{exp}?           	{ return ProcessNum(REALCONST); }
{qstring}                      	{ return ProcessString(STRCONST); }
"("         { return LPAR; }
")"         { return RPAR; }
"["         { return LBRAK; }
"]"         { return RBRAK; }
"{"         { return LBRACE; }
"}"         { return RBRACE; }
","         { return COMMA; }
";"         { return SEMI; }
":"         { return COLON; }
"."         { return DOT; }
".."        { return DOTDOT; }
":="        { return GETS; }
"+"         { return PLUS; }
"-"         { return MINUS; }
"*"         { return TIMES; }
"/"         { return DIVIDE; }
"|"         { return OR; }
"&"         { return AND; }
"!"         { return NOT; }
"=="        { return EQUALS; }
"!="        { return NEQUAL; }
">"         { return GT; }
">="        { return GE; }
"<"         { return LT; }
"<="        { return LE; }
"->"        { return ARROW; }
"#"         { BEGIN OPTION;  return POUND; }
.           { IllegalToken(); }

%%

int yylex()
{
  int token = 0;
  while (0==token) {
    if (NULL==filename) {
      // current file is at EOF 
      if (!FileStack.Empty()) {
	// Revert back
	filename = IncludeFile(NULL, false);
	continue;
      }
      if (whichInputFile < numInputFiles) {
	// Next from command line
	NextFile();
	continue;
      }
      // we must be done
#ifdef DEBUG_LEX
      cout << "Lexer: end of input files\n";
#endif
      return 0;  
    }

    // Ok, we are reading from a valid file...
    token = lexer.yylex();

    if (0==token) filename = NULL;  // Hit EOF and loop
  }
#ifdef DEBUG_LEX
  cout << "Lexer: token " << token << "\n";
#endif
  return token;
}

