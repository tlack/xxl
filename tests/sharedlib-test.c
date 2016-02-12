#include "def.h"
#include "proto.h"
#include "accessors.h"

VP testfunc(VP x) {
	return xln(3,333,x,444);
}

static struct xxl_index_t XXL_INDEX[] = {
	{ "test", "testfunc", 1 },
	{ "",     "",         0 }
};

