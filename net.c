// NETWORKING

#include "def.h"
#include "proto.h"
#include "accessors.h"
#include "vary.h"

#ifdef STDLIBNET

int netr(int sock,void* b,size_t maxl) {
	return read(sock,b,maxl);
}
VP netw(int sock,VP buf) {
	XRAY_log("netw %d\n",sock);XRAY_emit(buf);
	if(buf==NULL) return NULL;
	if(!IS_b(buf)&&!IS_c(buf)) return EXC(Tt(type),"netw only strings",xi(sock),buf);
	if(write(sock,BUF(buf),buf->n)<buf->n) PERR("netw");
	return NULL;
}
#define NETLOOPBLK IOBLOCKSZ
VP netserve(VP sockcb) {
	int sock=AS_i(LIST_item(sockcb,0),0), nread;
	VP cb=LIST_item(sockcb,1);
	VP resp, t1, t2, t3;
	char input[NETLOOPBLK];
	/*
	if(remotea.sa_family==AF_INET) 
		ip = inet_ntoa(((struct sockaddr_in *)&remotea)->sin_addr);
	else
		ip = "n/a";
	*/
	XRAY_log("new connection %d\n",sock);
	memset(input, 0, NETLOOPBLK);
	nread=netr(sock, input, NETLOOPBLK-1);
	XRAY_log("read result %d\n", nread); 
	if(nread > 0) {
		t1=xfroms(input); t2=xfroms("n/a"); t3=xln(2,t1,t2);
		resp=apply(cb,t3);
		xfree(t3);xfree(t2);xfree(t1);
		XRAY_log("netloop handler resp for %d\n",sock); XRAY_emit(resp);
		if(resp!=NULL && !IS_c(resp)) {
			XRAY_log("massaging\n");XRAY_emit(resp);
			resp=repr(resp);
		}
		netw(sock,resp);
		xfree(resp);
	}
	shutdown(sock,SHUT_RDWR);
	XRAY_log("netloop closing %d\n",sock);
	close(sock);
	return NULL;
}
VP netloop(VP xsock,VP cb) {
	int cons;
	struct sockaddr remotea={0};
	socklen_t remotel={0};
	int sock=AS_i(xsock,0);
	int n=0;
	printf("netloop starting..\n");
	XRAY_emit(xsock);
	XRAY_emit(cb);
	for(;;) {
		printf(".");
		cons=accept(sock, &remotea, &remotel);
		netserve(xln(2,xi(cons),cb));
		// thr_run1(x1(&netserve),xln(2,xi(cons),cb));
		n++;
	}
	XRAY_log("netloop closing sock after %d\n",n);
	close(sock);
	return xl0();
}
VP netbind(VP opts,VP cb) {
	XRAY_log("netbind\n");XRAY_emit(opts);XRAY_emit(cb);
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
		strncpy(host,bfromx(ELl(opts,1)),64);
		if(inet_aton(host,&sin.sin_addr)==-1)return EXC(Tt(host),"hostname not found",opts,cb);
	}
	if(bind(sock,(struct sockaddr*)&sin,sizeof(sin))) {
		char* err=malloc(256); int len;
		snprintf(err,256,"couldn't bind to host/port: ");
		len=strlen(err);
		strerror_r(errno, err+len, 256-len);
		return EXC(Tt(bind),err,opts,cb);
	}
	listen(sock, 10); // XXX smarter listen backlog
	xref(cb);
	VP xsock=xi(sock);
	xref(xsock);
	thr_run(proj(2,&netloop,xsock,cb));
	printf("net booted\n");
	XRAY_emit(cb);
	return opts;
}
#endif

