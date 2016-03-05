VP a,b,c;
a=mkworkspace();
b=evalstrin("['a:1]$'table as 'z,['b:2]",a);
DUMP(b);
ASSERT(_len(b)==2 && _len(KEYS(b))==2,"table append dict");
xfree(b);
b=evalstrin("[1,2]as 'z!z,3 as 'b;z len=(b len)",a);
ASSERT(NUM_val(b)==0,"list not shadowed");
xfree(b);
PFW(({
b=evalstrin("['a:1]as 'z,['b,2]as 'b;z len=(b len)",a);
c=evalstrin("z len*(b len)",a);
DUMP(b);
DUMP(c);
ASSERT(NUM_val(b)==0 && NUM_val(c)==2,"dict not shadowed");
xfree(b);
xfree(c);
}));
b=evalstrin("['a:1]$'table as 'z,['b:2]as 'b;z len=(b len)",a);
DUMP(key(a));
ASSERT(NUM_val(b)==0,"table not shadowed");
xfree(b);
/*
 * b=evalstrin("['a:1,'b:2] make 'table as 't; 128 count :: {t,['a:x,'b:(7*x)] as 't};t",a);
ASSERT(_len(b)==129,"table append a few");
c=evalstrin("t@'b sum",a);
ASSERT(NUM_val(c)==56898,"table count a few");
*/
xfree(a);

// TODO scope tests
// TODO self tests

