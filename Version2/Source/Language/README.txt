
$Id$

Smart version 2, "Language" module.

Files, and what they define, in a roughly layered sense.

File	  	What's there
----------	----------------------------------------------------
types		Definitions of type constants, and some handy 
		functions (mostly used by the compiler)

exprs		Result class.  Engine information class.
		Expression class base.
		Symbols.
		Constant bool, int, and real expressions.

stmts		Statements.

sets		Set expressions and their results.

options

infinity	Infinity constants.

variables	Functions with no parameters.

functions	User-defined functions and internal functions.

arrays		User-defined arrays.
		For loops.
		Array assigmnent statements.

baseops		Base classes for operators like "+"
		(to reduce cut&paste of virtual functions)

operators	Operator expressions for all types,
		including sets.

casting		Type casting operators for all types.

initfuncs	Initializes built-in (not model-related) functions

api		Front end

