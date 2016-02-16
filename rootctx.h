
	// postfix/unary operators
	// res=assign(res,xt(_tagnums("]")),x1(&identity));
	res=assign(res,Tt(arity),x1(&arity));
	res=assign(res,Tt(condense),x1(&condense));
	res=assign(res,Tt(count),x1(&count));
	res=assign(res,Tt(curtail),x1(&curtail));
	res=assign(res,Tt(info),x1(&info));
	res=assign(res,Tt(flat),x1(&flatten));
	res=assign(res,Tt(last),x1(&last));
	res=assign(res,Tt(len),x1(&len));
	res=assign(res,Tt(key),x1(&key));
	res=assign(res,Tt(min),x1(&min));
	res=assign(res,Tt(max),x1(&max));
	res=assign(res,Tt(not),x1(&not));
	res=assign(res,Tt(parse),x1(&parse));
	res=assign(res,Tt(repr),x1(&repr));
	res=assign(res,Tt(rev),x1(&reverse));
	res=assign(res,Tt(show),x1(&show));
	res=assign(res,Tt(str),x1(&str));
	res=assign(res,Tt(sum),x1(&sum));
	res=assign(res,Tt(sums),x1(&sums));
	res=assign(res,Tt(sys),x1(&sys));
	res=assign(res,Tt(type),x1(&type));
	res=assign(res,Tt(val),x1(&val));
	res=assign(res,Tt(ver),xi(0));

	// infix/binary operators
	res=assign(res,Tt(=),x2(&equal));
	res=assign(res,Tt(+),x2(&plus));
	res=assign(res,Tt(-),x2(&minus));
	res=assign(res,Tt(*),x2(&times));
	res=assign(res,Tt(/),x2(&divv));
	res=assign(res,Tt(%),x2(&mod));
	res=assign(res,Tt(&),x2(&and));
	res=assign(res,Tt(|),x2(&or));
	res=assign(res,Tt(^),x2(&xor));
	res=assign(res,xt(_tagnums("<")),x2(&lesser));
	res=assign(res,xt(_tagnums(">")),x2(&greater));
	res=assign(res,Tt(!),x2(&amend));
	res=assign(res,Tt(@),x2(&apply));
	res=assign(res,Tt(:),x2(&dict)); // gcc gets confused by Tt(,) - thinks its two empty args
	res=assign(res,Tt(?),x2(&find1));
	res=assign(res,Tt(~),x2(&matcheasy));
	res=assign(res,xt(_tagnums(",")),x2(&join)); // gcc gets confused by Tt(,) - thinks its two empty args
	res=assign(res,Tt(bracketj),x2(&bracketjoin));
	res=assign(res,Tt(cast),x2(&cast));
	res=assign(res,Tt(consecj),x2(&consecutivejoin));
	res=assign(res,Tt(deal),x2(&deal));
	res=assign(res,Tt(deep),x2(&deep));
	res=assign(res,Tt(drop),x2(&drop));
	res=assign(res,Tt(each),x2(&each));
	res=assign(res,Tt(evalin),x2(&evalin));
	res=assign(res,Tt(get),x2(&get));
	res=assign(res,Tt(iftrue),x2(&iftrue));
	res=assign(res,Tt(ifelse),x2(&ifelse));
	res=assign(res,Tt(in),x2(&matchany));
	res=assign(res,Tt(loadin),x2(&loadin));
	res=assign(res,Tt(nest),x2(&nest));
	res=assign(res,Tt(pick),x2(&pick));
	res=assign(res,Tt(over),x2(&over));
	res=assign(res,Tt(rot),x2(&shift));
	res=assign(res,Tt(split),x2(&split));
	res=assign(res,Tt(take),x2(&take));
	res=assign(res,Tt(wide),x2(&wide));

	//stdlib
	VP d;
	#ifdef STDLIBFILE
	d=xd0();
	d=assign(d,Tt(get),x1(&fileget));
	d=assign(d,Tt(set),x2(&fileset));
	res=assign(res,Tt(file),d);
	xfree(d);
	#endif

	#ifdef STDLIBSHAREDLIB
	d=xd0();
	d=assign(d,Tt(get),x1(&sharedlibget));
	d=assign(d,Tt(set),x2(&sharedlibset));
	res=assign(res,Tt(sharedlib),d);
	xfree(d);
	#endif

	#ifdef STDLIBSHELL
	d=xd0();
	d=assign(d,Tt(get),x1(&shellget));
	res=assign(res,Tt(shell),d);
	xfree(d);
	#endif

	#ifdef STDLIBNET
	d=xd0();
	d=assign(d,Tt(bind),x2(&netbind));
	res=assign(res,Tt(net),d);
	xfree(d);
	#endif

