#include "def.h"
#include "proto.h"
#include "accessors.h"
#include "vary.h"

void repl() {
	VP t1,t2,t3,ws=mkworkspace();
	char line[1024];
	int i;
	t1=xfroms("wkspc");

	for(;;) {
		// printf("xxl@%s> ", sfromx(get(ws,t1)));
		printf("xxl>");
		fgets(line, sizeof(line), stdin);
		if(strncmp(line,"\n",1024)==0) continue;
		if(strncmp(line,"\\\\\n",1024)==0 ||
			 strncmp(line,"exit\n",1024)==0 ||
			 strncmp(line,"quit\n",1024)==0)
			exit(1);
		PFW({
		t2=parsestr(line);
		// DUMP(t2);
		PF("APPENDING!!\n");
		append(ws,t2);
		t3=applyctx(ws,xi0());
		});
		PF("curtailing\n");
		ws=curtail(ws);
		DUMP(ws);
		printf("%s\n", line);
		printf("%s\n", sfromx(repr(t3)));
	}
}
