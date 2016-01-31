#include "def.h"
#include "proto.h"

void repl() {
	VP t1,t2,t3,ws=mkworkspace();
	char line[1024];
	int i;
	PFW({
	t1=xfroms("wkspc");

	for(;;) {
		// printf("xxl@%s> ", sfromx(get(ws,t1)));
		printf("xxl>");
		fgets(line, sizeof(line), stdin);
		if(strncmp(line,"\\\\\n",1024)==0 ||
			 strncmp(line,"exit\n",1024)==0 ||
			 strncmp(line,"quit\n",1024)==0)
			exit(1);
		t2=parsestr(line);
		// DUMP(t2);
		append(ws,t2);
		t3=applyctx(ws,xi0());
		printf("%s\n", line);
		printf("%s\n", sfromx(repr(t3)));
	}
	});
}
