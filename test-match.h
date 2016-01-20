
	VP a,b,res;

	printf("TEST_MATCH\n");
	// test mixing int vectors and lists of int vectors
	res=match(xin(1,1),xln(1,xi(1)));
	ASSERT(_equal(res,xin(1,0)),"ma0");
	res=match(xln(1,xi(1)),xln(1,xi(1)));
	ASSERT(_equal(res,xin(1,0)),"ma1");
	res=match(xln(1,xi(1)),xln(1,xi(1)));
	ASSERT(_equal(res,xin(1,0)),"ma2");

	// simpler stuff
	res=match(xin(1,1),xin(1,1));
	ASSERT(_equal(res,xin(1,0)),"ma");
	res=match(xin(2,1,2),xin(1,1));
	ASSERT(_equal(res,xin(1,0)),"mb");
	res=match(xin(2,1,2),xin(1,2));
	ASSERT(_equal(res,xin(1,1)),"mc");
	res=match(xin(3,1,2,3),xin(1,2));
	ASSERT(_equal(res,xin(1,1)),"mcc");
	res=match(xin(3,1,2,3),xin(1,3));
	ASSERT(_equal(res,xin(1,2)),"mccc");

	// simple anyof
	res=match(xin(2,1,2),tagv("anyof",xin(1,2)));
	ASSERT(_equal(res,xin(1,1)),"md");
	res=match(xin(2,1,2),tagv("anyof",xin(2,2,3)));
	ASSERT(_equal(res,xin(1,1)),"me");
	res=match(xin(2,1,2),tagv("anyof",xin(2,1,3)));
	ASSERT(_equal(res,xin(1,0)),"mf");
	res=match(xin(2,1,2),tagv("anyof",xin(2,1,3)));
	ASSERT(_equal(res,xin(1,0)),"mg");

	// simple greedy, start, exact
	res=match(xin(2,5,6),tagv("greedy",xin(1,5)));
	ASSERT(_equal(res,xin(1,0)),"mh0");
	res=match(xin(2,5,6),tagv("greedy",xin(1,6)));
	ASSERT(_equal(res,xin(1,1)),"mh0a");

	res=match(xin(3,5,5,7),tagv("greedy",xin(1,5)));
	ASSERT(_equal(res,xin(2,0,1)),"mh0b");
	res=match(xin(4,3,3,9,9),tagv("greedy",xin(1,9)));
	ASSERT(_equal(res,xin(2,2,3)),"mh1");
	res=match(xin(4,2,9,2,2),tagv("greedy",xin(1,9)));
	ASSERT(_equal(res,xin(1,1)),"mh2");

	res=match(xin(4,2,1,1,1),tagv("greedy",xin(1,1)));
	ASSERT(_equal(res,xin(3,1,2,3)),"mh3");

	res=match(xin(3,1,1,2),tagv("start",xin(1,1)));
	ASSERT(_equal(res,xin(1,0)),"mi");
	res=match(xin(3,1,1,2),tagv("start",xin(1,2)));
	ASSERT(_equal(res,xi0()),"mj");
	res=match(xin(3,1,1,2),tagv("exact",xin(1,1)));
	ASSERT(_equal(res,xi0()),"mk");
	res=match(xin(3,1,1,2),tagv("exact",xin(2,1,1)));
	ASSERT(_equal(res,xi0()),"ml");
	res=match(xin(3,1,1,2),tagv("exact",xin(3,1,1,2)));
	ASSERT(_equal(res,xin(3,0,1,2)),"mn");

	res=match(xin(4,1,2,3,4),xin(2,3,4));
	ASSERT(_equal(res,xin(2,2,3)),"mo");

	// mixed types
	PFW({
	res=match(xin(4,7,8,5,6),xln(3,
		tagv("anyof", xin(2,7,8)),
		tagv("greedy", xi0()),
		tagv("anyof", xin(2,5,6))));
	ASSERT(_equal(res,xin(4,0,1,2,3)),"mp");
	});

	res=match(xin(4,9,8,7,6),tagv("greedy", xln(3,
		tagv("anyof", xin(2,9,8)),
		xi0(),
		tagv("anyof", xin(1,6)))));
		ASSERT(_equal(res,xin(4,0,1,2,3)),"mq");

	ASSERT(_equal(match(xin(4,1,2,3,4),xin(4,1,2,3,4)),xin(4,0,1,2,3)),"m0");
	ASSERT(_equal(match(xin(4,1,2,3,4),xin(2,1,2)),    xin(2,0,1)),"m1");
	ASSERT(_equal(match(xin(4,1,2,3,4),xin(1,1)),      xin(1,0)),"m2");
	ASSERT(_equal(match(xin(4,1,2,3,4),xin(1,2)),      xin(1,1)),"m3");
	ASSERT(_equal(match(xin(4,1,2,3,4),xin(2,2,3)),    xin(2,1,2)),"m4");

	ASSERT(_equal(match(xln(1,xi(9)),xin(2,9,8)),      xin(1,0)),"ml0");
	ASSERT(_equal(match(xln(2,xi(9),xi(8)),xin(2,9,8)),xin(2,0,1)),"ml1");

	ASSERT(_equal(
		match(
			a=xin(4,1,2,3,4),
			b=tagv("anyof",xin(4,5,6,7,8))
		), res=xi0()), "m5");
	xfree(a);xfree(b);xfree(res);

	ASSERT(_equal(
		match(
			a=xin(5,1,2,3,4,5),
			b=tagv("anyof",xin(3,5,6,7))
		), res=xin(1,4)), "m6");
	ASSERT(_equal(
		match(
			xin(5,5,8,7,6,4),
			tagv("anyof",xin(4,9,8,7,6))
		), xin(3,1,2,3)), "m7");
	ASSERT(_equal(
		match(
			xin(5,5,8,7,6,4),
			tagv("anyof",xin(3,1,1,1))
		), xi0()), "m7b");
	ASSERT(_equal(
		apply(xin(5,5,8,7,6,4), match(
			xin(5,5,8,7,6,4),
			tagv("anyof",xin(4,9,8,7,6))
		)), xin(3,8,7,6)), "m8");
	ASSERT(_equal(
		match(xin(5,1,2,3,4,1),
			tagv("greedy",
				xln(3, xi(1), 
						tagv("anyof",xin(3,2,3,4)), 
					xi(1))
			)), xin(5,0,1,2,3,4)), "m9");
	ASSERT(_equal(
		match(
			xln(3,xfroms("\""),xfroms("blah"),xfroms("\"")),
			tagv("greedy",
				xln(3,xfroms("\""),xc0(),xfroms("\"")))
		),xin(3,0,1,2)), "m10");
	ASSERT(_equal(
		match(
			xln(4,xi(100),xfroms("\""),xfroms("blah"),xfroms("\"")),
			tagv("greedy",
				xln(3,xfroms("\""),xc0(),xfroms("\"")))
		),xin(3,1,2,3)), "m11");
	ASSERT(_equal(
		match(xin(5,1,0,1,0,2),xi(1)),
		xin(2,0,2)), "m multi0");
	/*
	a=xin(4, 1, 2, 3, 4);
	b=xin(4, 1, 2, 3, 4);
	res=match(a,b);
	DUMP(res);
	ASSERT(res->n==4 && AS_i(res,0) == 0 && AS_i(res,3)==3, "match 0");
	xfree(res); xfree(a); xfree(b);

	a=xin(4, 1, 2, 3, 4);
	b=xin(2, 1, 2);
	res=match(a,b);
	DUMP(res);
	ASSERT(res->n==2 && AS_i(res,0) == 0 && AS_i(res,1)==1, "match 1");
	xfree(res); xfree(a); xfree(b);

	a=xin(4, 1, 2, 3, 4);
	b=xin(2, 2, 3);
	res=match(a,b);
	DUMP(res);
	*/
