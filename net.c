// NETWORKING

#include "def.h"
#include "proto.h"
#include "accessors.h"
#include "vary.h"

#ifdef STDLIBNET

size_t netr(int sock,void* b,size_t maxl) {
	return read(sock,b,maxl);
}
VP netw(int sock,VP buf) {
	PF("netw %d\n",sock);DUMP(buf);
	if(!IS_b(buf)&&!IS_c(buf)) return EXC(Tt(type),"netw only strings",xi(sock),buf);
	if(write(sock,BUF(buf),buf->n)<buf->n) PERR("netw");
	return 0;
}
VP netloop(VP xsock,VP cb) {
	#define NETLOOPBLK 1024*65
	int cons;
	struct sockaddr remotea={0};
	socklen_t remotel={0};
	char* ip;
	char* rep;
	char input[NETLOOPBLK];
	int sock=AS_i(xsock,0);
	VP t1,t2,t3;
	VP resp;
	int n=0;
	PF_LVL=3;
	printf("netloop starting..\n");
	DUMP(xsock);
	DUMP(cb);
	for(;;) {
		printf(".");
		cons=accept(sock, &remotea, &remotel);
		/*
		if(remotea.sa_family==AF_INET) 
			ip = inet_ntoa(((struct sockaddr_in *)&remotea)->sin_addr);
		else
			ip = "n/a";
		*/
		PF("new connection %d, #%d\n",cons, n);
		memset(input,0,NETLOOPBLK);
		netr(cons,input,NETLOOPBLK-1);
		t1=xfroms(input);t2=xfroms("n/a");t3=xln(2,t1,t2);
		resp=apply(cb,t3);
		xfree(t3);xfree(t2);xfree(t1);
		PF("netloop handler resp for %d\n",n); DUMP(resp);
		if(!IS_c(resp)) {
			PF("massaging\n");DUMP(resp);
			resp=repr(resp);
		}
		netw(cons,resp);
		shutdown(cons,SHUT_RDWR);
		xfree(resp);
		PF("netloop closing %d, #%d\n",cons,n);
		close(cons);
		n++;
	}
	PF("netloop closing sock after %d\n",n);
	close(sock);
	return xl0();
}
VP netbind(VP opts,VP cb) {
	if(!LIST(opts)) return EXC(Tt(type),"bad network bind options",opts,cb);
	int sock, opt, port;
	char host[64]; 
	struct linger lf= {0};
	struct sockaddr_in sin= {0};
	struct sockaddr remote;
	socklen_t remotel;
	sock=socket(sin.sin_family=AF_INET,SOCK_STREAM,IPPROTO_TCP);
	opt=1;
	setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
	/*
	lf.l_onoff=1;
	lf.l_linger=0;
	setsockopt(sock,SOL_SOCKET,SO_LINGER,&lf,sizeof(lf));
	*/
	//opt=5; setsockopt(sock,SOL_TCP,TCP_DEFER_ACCEPT,&opt,sizeof(opt));
	if(!IS_i(ELl(opts,0))) return EXC(Tt(port),"bad port",opts,cb);
	port=AS_i(ELl(opts,0),0);
	sin.sin_port=htons(port);
	if(ELl(opts,1)->n >= 1 && IS_c(ELl(opts,1))) {
		strncpy(host,sfromx(ELl(opts,1)),64);
		if(inet_aton(host,&sin.sin_addr)==-1)return EXC(Tt(host),"hostname not found",opts,cb);
	}
	if(bind(sock,(struct sockaddr*)&sin,sizeof(sin)))
		return EXC(Tt(bind),"couldnt bind host/port",opts,cb);
	listen(sock, 10); // XXX smarter listen backlog
	xref(cb);
	VP xsock=xi(sock);
	xref(xsock);
	thr_run(proj(2,&netloop,xsock,cb));
	printf("net booted\n");
	DUMP(cb);
	return opts;
}

#endif

