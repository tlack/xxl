
	// postfix/unary operators
	res=assign(res,xt(_tagnums("]")),x1(&identity));
	res=assign(res,Tt(condense),x1(&condense));
	res=assign(res,Tt(curtail),x1(&curtail));
	res=assign(res,Tt(info),x1(&info));
	res=assign(res,Tt(last),x1(&last));
	res=assign(res,Tt(len),x1(&len));
	res=assign(res,Tt(min),x1(&min));
	res=assign(res,Tt(max),x1(&max));
	res=assign(res,Tt(parse),x1(&parse));
	res=assign(res,Tt(repr),x1(&repr));
	res=assign(res,Tt(rev),x1(&reverse));
	res=assign(res,Tt(sum),x1(&sum));
	res=assign(res,Tt(til),x1(&til));
	res=assign(res,Tt(ver),xi(0));

	// infix/binary operators
	res=assign(res,xt(_tagnums(",")),x2(&join)); // gcc gets confused by Tt(,) - thinks its two empty args
	res=assign(res,Tt(:),x2(&dict)); // gcc gets confused by Tt(,) - thinks its two empty args
	res=assign(res,Tt(+),x2(&plus));
	res=assign(res,Tt(-),x2(&plus));
	res=assign(res,Tt(*),x2(&times));
	res=assign(res,Tt(/),x2(&times));
	res=assign(res,Tt(%),x2(&mod));
	res=assign(res,Tt(|),x2(&or));
	res=assign(res,Tt(&),x2(&and));
	res=assign(res,xt(_tagnums("<")),x2(&lesser));
	res=assign(res,xt(_tagnums(">")),x2(&greater));
	res=assign(res,xt(_tagnums("[")),x2(&list2));
	res=assign(res,Tt(~),x2(&matcheasy));
	res=assign(res,Tt(!),x2(&amend));
	res=assign(res,Tt(@),x2(&each));
	res=assign(res,Tt(bracketj),x2(&bracketjoin));
	res=assign(res,Tt(consecj),x2(&consecutivejoin));
	res=assign(res,Tt(deal),x2(&deal));
	res=assign(res,Tt(drop),x2(&drop));
	res=assign(res,Tt(each),x2(&each));
	res=assign(res,Tt(evalin),x2(&evalin));
	res=assign(res,Tt(in),x2(&matchany));
	res=assign(res,Tt(nest),x2(&nest));
	res=assign(res,Tt(pick),x2(&pick));
	res=assign(res,Tt(rot),x2(&shift));
	res=assign(res,Tt(take),x2(&take));
	res=assign(res,Tt(over),x2(&over));
