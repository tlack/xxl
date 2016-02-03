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
//ASSERT(_equal(b,xi(2)),"test if 3");
xfree(c);xfree(b);
