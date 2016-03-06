
	// postfix/unary operators
	// .. sequences and lists
	res=assign(res,xt(_tagnums(".|")),x1(&first));
	res=assign(res,xt(_tagnums("|.")),x1(&last));
	res=assign(res,xt(_tagnums("_|")),x1(&curtail));
	res=assign(res,xt(_tagnums("|_")),x1(&behead));
	// ..logic
	res=assign(res,xt(_tagnums("~>")),x1(&condense));
	// named postfix verbs:
	res=assign(res,Tt(any),x1(&any));
	res=assign(res,Tt(arity),x1(&arity));
	res=assign(res,Tt(behead),x1(&behead));
	res=assign(res,Tt(condense),x1(&condense));
	res=assign(res,Tt(count),x1(&count));
	res=assign(res,Tt(clone),x1(&clone));
	res=assign(res,Tt(curtail),x1(&curtail));
	res=assign(res,Tt(info),x1(&info));
	res=assign(res,Tt(first),x1(first));
	res=assign(res,Tt(flat),x1(&flatten));
	res=assign(res,Tt(last),x1(&last));
	res=assign(res,Tt(list),x1(&list));
	res=assign(res,Tt(len),x1(&len));
	res=assign(res,Tt(key),x1(&key));
	res=assign(res,Tt(min),x1(&min));
	res=assign(res,Tt(max),x1(&max));
	res=assign(res,Tt(neg),x1(&neg));
	res=assign(res,Tt(not),x1(&not));
	res=assign(res,Tt(parse),x1(&parse));
	res=assign(res,Tt(repr),x1(&repr));
	res=assign(res,Tt(rev),x1(&reverse));
	#ifdef DEBUG
	res=assign(res,Tt(selftest),x1(&selftest));
	#endif
	res=assign(res,Tt(show),x1(&show));
	res=assign(res,Tt(str),x1(&str));
	res=assign(res,Tt(sum),x1(&sum));
	res=assign(res,Tt(sums),x1(&sums));
	res=assign(res,Tt(sys),x1(&sys));
	res=assign(res,Tt(tag),x1(&tag));
	res=assign(res,Tt(type),x1(&type));
	res=assign(res,Tt(val),x1(&val));
	res=assign(res,Tt(vec),x1(&list2vec));
	res=assign(res,Tt(ver),xi(0));
	res=assign(res,Tt(xray),x1(&xray));

	// infix/binary verbs
	// operators:
	// .. sequences and lists:
	res=assign(res,xt(_tagnums("|,")),x2(&join));
	res=assign(res,xt(_tagnums("||")),x2(&split));
	res=assign(res,xt(_tagnums("#|")),x2(&take));
	res=assign(res,xt(_tagnums("|#")),x2(&drop));
	res=assign(res,xt(_tagnums("|?")),x2(&except));
	// .. looping:
	res=assign(res,xt(_tagnums(".:")),x2(&call));
	res=assign(res,Tt(::),x2(&each));
	res=assign(res,xt(_tagnums(">:")),x2(&eachboth));
	res=assign(res,xt(_tagnums("\\:")),x2(&eachleft));
	res=assign(res,xt(_tagnums("/:")),x2(&eachright));
	res=assign(res,xt(_tagnums("<:")),x2(&eachpair));
	res=assign(res,xt(_tagnums("':")),x2(&over));
	res=assign(res,xt(_tagnums(",:")),x2(&scan));
	res=assign(res,xt(_tagnums("!:")),x2(&exhaust));
	// .. math:
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
	// .. misc:
	res=assign(res,Tt(!),x2(&amend));
	res=assign(res,Tt(@),x2(&apply));
	res=assign(res,Tt(:),x2(&dict)); 
	res=assign(res,Tt(?),x2(&find1));
	res=assign(res,Tt($),x2(&make));
	res=assign(res,Tt(#),x2(&pin));
	res=assign(res,Tt(~),x2(&matcheasy));
	res=assign(res,xt(_tagnums(",")),x2(&catenate)); 
	// named infix verbs:
	res=assign(res,Tt(amend),x2(&amend));
	res=assign(res,Tt(and),x2(&and));
	res=assign(res,Tt(aside),x2(&aside));
	res=assign(res,Tt(base),x2(&base));
	res=assign(res,Tt(bracketj),x2(&bracketjoin));
	res=assign(res,Tt(call),x2(&call));
	res=assign(res,Tt(case),x2(&casee));
	res=assign(res,Tt(consecj),x2(&consecutivejoin));
	res=assign(res,Tt(deal),x2(&deal));
	res=assign(res,Tt(deep),x2(&deep));
	res=assign(res,Tt(drop),x2(&drop));
	res=assign(res,Tt(each),x2(&each));
	res=assign(res,Tt(eachb),x2(&eachboth));
	res=assign(res,Tt(eachl),x2(&eachleft));
	res=assign(res,Tt(eachr),x2(&eachright));
	res=assign(res,Tt(evalin),x2(&evalin));
	res=assign(res,Tt(except),x2(&except));
	res=assign(res,Tt(exhaust),x2(&exhaust));
	res=assign(res,Tt(get),x2(&get));
	res=assign(res,Tt(iftrue),x2(&iftrue));
	res=assign(res,Tt(ifelse),x2(&ifelse));
	res=assign(res,Tt(in),x2(&matchany));
	res=assign(res,Tt(join),x2(&join));
	res=assign(res,Tt(loadin),x2(&loadin));
	res=assign(res,Tt(make),x2(&make));
	res=assign(res,Tt(nest),x2(&nest));
	res=assign(res,Tt(pick),x2(&pick));
	res=assign(res,Tt(pin),x2(&pin));
	res=assign(res,Tt(or),x2(&or));
	res=assign(res,Tt(orelse),x2(&orelse));
	res=assign(res,Tt(over),x2(&over));
	res=assign(res,Tt(range),x2(&range));
	res=assign(res,Tt(recurse),x2(&recurse));
	res=assign(res,Tt(rot),x2(&shift));
	res=assign(res,Tt(scan),x2(&scan));
	res=assign(res,Tt(split),x2(&split));
	res=assign(res,Tt(take),x2(&take));
	res=assign(res,Tt(wide),x2(&wide));

	//stdlib
	VP d;
	#ifdef STDLIBFILE
	// note: assigns 'cwd' in root too
	d=xd0();
	d=assign(d,Tt(basename),x1(&filebasename));
	res=assign(res,Tt(cwd),x1(&filecwd));
	d=assign(d,Tt(cwd),x1(&filecwd));
	d=assign(d,Tt(dirname),x1(&filedirname));
	d=assign(d,Tt(get),x1(&fileget));
	d=assign(d,Tt(path),x1(&filepath));
	d=assign(d,Tt(set),x2(&fileset));
	res=assign(res,Tt(File),d);
	xfree(d);
	#endif
	#ifdef STDLIBSHAREDLIB
	d=xd0();
	d=assign(d,Tt(get),x1(&sharedlibget));
	d=assign(d,Tt(set),x2(&sharedlibset));
	res=assign(res,Tt(Sharedlib),d);
	xfree(d);
	#endif
	#ifdef STDLIBSHELL
	d=xd0();
	d=assign(d,Tt(get),x1(&shellget));
	res=assign(res,Tt(Shell),d);
	xfree(d);
	#endif
	#ifdef STDLIBNET
	d=xd0();
	d=assign(d,Tt(bind),x2(&netbind));
	res=assign(res,Tt(Net),d);
	xfree(d);
	#endif

