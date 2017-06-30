
Library for Markov chains.

Contents of the directories:

examples/   Example and test applications.


Source files in this directory:

error.cc            Implements the error class
markov_chain.cc     Implements the Markov_chain class
vanishing.cc        Implements the vanishing_chain class
mclib.cc            Implements library information function


----------------------------------------------------------------------
OLD IMPLEMENTATION:

Source files in this directory:

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


