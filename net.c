#include "def.h"
#include "proto.h"

// NETWORK
void net(void) {
	int sock, opt, port;
	char host[64]; 
	struct linger lf= {0};
	struct sockaddr_in sin= {0};
	struct sockaddr remote;
	socklen_t remotel;
	sock=socket(sin.sin_family=AF_INET,SOCK_STREAM,IPPROTO_TCP);
	opt=1;
	setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
	lf.l_onoff=1;
	lf.l_linger=0;
	setsockopt(sock,SOL_SOCKET,SO_LINGER,&lf,sizeof(lf));
	//opt=5; setsockopt(sock,SOL_TCP,TCP_DEFER_ACCEPT,&opt,sizeof(opt));
	port=8080; 
	strncpy(host,"localhost",64);
	if(inet_aton(host,&sin.sin_addr)==-1)perror("host name not found");
	sin.sin_port=htons(port);
	if(bind(sock,(struct sockaddr*)&sin,sizeof(sin)))perror("couldnt bind");
	listen(sock, 10); // XXX smarter listen backlog
	netloop(sock);
	printf("net booted\n");
}
size_t netr(int sock,void* b,size_t maxl) {
	return read(sock,b,maxl);
}
void netw(int sock,void* b,size_t l) {
	if(write(sock,b,l)<l) perror("netw");
}
void netloop(int sock) {
	int cons;
	struct sockaddr remotea;
	socklen_t remotel;
	char* ip;
	char greeting[]="Hello other side";
	char input[1024];
	for(;;) {
		cons=accept(sock, &remotea, &remotel);
		if(remotea.sa_family==AF_INET) 
			ip = inet_ntoa(((struct sockaddr_in *)&remotea)->sin_addr);
		else
			ip="n/a";
		printf("new connection from %s\n",ip);
		memset(input,0,sizeof(input));
		netr(cons,input,sizeof(input));
		printf("input = %s\n", input);
		netw(cons,greeting,strlen(greeting));
		close(cons);
	}
	close(sock);
}
