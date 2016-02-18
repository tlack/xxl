// TODO transcripts ("test.txt" log)
// TODO .xxlrc
// TODO clear memory
// TODO turn off inputs/outputs
#include "def.h"
#include "proto.h"
#include "accessors.h"
#include "vary.h"

void banner() {
	printf("XXL %s by @tlack; http://github.com/tlack/xxl/\n",XXLVER);
}

void tip() {
	VP before=xfroms("tip: "),after=xfroms(" ('tip' for more)");
	VP tips=xl0();
	tips=append(tips,xfroms("you can trace your program's behavior. type 'xray' to try it out."));
	tips=append(tips,xfroms("type 'memwatch' to see how your program allocates memory."));
	tips=append(tips,xfroms("you can reference previous commands or program output like a regular variable. type 'inputs first' or 'outputs last' to see."));
	VP tip=deal(tips,XI1);
	show(flatten(catenate(catenate(before,tip),after)));
	xfree(tip);xfree(tips);xfree(after);xfree(before);
}

void showexc(VP exc) {
	int i; VP strs=xfroms("code\nmessage\nx\ny"), labels=split(strs,xc('\n'));
	show(exc);
	printf("Oops. Exception:\n");
	for(i=0;i<exc->n;i++) {
		printf("%s: ",sfromx(ELl(labels,i)));
		show(ELl(exc,i));
	}
	xfree(labels); xfree(strs);
}

void repl(VP ctx) {
	VP in,out,t1,t2,t3;
	char line[1024];
	int i;
	clock_t st,en;

	t1=xfroms("wkspc");
	in=xl0();
	assign(ELl(ctx,1),Tt(inputs),in);
	out=xl0();
	assign(ELl(ctx,1),Tt(outputs),out);
	#ifdef STDLIBFILE
	assign(ELl(ctx,1),Tt(_dir),filecwd(XI0));
	#endif

	i=0;
	banner();
	tip();

	for(;;) {
		// printf("xxl@%s> ", sfromx(get(ctx,t1)));
		//PF_LVL=2;
		printf("% 4d. ",i);
		fgets(line, sizeof(line), stdin);
		if(strncmp(line,"\n",1024)==0) continue;
		if(strncmp(line,"\\\\\n",1024)==0 ||
			 strncmp(line,"exit\n",1024)==0 ||
			 strncmp(line,"quit\n",1024)==0)
			exit(1);
		if(strncmp(line,"clear\n",1024)==0) {
			each(in,x1(&xfree)); in->n=0;
			each(out,x1(&xfree)); out->n=0;
			printf("\n\n----------------------------------------------\n\n"); // for text logs/scrollback
			printf("\033[2J\033[;H\033[0m");
			continue;
		}
		if(strncmp(line,"tip\n",1024)==0) {
			tip();
			continue;
		}
		if(strncmp(line,"memwatch\n",1024)==0) {
			if(MEM_WATCH) printf("memwatch off\n"),MEM_WATCH=0; 
			else printf("memwatch on\n"),MEM_WATCH=1;
			continue;
		}
		if(strncmp(line,"xray\n",1024)==0) {
			VP cmd;
			if(PF_LVL) cmd=xfroms("0 xray");
			else cmd=xfroms("1 xray");
			show(cmd);
			show(evalin(cmd,ctx));
			xfree(cmd);
			continue;
		}
		st=clock();
		t2=parsestr(line);
		in=append(in,t2);
		// DUMP(t2);
		// PF("APPENDING!!\n");
		append(ctx,t2);
		t3=applyctx(ctx,0,0);
		ctx=curtail(ctx);
		en=clock();
		printf("(%0.04f sec)\ninputs@%d: %s", ((double)(en-st)/CLOCKS_PER_SEC), i, line);
		if(t3==NULL) {
			out=append(out,xl0());
			printf("null\n");
		} else if(!IS_EXC(t3)) {
			out=append(out,t3);
			printf("outputs@%d:\n%s\n", i, sfromx(repr(t3)));
		} else {
			out=append(out,Tt(exception));
			showexc(t3);
		}
		i++;
	}
}
