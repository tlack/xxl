// TODO transcripts ("test.txt" log)
// TODO .xxlrc
// TODO clear memory
// TODO turn off inputs/outputs
#include "def.h"
#include "proto.h"
#include "accessors.h"
#include "vary.h"

void banner() {
	printf("XXL %s by @tlack; http://github.com/tlack/xxl/\n",XXL_VER);
}

VP randtip() {
	VP tips=xl0();
	tips=append(tips,xfroms("you can trace your program's behavior. type 'xray' to try it out."));
	tips=append(tips,xfroms("type 'memwatch' to see how your program allocates memory."));
	tips=append(tips,xfroms("you can reference previous commands or program output like a regular variable. type 'inputs first' or 'outputs last' to see."));
	tips=append(tips,xfroms("is your XXL acting wonky? run the self tests with '[] selftest'. you can find their source in 'test-*.h'."));
	VP tip=deal(tips,XI1);
	xfree(tips);
	return tip;
}

void tip() {
	VP before=xfroms("tip: "),after=xfroms(" ('tip' for more)");
	VP tip=randtip();
	VP str=flatten(catenate(catenate(before,tip),after));
	show(str);
	xfree(str);xfree(tip);xfree(after);xfree(before);
}

void showexc(VP ctx,VP exc) {
	int i; VP clue,csel,strs=xfroms("code\nmessage\nx\ny"), labels=split(strs,xc('\n'));
	show(exc);
	printf("Oops. Exception:\n");
	/*
	csel=xln(2,Tt(repl),Tt(clues)); clue=get(ctx,csel);
	if(!IS_EXC(clue)) {
		DUMP(clue);
		VP specific = apply(clue,ELl(exc,0));
		printf("spec=%s\n", reprA(specific));
		VP tmp = apply(specific,xln(2,Tt(caughtexec),exc));
		printf("tmp=%s\n", reprA(tmp));
		xfree(tmp); xfree(specific);
	}
	xfree(clue);xfree(csel);
	*/
	for(i=0;i<exc->n;i++) {
		char* s=sfromxA(ELl(labels,i));
		printf("%s: ",s);
		show(ELl(exc,i));
		free(s);
	}
	xfree(labels); xfree(strs);
}

void repl(VP ctx) {
	VP in,out,t1,t2,t3;
	#define LINESZ 1024
	char line[LINESZ];
	char* ret;
	int i;
	clock_t st,en;

	in=xl0();
	set(ctx,Tt(inputs),in);
	out=xl0();
	set(ctx,Tt(outputs),out);
	#ifdef STDLIBFILE
	set(ctx,Tt(_dir),filecwd(XI0));
	#endif

	i=0;
	banner();
	tip();
	printf("\n");

	for(;;) {
		// printf("xxl@%s> ", bfromx(get(ctx,t1)));
		//XRAY_LVL=2;
		printf("%d. ",i);
		memset(line, 0, LINESZ);
		ret = fgets(line, LINESZ, stdin);
		if(ret == NULL) 
			break;
		if(line[0] == 0 || strncmp(line,"\n",LINESZ)==0) continue;
		if(strncmp(line,"\\\\\n",LINESZ)==0 ||
			 strncmp(line,"exit\n",LINESZ)==0 ||
			 strncmp(line,"quit\n",LINESZ)==0)
			break;
		if(strncmp(line,"clear\n",LINESZ)==0) {
			i=0;
			each(in,x1(&xfree)); in->n=0;
			each(out,x1(&xfree)); out->n=0;
			printf("\n\n----------------------------------------------\n\n"); // for text logs/scrollback
			printf("\033[2J\033[;H\033[0m");
			continue;
		}
		if(strncmp(line,"tip\n",LINESZ)==0) {
			tip();
			continue;
		}
		if(strncmp(line,"memwatch\n",LINESZ)==0) {
			if(MEM_WATCH) printf("memwatch off\n"),MEM_WATCH=0; 
			else printf("memwatch on\n"),MEM_WATCH=1;
			continue;
		}
		if(strncmp(line,"xray\n",LINESZ)==0) {
			VP cmd;
			if(XRAY_LVL) cmd=xfroms("0 xray");
			else cmd=xfroms("1 xray");
			show(evalin(cmd,ctx));
			xfree(cmd);
			continue;
		}
		st=clock();
		t2=parsestr(line);
		in=append(in,t2);
		ctx=append(ctx,t2); // set code body for this context - doesnt actually append
		xfree(t2);
		t3=applyctx(ctx,NULL,NULL);	
		en=clock();
		printf("(%0.04f sec)\ninputs@%d: %s", ((double)(en-st)/CLOCKS_PER_SEC), i, line);
		if(t3==NULL) {
			out=append(out,xl0());
			printf("null\n");
		} else if(!IS_EXC(t3)) {
			out=append(out,t3);
			printf("outputs@%d:\n%s\n", i, bfromx(repr(t3)));
		} else {
			out=append(out,Tt(exception));
			showexc(ctx,t3);
		}
		xfree(t3);
		printf("\n"); // the horror!
		i++;
	}
	xfree(in);
	xfree(out);
	return;
}
