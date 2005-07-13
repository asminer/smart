
$Id$

Smart version 2, "Language" module.

Files, and what they define, in a roughly layered sense.

File	  	What's there
----------	----------------------------------------------------
results		The result class.  Functions to print and delete results.

exprs		Engine information class.
		Expression class base.
		Symbols.
		Constant bool, int, and real expressions.

stmts		Statements.

sets		Set expressions and their results.

variables	Functions with no parameters.

functions	User-defined functions and internal functions.

arrays		User-defined arrays.
		For loops.
		Array assigmnent statements.

operators	Operator expressions for all types, including sets.  
		This is so huge that it is split into its own hierarchy:
		baseops		Base classes for operators like "+"
		ops_bool	Operators for "bool" types
		ops_int		Operators for "int" types
		ops_real	Operators for "real" types
		ops_misc	Operators for misc. types

casting		Type casting operators for all types.

converge	Converge statements (including within arrays)

infinity	Infinity constants.

strings		String results and operators.

measures	Measures (special case of variable)

api		Front end

