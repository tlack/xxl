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
		printf("%s: ",bfromx(ELl(labels,i)));
		show(ELl(exc,i));
	}
	xfree(labels); xfree(strs);
}

void repl(VP ctx) {
	VP in,out,t1,t2,t3;
	char line[1024];
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

	for(;;) {
		// printf("xxl@%s> ", bfromx(get(ctx,t1)));
		//PF_LVL=2;
		printf("%d. ",i);
		fgets(line, sizeof(line), stdin);
		if(strncmp(line,"\n",1024)==0) continue;
		if(strncmp(line,"\\\\\n",1024)==0 ||
			 strncmp(line,"exit\n",1024)==0 ||
			 strncmp(line,"quit\n",1024)==0)
			exit(1);
		if(strncmp(line,"clear\n",1024)==0) {
			i=0;
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
			show(evalin(cmd,ctx));
			xfree(cmd);
			continue;
		}
		st=clock();
		t2=parsestr(line);
		in=append(in,t2);
		VP subctx=CTX_make_subctx(ctx, t2);
		t3=applyctx(subctx,0,0);	
		// PF_LVL++;
		// DUMP(subctx);
		KEYS(subctx)=del(KEYS(subctx),TTPARENT);
		// DUMP(subctx);
		// PF_LVL--;
		KEYS(ctx)=unionn(KEYS(ctx),KEYS(subctx));
		xfree(subctx);
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
		printf("\n"); // the horror!
		i++;
	}
}
