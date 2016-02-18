
	VP a,b,c,d; int i, tc;

	a=xalloc(T_i,1);
	EL(a,int,0)=1;
	ASSERT(EL(a,int,0)==1,"x0");
	ASSERT(AS_i(a,0)==1,"AS_i0");
	xfree(a);
	a=xi(99);
	b=xi(99);
	ASSERT(EL(a,int,0)==99,"x1");
	ASSERT(_equal(a,b)==1,"_equal0");
	ASSERT(_equal(b,a)==1,"_equal1");
	ASSERT(_equal(a,a)==1,"_equal2");
	ASSERT(_equal(b,b)==1,"_equal3");
	a=append(a,xi(98));
	ASSERT(EL(a,int,0)==99,"x2");
	ASSERT(EL(a,int,1)==98,"x3");
	ASSERT(_equal(a,b)==0,"_equal4");
	ASSERT(_equal(b,a)==0,"_equal5");
	c=xi(0);
	DUMP(apply(a,c));
	DUMP(b);
	ASSERT(_equal(apply(a,c),b)==1,"apply eq 0");
	xfree(b); 
	xfree(c);
	b=xi(98);
	ASSERT(_contains(a,b)==1,"_contains 0");
	c=xi(10);
	ASSERT(_contains(a,c)==0,"_contains 1");
	ASSERT(AS_i(contains(a,b),0)==1,"contains 0");
	ASSERT(AS_i(contains(a,c),0)==0,"contains 1");
	xfree(b);
	xfree(a);
	xfree(c);
	ASSERT(!_equal(xi(-1),xi(1)),"xi neg 0");
	a=xin(5,1,2,3,4,100);
	ASSERT(a->n==5,"_xin 0");
	ASSERT(EL(a,int,4)==100,"_xin 1");
	a=xian(a,3,200,300,400);
	ASSERT(EL(a,int,4)==100,"xian 2");
	ASSERT(EL(a,int,7)==400,"xian 3");
	ASSERT(EL(a,int,0)==1,"xian 4");
	ASSERT(a->n==8,"xian 5");
	#define NADD 1024
	for(i=0;i<NADD;i++) {
		a=xia(a,i*3);
	}
	ASSERT(a->n==8+NADD,"xia 1");
	ASSERT(EL(a,int,0)==1,"xia 1b");
	ASSERT(EL(a,int,1)==2,"xia 1c");
	ASSERT(EL(a,int,NADD+7)==(NADD-1)*3,"xia 2");
	xfree(a);
	a=xl0();
	b=xi(100);
	append(a,b);
	ASSERT(a->n=1,"append i 0");
	xfree(b);
	b = ELl(a,0);
	ASSERT(b->n==1,"append i 1");
	ASSERT(EL(b,int,0)==100,"append i 2");
	append(a,xfroms("test"));
	append(a,xfroms("test2"));
	DUMP(a);
	ASSERT(a->n==3,"append str 1");
	ASSERT(ELl(a,1)->n==4,"append str 2"); // test\0
	ASSERT(ELl(a,2)->n==5,"append str 2"); // test2\0
	ASSERT(_contains(a,ELl(a,2))==1,"str contains 0");
	ASSERT(_contains(a,xfroms("tes"))==0,"str contains 1");
	ASSERT(_equal(take(xin(3,1,2,3),xi(1)),xi(1)),"take 0");
	ASSERT(_equal(take(xin(3,1,2,3),xi(2)),xin(2,1,2)),"take 1");
	ASSERT(_equal(take(xin(3,1,2,3),xi(-2)),xin(2,2,3)),"take 2");
	ASSERT(_equal(take(xin(3,1,2,3),xi(-1)),xi(3)),"take 3");
	ASSERT(_equal(take(xin(3,1,2,3),xi(0)),xi0()),"take 4");
	ASSERT(_equal(take(xin(3,1,2,3),xi(4)),xin(4,1,2,3,1)),"take 5");
	ASSERT(_equal(take(xin(3,1,2,3),xi(3)),xin(3,1,2,3)),"take 6");
	ASSERT(_equal(take(xi(1),xi(3)),xin(3,1,1,1)),"take 7");
	ASSERT(_equal(take(xi(1),xi(-3)),xin(3,1,1,1)),"take 8");
	ASSERT(_equal(take(xi(-1),xi(-3)),xin(3,-1,-1,-1)),"take 9");
	ASSERT(_equal(take(xin(2,7,9),xi(-3)),xin(3,9,7,9)),"take 10");

	ASSERT(_equal(drop(xin(3,5,6,7),xi(1)),xin(2,6,7)),"drop 1");
	ASSERT(_equal(drop(xin(3,5,6,7),xi(-1)),xin(2,5,6)),"drop 2");
	ASSERT(_equal(drop(xin(3,5,6,7),xi(0)),xin(3,5,6,7)),"drop 3");

	upsert(a,xfroms("test"));
	ASSERT(a->n==3,"upsert 1");
	b = xi(101);
	upsert(a,b);
	xfree(b);
	DUMP(a);
	ASSERT(a->n==4,"upsert 2");
	ASSERT(_find1(a,xfroms("test"))==1,"_find1 0");
	ASSERT(_find1(a,xfroms("est"))==-1,"_find1 1");
	ASSERT(_find1(a,xfroms("tes"))==-1,"_find1 2");
	xfree(a);
	a = xd0();
	b = apply(a,xfroms("a"));
	DUMP(a);
	DUMP(b);
	ASSERT(b==NULL,"apply 0");
	b = xln(2,xi(1),xi(10)); // key:value dict
	append(a,b);
	xfree(b);
	d = xi(1);
	c = apply(a,d);
	ASSERT(_equal(c,xi(10))==1,"apply equal 0");
	xfree(a); xfree(c); xfree(d);

	a = xd0();
	b = xln(2,xln(1,xfroms("name")),xfroms("tom"));
	a=append(a,b);
	c=apply(a,xln(1,xfroms("name")));
	DUMP(c);
	ASSERT(IS_c(c) && memcmp(BUF(c),"tom",c->n)==0,"apply equal str");
	xfree(c);

	a = xd0();
	b = xln(2,xln(1,xfroms("over")),xfroms("tom"));
	a=append(a,b);
	xfree(b);
	b=xln(2,xln(1,xfroms("ver")),xfroms("frank"));
	a=append(a,b);
	c=apply(a,xln(1,xfroms("ver")));
	DUMP(c);
	ASSERT(IS_c(c) && memcmp(BUF(c),"frank",c->n)==0,"apply equal str 2");
	xfree(c);
	xfree(a);

	a=xd0();
	b=xln(2,xln(1,xfroms("over")),xfroms("tom"));
	a=append(a,b);
	xfree(b);
	b=xln(2,xln(1,xfroms("over")),xfroms("tom"));
	a=append(a,b);
	xfree(b);
	b=xln(2,xln(1,xfroms("ver")),xfroms("frank"));
	a=append(a,b);
	c=apply(a,xln(1,xfroms("ver")));
	DUMP(c);
	ASSERT(IS_c(c) && memcmp(BUF(c),"frank",c->n)==0,"apply equal str 3");
	xfree(c);
	xfree(a);

	a=xd0();
	b=xln(2,xln(1,xfroms("over")),xfroms("tom"));
	a=append(a,b);
	xfree(b);
	b=xln(2,xln(1,xfroms("over")),xfroms("tom"));
	a=append(a,b);
	xfree(b);
	b=xln(2,xln(1,xfroms("ver")),xfroms("frank"));
	a=append(a,b);
	c=apply(a,xln(1,xfroms("over")));
	DUMP(c);
	ASSERT(IS_c(c) && memcmp(BUF(c),"tom",c->n)==0,"apply equal str 3");
	xfree(c);
	xfree(a);

	a=xd0();
	b=xln(2,xln(1,xfroms("aa")),xfroms("tom"));
	a=append(a,b);
	PF("o0 0");
	DUMP(a);
	xfree(b);
	b=xln(2,xln(1,xfroms("aa")),xfroms("frank"));
	a=append(a,b);
	PF("o0 1");DUMP(a);
	xfree(b);
	c=apply(a,xfroms("aa"));
	PF("o0 2");DUMP(c);
	DUMP(c);
	ASSERT(IS_c(c) && memcmp(BUF(c),"frank",c->n)==0,"apply dictionary overlap 0");
	xfree(c);

	c=apply(a,xln(1,xfroms("")));
	ASSERT(c==NULL,"apply empty search");
	DUMP(c);
	xfree(c);
	xfree(a);

	_tagnums("");
	a=xin(3,10,9,8);
	a->tag=_tagnums("test tag");
	ASSERT(strcmp(sfromx(tagname(a->tag)),"test tag")==0,"tag name 0");
	xfree(a);

	a=xin(3,1,2,3); b=cast(a,xt(Ti(byte)));
	DUMP(b);
	ASSERT(_equal(a,b)&&b->n==3&&AS_b(b,2)==3,"cib");
	xfree(a);xfree(b);
	a=xin(3,1,2,3); b=cast(a,xt(Ti(octo)));
	DUMP(b);
	ASSERT(_equal(a,b)&&b->n==3&&AS_o(b,2)==3,"cio");
	xfree(a);
	xfree(b);
	a=xbn(2,9,8); b=cast(a,xt(Ti(byte)));
	DUMP(b);
	ASSERT(_equal(a,b)&&b->n==2&&AS_b(b,0)==9,"cbb");
	xfree(a);xfree(b);

	ASSERT(_equal(count(xb(3)),xbn(3,0,1,2)),"count b");
	ASSERT(_equal(count(xo(3)),xon(3,0,1,2)),"count o");

	c=and(xi(0),xi(1));
	DUMP(c);
	ASSERT(c->n==1 && _equal(c, xi(0)), "and 0");
	c=and(xi(1),xi(0));
	ASSERT(c->n==1 && _equal(c, xi(0)), "and 1");
	c=or(xi(0),xi(1));
	DUMP(c);
	ASSERT(c->n==1 && _equal(c, xi(1)), "and 0");
	c=or(xi(1),xi(0));
	ASSERT(c->n==1 && _equal(c, xi(1)), "and 1");

	c=min(xin(3,1,2,3));
	DUMP(c);
	ASSERT(c->n==1 && _equal(c, xi(1)), "min 0");
	c=min(xin(3,1,2,-3));
	ASSERT(c->n==1 && _equal(c, xi(-3)), "min 1");

	c=max(xin(3,1,2,3));
	DUMP(c);
	ASSERT(c->n==1 && _equal(c, xi(3)), "max 0");
	c=max(xin(3,1,2,-3));
	ASSERT(c->n==1 && _equal(c, xi(2)), "max 1");

	c=deal(xi(10),xi(100));
	DUMP(c);
	ASSERT(c->n==100 && _equal(min(c),xi(0)), "deal 0");

	ASSERT(_equal(condense(xbn(5,0,1,1,0,1)),xin(3,1,2,4)), "condense 0");
	ASSERT(_equal(shift(xbn(3,5,6,7),xb(1)),xbn(3,6,7,5)),"rot0");
	ASSERT(_equal(shift(xbn(3,5,6,7),xb(-1)),xbn(3,7,5,6)),"rot1");
	ASSERT(_equal(shift(xin(3,5,6,7),xi(555)),xin(3,5,6,7)),"rot2");
	ASSERT(_equal(shift(xin(3,5,6,7),xi(-555)),xin(3,5,6,7)),"rot3");
	ASSERT(_equal(reverse(xi(1)),xi(1)),"rev0");
	ASSERT(_equal(reverse(xin(3,9,7,6)),xin(3,6,7,9)),"rev1");

	ASSERT(_equal(amend(xin(4,6,0,0,6),xln(2,xin(2,1,2),xi(7))),xin(4,6,7,7,6)),"amend0");

	a=xin(3,2,2,7); // my favorite show
	b=clone(a);
	a=amend(a,xln(2,xi(0),xi(1)));
	ASSERT(_equal(a,a)&&!_equal(a,b),"clone0");
	xfree(a);xfree(b);

	c=xi(100);
	a=xln(3,xi(0),c,xi(0)); // my favorite show
	b=clone(a);
	c=amend(c,xln(2,xi(0),xi(5)));
	ASSERT(_equal(a,a)&&!_equal(a,b)&&_equal(ELl(b,1),xi(100)),"clone1");
	xfree(a);xfree(b);xfree(c);

	ASSERT(_equal(greater(xi(2),xi(1)),xb(1)),"greater0");
	ASSERT(_equal(greater(xi(1),xi(2)),xb(0)),"greater1");
	ASSERT(_equal(greater(xi(-2),xi(1)),xb(0)),"greater2");
	ASSERT(_equal(greater(xi(1),xi(-2)),xb(1)),"greater3");
	ASSERT(_equal(greater(xin(3,1,2,3),xi(2)),xbn(3,0,0,1)),"greater4");
	ASSERT(_equal(lesser(xi(2),xi(1)),xb(0)),"lesser0");
	ASSERT(_equal(lesser(xi(1),xi(2)),xb(1)),"lesser1");
	ASSERT(_equal(lesser(xi(-2),xi(1)),xb(1)),"lesser2");
	ASSERT(_equal(lesser(xi(1),xi(-2)),xb(0)),"lesser3");
	ASSERT(_equal(lesser(xin(3,1,2,3),xi(2)),xbn(3,1,0,0)),"lesser4");

	ASSERT(_equal(xf(3.0), plus(xf(1.5),xf(1.5))), "float 0");
	xfree(a);xfree(b);

	a=xi(3);
	ASSERT(_equal(a, plus(xf(1.5),xf(1.5))), "float int compare 0");
	xfree(a);xfree(b);
	
	a=xin(5,1,2,3,4,5);
	b=split(a,xi(3));
	DUMP(b);
	ASSERT(_equal(xln(2,xin(2,1,2),xin(2,4,5)),b),"split int token");

	a=xfroms("abXYcd");
	b=split(a,xfroms("XY"));
	ASSERT(b->n==2 && _equal(b,xln(2,xfroms("ab"),xfroms("cd"))),"split double char token");

	a=xfroms(".hello.there");
	b=split(a,xfroms("."));
	DUMP(b);
	ASSERT(_equal(xln(3,xc0(),xfroms("hello"),xfroms("there")),b),"split char token 0");
	a=xfroms(".hello..there.");
	b=split(a,xfroms("."));
	DUMP(b);
	ASSERT(_equal(xln(5,xc0(),xfroms("hello"),xc0(),xfroms("there"),xc0()),b),"split char token 1");
	xfree(a);xfree(b);

	a=xd0();a=assign(a,Tt(q),xi(10));a=assign(a,Tt(w),xi(11));b=key(a);
	ASSERT(_equal(b,xln(2,Tt(q),Tt(w))),"key0");
	ASSERT(_equal(key(xin(3,5,6,7)),xin(3,0,1,2)),"key1");

	a=xin(2,1,2); b=join(a,xi(3));
	ASSERT(_equal(b,xin(3,1,3,2)),"join 0");
	a=xi(1); b=join(a,xi(3));
	ASSERT(_equal(b,xi(1)),"join 1");
	a=xln(3,xfroms("a"),xfroms("b"),xfroms("c")); b=join(a,xfroms("/"));
	ASSERT(_equal(b,xfroms("a/b/c")),"join 2");
