VP d;

c=mkworkspace();
b=evalstrin("(1,2,3,4)@2",c);
ASSERT(_equal(b,xi(3)),"test apply num 0");

c=mkworkspace();
b=evalstrin("1 iftrue 3",c);
ASSERT(_equal(b,xi(3)),"test if 0");
xfree(c);xfree(b);

c=mkworkspace();
b=evalstrin("1 iftrue 3",c);
ASSERT(_equal(b,xi(3)),"test if 0");
xfree(c);xfree(b);

c=mkworkspace();
b=evalstrin("1 iftrue {3}",c);
ASSERT(_equal(b,xi(3)),"test if 0 f");
xfree(c);xfree(b);

c=mkworkspace();
b=evalstrin("1 ifelse (3,2)",c);
ASSERT(_equal(b,xi(3)),"test if 1");
xfree(c);xfree(b);

c=mkworkspace();
b=evalstrin("0 ifelse (3,2)",c);
ASSERT(_equal(b,xi(2)),"test if 2");
xfree(c);xfree(b);

c=mkworkspace();
b=evalstrin("1 ifelse ({x*2},{x*3})",c);
ASSERT(_equal(b,xi(2)),"test if 3");
xfree(c);xfree(b);

c=mkworkspace();
b=evalstrin("[[4,5],[6,7]]@[0,1]",c);
ASSERT(_equal(b,xi(5)),"test apply at depth 0");
xfree(c);xfree(b);

c=mkworkspace();
b=evalstrin("['a:(1,2,3),'b:(4,5,6)]@['b,2]",c);
ASSERT(_equal(b,xi(6)),"test apply at depth in dict");
xfree(c);xfree(b);

/* 
this doesnt work yet - probably rightfully.. 
c=mkworkspace();
b=evalstrin("(1,2)as 'd;(3,4)as 'e;(d,e)",c);
ASSERT(_equal(b,xin(4,1,2,3,4)),"compound subexpr 0");
xfree(c);xfree(b);
*/

#ifdef STDLIBFILE
c=mkworkspace();
b=evalstrin("File.get",c);
DUMP(b);
ASSERT(_equal(b,x1(&fileget)),"stdlib file reference");
xfree(c);xfree(b);
c=mkworkspace();
b=evalstrin(". get ('File#\"def.h\")",c);
DUMP(b);
ASSERT(IS_c(b)&&b->n>5000,"stdlib modular get def.h");
xfree(c);xfree(b);
c=mkworkspace();
b=evalstrin("[\"a\",\"b\",\".x\"] File.path",c);
ASSERT(_equal(b,xfroms("a/b.x")),"File.path");
c=mkworkspace();
b=evalstrin("\"/a/b/c\" File.basename",c);
ASSERT(_equal(b,xfroms("c")),"File.basename");
xfree(b);xfree(c);
c=mkworkspace();
b=evalstrin("\"/a/b/c\" File.dirname",c);
ASSERT(_equal(b,xfroms("/a/b/")),"File.dirname");
xfree(b);xfree(c);
#endif

c=mkworkspace();
b=evalstrin("543 as 'zebra; . get 'zebra",c);
ASSERT(IS_i(b) && AS_i(b,0) == 543,"test . get");
xfree(c);xfree(b);

c=mkworkspace();
b=evalstrin("['abc:100,'xyz:999]each{-1}",c);
a=evalstrin("['abc:99,'xyz:998]",c);
ASSERT(_equal(repr(b),repr(a)),"dict each");

c=mkworkspace();
b=evalstrin("7 as 'p;[[p]]",c);
ASSERT(_equal(b,xl(xl(xi(7)))),"nested simple listexpr");

c=mkworkspace();
b=evalstrin("[(1,2,3),(9,8,7)] eachb +",c);
ASSERT(_equal(b,xin(3,10,10,10)),"eachboth");

c=mkworkspace();
b=evalstrin("[(1,2,3),(9,8,7)] >: +",c);
ASSERT(_equal(b,xin(3,10,10,10)),"eachboth short");

c=mkworkspace();
b=evalstrin("[(1,2,3),3] \\: +",c);
ASSERT(_equal(b,xin(3,4,5,6)),"eachleft short");

c=mkworkspace();
b=evalstrin("[3,(1,2,3)] /: +",c);
ASSERT(_equal(b,xin(3,4,5,6)),"eachright short");

c=mkworkspace();
b=evalstrin("2,3,4 :: (*2)",c);
ASSERT(_equal(b,xin(3,4,6,8)),"each as ::");

c=mkworkspace();
b=evalstrin("\"hello\"!((1,2,5),\"x\")",c);
ASSERT(_equal(b,xfroms("hxxlox")),"amend many indices one value");

c=mkworkspace();
b=evalstrin("[]!(1,\"jordache\")",c);
ASSERT(_equal(repr(b),xfroms("[null, \"jordache\"]")),"amend empty list");
xfree(b); xfree(c);

c=mkworkspace();
b=evalstrin("\"abc\"~\"\"![0,1]",c);
ASSERT(_equal(b,xbn(3,1,0,0)),"amend byte vec with int");
xfree(b); xfree(c);

c=mkworkspace();
b=evalstrin("'z is 20; 30 as 'b;z*b",c);
DUMP(b);
ASSERT(_equal(b,xi(600)),"is vs as");

c=mkworkspace();
b=evalstrin("['a:1,'b:2] make 'table,[3,4],[5,6],[7,8],[[9,10],[11,12]] as 't; t@'a vec + (t@'b vec) = (3,7,11,15,19,23)",c);
ASSERT(_equal(b,XI1), "table 0");

c=mkworkspace();
b=evalstrin("['a:1,'b:'j] make 'table,[3,'z],[5,'q],[7,'m],[[9,'b],[11,'m]] as 't; [(t@'b)],(t@'a) >: {x str,(y str)} join \"\"",c);
ASSERT(_equal(b,xfroms("j1z3q5m7b9m11")), "table 1");

c=mkworkspace();
b=evalstrin("['a:1,'b:'j] make 'table,[3,'z],[5,'q],[7,'m],[[9,'b],[11,'m]]@0",c);
ASSERT(_equal(repr(b),xfroms("['a:1i, 'b:'j]")),"table scalar subscript");

c=mkworkspace();
b=evalstrin("['a:1,'b:'j] make 'table,[3,'z],[5,'q],[7,'m],[[9,'b],[11,'m]]@(1,2)@'a vec sum",c);
ASSERT(_equal(b,xi(8)),"table vector subscript");

c=mkworkspace();
b=evalstrin("('a,'b,'c):[]",c);
ASSERT(IS_EXC(b), "unlike vectors check when making table");

c=mkworkspace();
b=evalstrin("('a,'b,'c):[['a,2,4], ['b,4,6]]as 't len*(t@'c vec)sum",c);
ASSERT(_equal(b,xi(20)),"table built with a list of lists");

c=mkworkspace();
b=apply_simple_(evalstrin("1,2,3",c),0);
ASSERT(_equal(b,xi(1)),"apply_simple_ 0");
xfree(b);xfree(c);

c=mkworkspace();
b=evalstrin("[\"B1\"]",c);
ASSERT(LIST(b) && IS_c(LIST_first(b)), "listexpr with non-scalar simple content");

c=mkworkspace();
b=evalstrin("('a,'b,'c):[['a,2,4],['b,4,6]]~{x}",c);
ASSERT(_equal(repr(b),xfroms("[['a:'a, 'b:2i, 'c:4i], ['a:'b, 'b:4i, 'c:6i]]")),"matcheasy with table - identity");

c=mkworkspace();
b=evalstrin("('a,'b,'c):[ ['a,2,4], ['b,4,6] ]first",c);
ASSERT(_equal(repr(b),xfroms("['a:'a, 'b:2i, 'c:4i]")),"table first");

c=mkworkspace();
b=evalstrin("1,2,3 recurse {rot1}",c);
ASSERT(_equal(repr(b),xfroms("[(2,3,1i), (3,1,2i)]")),"recurse0");

c=mkworkspace();
b=evalstrin("1,2,3 exhaust {rot1}",c);
ASSERT(_equal(repr(b),xfroms("(3,1,2i)")),"exhaust0");

c=mkworkspace();
b=evalstrin("1,2,3 except 4",c);
ASSERT(_equal(repr(b),xfroms("(1,2,3i)")),"except0");

c=mkworkspace();
b=evalstrin("1,2,3 except (3)",c);
ASSERT(_equal(repr(b),xfroms("(1,2i)")),"except1");

c=mkworkspace();
b=evalstrin("1,2,3 except (3,1)",c);
ASSERT(_equal(repr(b),xfroms("2i")),"except2");

c=mkworkspace();
b=evalstrin("[1,2,3]::{\"{\"}flat",c);
DUMP(repr(b));
ASSERT(_equal(repr(b),xfroms("\"{{{\"")),"bracequote");

c=mkworkspace();
b=evalstrin("[[1,2,3],[4,5,6]]join\":\"",c);
ASSERT(_equal(repr(b),xfroms("[[1i, 2i, 3i], \":\", [4i, 5i, 6i]]")),"join list flat");

c=mkworkspace();
b=evalstrin("\"a\"range\"d\"",c);
ASSERT(_equal(repr(b),repr(xfroms("abcd"))),"range with chars");
xfree(c);

c=mkworkspace();
b=evalstrin("65535 base 16",c);
ASSERT(_equal(repr(b),repr(xfroms("ffff"))),"base 16 0");
b=evalstrin("45325235 base 16",c); //arb
ASSERT(_equal(repr(b),repr(xfroms("2b39bb3"))),"base 16 1");
b=evalstrin("45325235 as 'z base 16 base 16 = z",c); //arb
ASSERT(_equal(b,xi(1)),"base 16 2");
xfree(c);

c=mkworkspace();
b=evalstrin("[1,2,3]del0",c);
a=evalstrin("[2,3]",c);
ASSERT(_equal(repr(b),repr(a)),"del list 0");

c=mkworkspace();
b=evalstrin("[1,2,3]del2",c);
a=evalstrin("[1,2]",c);
ASSERT(_equal(repr(b),repr(a)),"del list 1");

c=mkworkspace();
b=evalstrin("[1,2,3]del4",c);
a=evalstrin("[1,2,3]",c);
ASSERT(_equal(repr(b),repr(a)),"del list 2");

c=mkworkspace();
b=evalstrin("['a:1,'b:2,'c:3]del 'b",c);
a=evalstrin("['a:1,'c:3]",c);
ASSERT(_equal(repr(b),repr(a)),"del dict 0");

c=mkworkspace();
b=evalstrin("['a:1,'b:2,'c:3]del 'z",c);
a=evalstrin("['a:1,'b:2,'c:3]",c);
ASSERT(_equal(repr(b),repr(a)),"del dict 1");
xfree(c);xfree(b);xfree(a);

c=mkworkspace();
b=evalstrin("['a:1;'b:2]as 'z;4 as 'z.b;z",c);
a=evalstrin("['a:1;'b:4]",c);
ASSERT(_equal(repr(b),repr(a)),"set at depth 0");
b=evalstrin("'inner is ['bb:['c:10]]; 'outer is ['b:['ba:5,'bb:inner]]; ['z:10,outer] as 't; t.b.bb.c",c);
ASSERT(_equal(b,xi(10)),"index at very deep 0");

b=evalstrin("(1,2)from(4,5,6)",c);
ASSERT(_equal(b,xin(2,5,6)),"from 0");


