//
// Demonstration of using shared libraries in XXL
// Use like this: 
// xxl 0>"./tests/sharedlib-test.so" .sharedlib.get as 'mylib
// (0.0310 sec)
// inputs@0: "./tests/sharedlib-test.so" .sharedlib.get as 'mylib
// outputs@0: ['test:'1(...)]
// xxl 1>mylib@'test@666
// testfunc
// '2(...)
// 100i
// (0.0000 sec)
// inputs@1: mylib@'test@666
// outputs@1: 666i
//
#define STDLIBSHAREDLIB 1

#include "../def.h"
#include "../proto.h"
#include "../accessors.h"

VP testfunc(VP x) {
	printf("testfunc\n");
	PF_LVL=10;
	printf("%s\n", reprA(x));
	VP res=xln(3,xi(111),x,xi(444));
	DUMP(res);
	PF_LVL=0;
	printf("%s\n", reprA(res));
	return res;
}

struct xxl_index_t XXL_INDEX[] = {
	{ "test", "testfunc", 1 },
	{ "",     "",         0 }
};

