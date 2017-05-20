
# $Id$

The library is implemented via the following source files.

matrix.cc	Fancy sparse matrix storage class.

mcbase.cc	Implementation of the Markov chain analysis methods,
		via an abstract base class that contains the final
		"finished" data structures.  Derived classes handle
		specific ways of building Markov chains; they will
		all "compile" down to the same finished data structures.

mclib.cc	Implementation of the abstract Markov chain class
		defined in the library interface, also the library
		information function.

poiss.cc	Computes the poisson distribution using the algorithm
		of Fox & Glynn.

sccs.cc		Efficient algorithm to determine SCCs (strongly 
		connected components).

