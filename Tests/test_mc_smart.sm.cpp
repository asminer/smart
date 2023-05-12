dtmc test :={
	state a,b,c,d,e;
	init(a:1);
	arcs(
		a:b:1;
		b:c:1;
		c:d:1;
		d:d:1;
		d:e:1;
		e:e:1
		);
	stateset abs := potential(is_absorbed);

  	stateprobs prop1 := reverse_tta(5,abs);

};
print("test:::::::::",test.prop1,"\n");
