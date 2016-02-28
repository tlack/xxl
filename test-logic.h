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

c=mkworkspace();
b=evalstrin("777 as 'last;last",c);
d=evalstrin("777 as 'last;.last",c);
DUMP(b);
DUMP(d);
ASSERT(!_equal(b,d) && _equal(b,xi(777)) && _equal(d,x1(&last)), "names derived from root");
xfree(b);xfree(d);xfree(c);

/* 
this doesnt work yet - probably rightfully.. 
c=mkworkspace();
b=evalstrin("(1,2)as 'd;(3,4)as 'e;(d,e)",c);
ASSERT(_equal(b,xin(4,1,2,3,4)),"compound subexpr 0");
xfree(c);xfree(b);
*/

#ifdef STDLIBFILE
c=mkworkspace();
b=evalstrin(".file.get",c);
DUMP(b);
ASSERT(_equal(b,x1(&fileget)),"stdlib file reference");
xfree(c);xfree(b);
c=mkworkspace();
b=evalstrin(". get['file,\"def.h\"]",c);
DUMP(b);
ASSERT(IS_c(b)&&b->n>5000,"stdlib modular get def.h");
xfree(c);xfree(b);
c=mkworkspace();
b=evalstrin("[\"a\",\"b\",\".x\"] .file.path",c);
ASSERT(_equal(b,xfroms("a/b.x")),".file.path");
c=mkworkspace();
b=evalstrin("\"/a/b/c\" .file.basename",c);
ASSERT(_equal(b,xfroms("c")),".file.basename");
xfree(b);xfree(c);
c=mkworkspace();
b=evalstrin("\"/a/b/c\" .file.dirname",c);
ASSERT(_equal(b,xfroms("/a/b/")),".file.dirname");
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
ASSERT(_equal(b,xi(7)),"nested simple listexpr");

c=mkworkspace();
b=evalstrin("[1,2,3],[9,8,7] eachb +",c);
ASSERT(_equal(b,xin(3,10,10,10)),"eachboth");

c=mkworkspace();
b=evalstrin("[1,2,3],[9,8,7] >: +",c);
ASSERT(_equal(b,xin(3,10,10,10)),"eachboth short");

c=mkworkspace();
b=evalstrin("[1,2,3],[3] \\: +",c);
ASSERT(_equal(b,xin(3,4,5,6)),"eachleft short");

c=mkworkspace();
b=evalstrin("[3],[1,2,3] /: +",c);
ASSERT(_equal(b,xin(3,4,5,6)),"eachright short");

c=mkworkspace();
b=evalstrin("2,3,4 :: (*2)",c);
ASSERT(_equal(b,xin(3,4,6,8)),"each as ::");

c=mkworkspace();
b=evalstrin("\"hello\"!((1,2,5),\"x\")",c);
ASSERT(_equal(b,xfroms("hxxlox")),"amend many indices one value");

c=mkworkspace();
b=evalstrin("[]!(1,\"jordache\")",c);
ASSERT(_equal(repr(b),xfroms("[/*null*/, \"jordache\"]")),"amend empty list");

c=mkworkspace();
b=evalstrin("'z is 20; 30 as 'b;z*b",c);
DUMP(b);
ASSERT(_equal(b,xi(600)),"is vs as");

c=mkworkspace();
b=evalstrin("['a:1,'b:2] make 'table,[3,4],[5,6],[7,8],[[9,10],[11,12]]",c);
ASSERT(_equal(repr(b),xfroms("+['a, 'b]\n0:[1i, 2i],\n1:[3i, 4i],\n2:[5i, 6i],\n3:[7i, 8i],\n4:[9i, 10i],\n5:[11i, 12i]")),"table 0");

c=mkworkspace();
b=evalstrin("['a:1,'b:'j] make 'table,[3,'z],[5,'q],[7,'m],[[9,'b],[11,'m]]",c);
ASSERT(_equal(repr(b),xfroms("+['a, 'b]\n0:[1i, 'j],\n1:[3i, 'z],\n2:[5i, 'q],\n3:[7i, 'm],\n4:[9i, 'b],\n5:[11i, 'm]")),"table 1");

c=mkworkspace();
b=evalstrin("['a:1,'b:'j] make 'table,[3,'z],[5,'q],[7,'m],[[9,'b],[11,'m]]@0",c);
ASSERT(_equal(repr(b),xfroms("['a:1i, 'b:'j]")),"table scalar subscript");

c=mkworkspace();
b=evalstrin("['a:1,'b:'j] make 'table,[3,'z],[5,'q],[7,'m],[[9,'b],[11,'m]]@(1,2)",c);
ASSERT(_equal(repr(b),xfroms("+['a, 'b]\n0:[3i, 'z],\n1:[5i, 'q]")),"table vector subscript");

c=mkworkspace();
b=evalstrin("('a,'b,'c):[]",c);
ASSERT(IS_EXC(b), "unlike vectors check when making table");

c=mkworkspace();
b=evalstrin("('a,'b,'c):[ ['a,2,4], ['b,4,6] ]",c);
ASSERT(_equal(repr(b),xfroms("+('a,'b,'c)\n0:['a, 2i, 4i],\n1:['b, 4i, 6i]")),"table built with list of lists");

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
