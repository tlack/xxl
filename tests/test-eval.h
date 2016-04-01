
	VP a;
	printf("TEST_EVAL\n");
	ASSERT(
		_equal(
			parsestr("// "),
			entags(xfroms("// \n"),"comment")
		), "tec0");
	ASSERT(
		_equal(
			parsestr("/* */"),
			xln(2,entags(xfroms("/* */"),"comment"),xfroms("\n"))
		), "tec0b");
	ASSERT(
		_equal(
			parsestr("/*a*/ /*z*/"),
			xln(4,
				entags(xfroms("/*a*/"),"comment"),
				xfroms(" "),
				entags(xfroms("/*z*/"),"comment"),
				xfroms("\n"))
		), "tec1bbbb");
	ASSERT(
		_equal(
			parsestr("//x"),
			xl(entags(xfroms("//x\n"),"comment"))
		), "tec1");
	ASSERT(
		_equal(
			parsestr("/*x*/"),
			xln(2,entags(xfroms("/*x*/"),"comment"),xfroms("\n"))
		), "tec1b");
	ASSERT(
		_equal(
			parsestr("//xy"),
			xl(entags(xfroms("//xy\n"),"comment"))
		), "tec1c");
	ASSERT(
		_equal(
			parsestr("//x "),
			xl(entags(xfroms("//x \n"),"comment"))
		), "tec2");
	ASSERT(
		_equal(
			parsestr("// x"),
			xl(entags(xfroms("// x\n"),"comment"))
		), "tec2b");
	ASSERT(
		_equal(
			parsestr("// abc "),
			xl(entags(xfroms("// abc \n"),"comment"))
		), "tec3");
	ASSERT(
		_equal(
			parsestr("// a\n//b"),
			xln(2,
				entags(xfroms("// a\n"),"comment"),
				entags(xfroms("//b\n"),"comment"))
		), "tec4");
	ASSERT(
		_equal(
			parsestr("/* abc */"),
			xln(2,entags(xfroms("/* abc */"),"comment"),xfroms("\n"))
		), "tec5");
	ASSERT(
		_equal(
			parsestr("1"),
			xln(2,xi(1),xfroms("\n"))
		), "tei0");
	ASSERT(
		_equal(
			parsestr("1//blah"),
			xln(2,
				xi(1),
				entags(xfroms("//blah\n"),"comment"))
		), "teic0");
	ASSERT(
		_equal(
			parsestr("1//blah\n2"),
			xln(4,
				xi(1),
				entags(xfroms("//blah\n"),"comment"),
				xi(2),
				xfroms("\n")
			)
		), "teic1");
	//DUMP(parsestr("// test"));
	//parsestr("// test\nx:\"Hello!\"\ncount 1024");
