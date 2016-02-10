#include "def.h"
#include "proto.h"
#include "accessors.h"
#include "vary.h"

void repl() {
	VP in,out,t1,t2,t3,ws=mkworkspace();
	char line[1024];
	int i;
	t1=xfroms("wkspc");

	in=xl0();
	assign(ELl(ws,1),Tt(inputs),in);
	out=xl0();
	assign(ELl(ws,1),Tt(outputs),out);

	i=0;
	for(;;) {
		// printf("xxl@%s> ", sfromx(get(ws,t1)));
		PF_LVL=2;
		printf("xxl %d>",i);
		fgets(line, sizeof(line), stdin);
		if(strncmp(line,"\n",1024)==0) continue;
		if(strncmp(line,"\\\\\n",1024)==0 ||
			 strncmp(line,"exit\n",1024)==0 ||
			 strncmp(line,"quit\n",1024)==0)
			exit(1);
		t2=parsestr(line);
		in=append(in,t2);
		// DUMP(t2);
		PF("APPENDING!!\n");
		append(ws,t2);
		t3=applyctx(ws,0);
		out=append(out,t3);
		PF("curtailing\n");
		ws=curtail(ws);
		DUMP(ws);
		printf("inputs@%d: %s\n", i, line);
		printf("outputs@%d: %s\n", i, sfromx(repr(t3)));
		i++;
	}
}
