VP a,b,c;
a=mkworkspace();
b=evalstrin("['a:1]$'table as 'z,['b:2]",a);
DUMP(b);
ASSERT(_len(b)==2 && _len(KEYS(b))==2,"table append dict");
xfree(b);
b=evalstrin("[1,2]as 'z!z,3 as 'b;z len=(b len)",a);
ASSERT(NUM_val(b)==0,"list not shadowed");
xfree(b);
b=evalstrin("['a:1]as 'z,['b,2]as 'b;z len=(b len)",a);
c=evalstrin("z len*(b len)",a);
DUMP(b);
DUMP(c);
ASSERT(NUM_val(b)==0 && NUM_val(c)==2,"dict not shadowed");
xfree(b);
xfree(c);
b=evalstrin("['a:1]$'table as 'z,['b:2]as 'b;z len=(b len)",a);
DUMP(key(a));
ASSERT(NUM_val(b)==0,"table not shadowed");
xfree(b);

b=evalstrin("['a:1]$'table,['a:5],['a:7]@'a over +",a);
ASSERT(NUM_val(b)==13,"table get sym");

b=evalstrin("[\"abc\":1]$'table,[\"abc\":5],[\"abc\":7]@\"abc\" over +",a);
ASSERT(NUM_val(b)==13,"table get string col");

/*
 * b=evalstrin("['a:1,'b:2] make 'table as 't; 128 count :: {t,['a:x,'b:(7*x)] as 't};t",a);
ASSERT(_len(b)==129,"table append a few");
c=evalstrin("t@'b sum",a);
ASSERT(NUM_val(c)==56898,"table count a few");
*/
b=evalstrin("'blah is 6; 10 {x as '.blah}; blah",a);
ASSERT(_equal(b,xi(10)), "assign global in anonymous lambda");
b=evalstrin("'blah is 6; {x as '.blah} as 'cb; 10 cb; blah",a);
ASSERT(_equal(b,xi(10)), "assign global in assigned lambda");
xfree(b);
b=evalstrin("'addermaker is {10+x as 'a; {x+a as '.a}}; 100 addermaker as 'myadd; 1 range 5 :: myadd",a);
ASSERT(_equal(b,xin(5,111,113,116,120,125)),"test adder");
xfree(b);
xfree(a);
a=mkworkspace();
b=evalstrin("'fun is {'addermaker is {10+x as 'a; {x+a as '.a}}; x addermaker as 'myadd; 1 range 5 :: { myadd }}; 100 fun",a);
ASSERT(_equal(b,xin(5,111,113,116,120,125)),"test adder nested");
xfree(a);

// TODO scope tests
// TODO self tests

