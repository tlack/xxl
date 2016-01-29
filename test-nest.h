	// historical note:
	// nest() used to always break apart the matched pieces, as shown here:
	// a=xin(3,1,2,3);
	// b=xin(2,1,3);
	// ASSERT(_equal(c, xl(xln(3, xi(1), xi(2), xi(3)))), "nest 0");
	// this is because of the dumb iterative algorithm in the old version;
	// though part of the api's contract, i dont think it makes sense.
	// i'd like to avoid this for like-typed inputs for now, so im changing
	// these tests. it still does wrap non-list results in a list, because
	// nest is supposed to "create" lists out of items (like and unlike) 
	// @tlack 1/25
	PFW({
	a=xin(3,1,2,3);
	b=xin(2,1,3);
	c=nest(a,b);
	DUMP(c);
	DUMP(info(c));
	if(LIST(c))
		DUMP(info(ELl(c,0)));
	ASSERT(_equal(c, xl(xin(3, 1, 2, 3))), "nest 0");
	
	a=xin(5,9,0,0,0,8);
	b=xin(2,9,8);
	c=nest(a,b);
	DUMP(c);
	ASSERT(_equal(c,xl(xin(5,9,0,0,0,8))),"nest 1");

	a=xin(5,9,0,0,0,8);
	b=xin(2,6,6);
	c=nest(a,b);
	PF("nest2:\n");
	DUMP(c);
	DUMP(info(c));
	ASSERT(_equal(c,xin(5,9,0,0,0,8)),"nest 2");

	a=xin(5,9,1,0,2,8);
	b=xin(2,1,2);
	PF("nest3 call:\n");
	c=nest(a,b);
	DUMP(c);
	ASSERT(_equal(c,xln(3,xi(9),xl(xin(3,1,0,2)),xi(8))),"nest 3");

	a=xin(5,9,1,0,2,8);
	b=xin(2,2,8);
	PF("nest4 call:\n");
	c=nest(a,b);
	DUMP(c);
	printf("%s\n",reprA(c));
	ASSERT(_equal(c,xln(4,xi(9),xi(1),xi(0),xin(2,2,8))),"nest 4");


	a=xin(7,9,1,1,0,2,2,8);
	b=xin(2,1,2);
	PF("nest5 call:\n");
	c=exhaust(a,mkproj(2,&nest,0,b));
	//c=nest(a,b);
	DUMP(c);
	ASSERT(_equal(c,xln(3, 
		xi(9), 
			xln(3, xi(1), 
					xin(3, 1,0,2), 
				xi(2)), 
		xi(8))),"nest 5");

	a=xin(5,7,7,0,8,8);
	b=xln(2, xin(2,7,7), xin(2,8,8));
	c=nest(a,b);
	DUMP(c);
	DUMP(info(c));
	DUMP(each(c,x1(&info)));
	ASSERT(_equal(c, xl( xin(5,7,7,0,8,8) )), "nest multi 0");


	});
