
States module.

Classes to define discrete states, packed sets of discrete states,
final representations for state spaces (for numerical stuff), and such.

Files:

stateheap	Definition of state class, and centralized functions
		to create and destroy states & substates.

flatss		Collection of compressed states for flat storage.
		For now, copied from version 1;  soon to be updated ;^)

reachset	Class used to store a reachability set.
		We allow three types of storage: 
		"enumerated", where the states are explicitly named 
			as done by a Markov chain;
		"explicit", stored using flatss 
			(eventually: either canonical order or lexical)
		"evmdd", a K-level MDD with offsets stored along edges.

mdds		Temporary mdd library

testflat	Not included in SMART, but used to test the
		state_array class in flatss.

testmdd		Not included in SMART; used to test the mdd library

