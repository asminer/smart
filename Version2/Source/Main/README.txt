
$Id$

Smart version 2, "Main" module.

Files, and what they define, in a roughly layered sense.

File	  	What's there
----------	----------------------------------------------------
fnlib		Built-in functions (not for models, though)

tables		Symbol table classes.

compile		Compile-time support functions used by yacc file.

smart.l		Lex file.
		Handles file inclusion and command-line stuff.

smart.y		Yacc file (ONLY).
		Minimalist reduction rules. 
		Everything calls some function in "compile.h",
		this keeps readability high.

api		Functions smart_main and smart_exit
