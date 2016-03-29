// STANDARD LIBRARY

#include "def.h"
#include "proto.h"
#include "accessors.h"
#include "vary.h"

#ifdef STDLIBSHAREDLIB
#include <dlfcn.h>
#endif

#ifdef STDLIBFILE
VP filebasename(VP fn) {
	VP fname=fn;
	if(!IS_c(fname)) fname=filepath(fn);
	if(!IS_c(fname)) return EXC(Tt(type),"basename filename must be string or pathlist",fn,0);
	char* str=sfromxA(fname);
	VP res=xfroms(basename(str));
	xfree(fname); free(str);
	return res;
}
VP filedirname(VP fn) {
	VP fname=fn;
	if(!IS_c(fname)) fname=filepath(fn);
	if(!IS_c(fname)) return EXC(Tt(type),"dirname filename must be string or pathlist",fn,0);
	char* str=sfromxA(fname);
	VP res=xfroms(dirname(str)), slash=xc('/');
	res=append(res,slash);
	free(str);
	return res;
}
VP filecwd(VP dummy) {
	char cwd[1024];
	if(getcwd(cwd,sizeof(cwd))!=NULL) return xfroms(cwd);
	else return EXC(Tt(open),"couldnt get current working directory",0,0);	
}
VP fileget(VP fn) {
	int r, fd; char buf[IOBLOCKSZ]; VP fname=fn;
	if(!IS_c(fname)) fname=filepath(fn);
	if(!IS_c(fname)) return EXC(Tt(type),"readfile filename must be string or pathlist",fn,0);
	//XRAY_LVL++;XRAY_log("fileget\n");XRAY_emit(fn);XRAY_LVL--;
	if(LEN(fname)==1 && AS_c(fname,0)=='-') fd=STDIN_FILENO; 
	else {
		char* str=sfromxA(fname);
		fd=open(str,O_RDONLY);
		free(str);
	}
	if(fd<0) return EXC(Tt(open),"could not open file for reading",fname,0);
	VP acc=xcsz(IOBLOCKSZ);
	do {
		r=read(fd,buf,IOBLOCKSZ);
		if(r<=0) { xfree(acc); close(fd); return EXC(Tt(read),"could not read from file",fname,0); }
		else appendbuf(acc,(buf_t)&buf,r);
	} while (r==IOBLOCKSZ);
	close(fd);
	return acc;
}
VP filepath(VP pathlist) {
	if(IS_c(pathlist)) return pathlist;
	if(!LIST(pathlist)) return EXC(Tt(type),"pathlist only works with lists of strings",pathlist,NULL);
	VP item,sep=xfroms("/"),acc=xcsz(64); int i,pn=pathlist->n;
	for(i=0;i<pn;i++){
		item=ELl(pathlist,i);
		if(!IS_c(item)) item=str(item); // try to convert to string, but still bomb: 
		if(!IS_c(item)) return EXC(Tt(type),"pathlist only works with lists of strings",pathlist,NULL);
		if(i!=0 && AS_c(item,0)!='.') appendbuf(acc,BUF(sep),sep->n);
		appendbuf(acc,BUF(item),item->n);
	}
	xfree(sep);
	XRAY_log("filepath returning\n");XRAY_emit(acc);
	return acc;
}
VP fileset(VP str,VP fn) {
	VP fname=fn;
	if(!IS_c(fname)) fname=filepath(fn);
	if(!IS_c(fname)) return EXC(Tt(type),"writefile filename must be string or pathlist",fn,0);
	if(!IS_c(str)) return EXC(Tt(type),"writefile only deals writes strings right now",str,fn);
	char* fns=sfromxA(fname);
	int fd=open(fns,O_CREAT|O_WRONLY,0600);
	free(fns);
	if(fd<0) return EXC(Tt(open),"could not open file for writing",str,fname);
	if(write(fd,ELb(str,0),str->n)<str->n) return EXC(Tt(write),"could not write file contents",str,fname);
	close(fd);
	return str;
}
#endif

#ifdef STDLIBHTTP
#ifdef STDLIBNET
VP httpget(VP url) {
}
#endif
#endif

#ifdef STDLIBSHAREDLIB
VP sharedlibget(VP fn) {
	if(!IS_c(fn)) return EXC(Tt(type),".sharedlib.get requires a filename string as argument",fn,0);
	char* fns=sfromxA(fn);
	void* fp;
	fp=dlopen(fns,RTLD_LAZY);
	free(fns);
	if(fp==NULL) { 
		printf("dlerr:%s\n",dlerror());
		return EXC(Tt(open),".sharedlib.get could not open shared library",fn,0);
	}
	void* indexp;
	indexp=dlsym(fp,"XXL_INDEX");
	if(indexp==NULL) return EXC(Tt(read),".sharedlib.get could not read index",fn,0);
	// XRAY_LVL=10;
	struct xxl_index_t* idx;
	idx=indexp;
	int i=0;
	VP contents=xd0();
	while(idx[i].arity != 0) {
		void* itemp;
		itemp=dlsym(fp,idx[i].implfunc);
		if(itemp==NULL) { xfree(contents); return EXC(Tt(read),".sharedlib.get could not read item",fn,0); }
		if(idx[i].arity == 1)
			contents=assign(contents,xt(_tagnums(idx[0].name)),x1(itemp));
		else
			contents=assign(contents,xt(_tagnums(idx[0].name)),x2(itemp));
		idx++;
	}
	XRAY_emit(contents);
	return contents;
}
VP sharedlibset(VP fn,VP funcs) {
	return EXC(Tt(nyi),".sharedlib.set nyi",fn,funcs);
}
#endif 

#ifdef STDLIBSHELL 
VP shellget(VP cmd) {
	if(!IS_c(cmd)) return EXC(Tt(type),".shell.get requires command arg as a string",cmd,0);
	char buf[IOBLOCKSZ]={0}; size_t r; char* cmds=sfromxA(cmd); 
	FILE* fp=popen(cmds,"r");
	if(fp==NULL) return free(cmds),EXC(Tt(popen),"popen failed",cmd,0);
	VP acc=xcsz(IOBLOCKSZ);
	while(fgets(buf,IOBLOCKSZ-1,fp)!=NULL)
		appendbuf(acc,(buf_t)buf,strlen(buf));
	fclose(fp);
	return free(cmds),acc;
}
#endif

#ifdef STDLIBMBOX
#ifdef THREAD
#define IS_mbox(mb) (LIST(mb) && mb->tag==Ti(mbox))
#define MBOX_tm struct timespec tm;tm.tv_sec=5;tm.tv_nsec=0
#define MBOX_usleep 5000
VP mboxnew(VP x) {
	pthread_mutex_t *rm, *wm; VP res;
	rm = malloc(sizeof(pthread_mutex_t)); pthread_mutex_init(rm, NULL);
	wm = malloc(sizeof(pthread_mutex_t)); pthread_mutex_init(wm, NULL);
	return entags(xln(3, xj((long long)rm), xj((long long)wm), xl0()), "mbox");
}
VP mboxsend(VP mbox,VP msg) {
	if(!IS_mbox(mbox)) { return EXC(Tt(type),"mbox.send needs a mailbox in x",mbox,msg); }
	pthread_mutex_t* wm=(pthread_mutex_t*)AS_j(LIST_item(mbox, 1),0);
	MBOX_tm;
	#ifndef THREAD_NO_TIMEDLOCK
		int ret=pthread_mutex_timedlock(wm, &tm);
		if(ret!=0) { return Tt(timeout); }
	#else
		int ret=pthread_mutex_lock(wm);
		if(ret!=0) { return Tt(nomutex); }
	#endif
	VP items=ELl(mbox,2);
	ELl(mbox,2)=append(items,msg);
	pthread_mutex_unlock(wm);
	return msg;
}
VP mboxrecv0(VP mbox,int pop) {
	pthread_mutex_t* rm=(pthread_mutex_t*)AS_j(LIST_item(mbox, 0),0);
	MBOX_tm;
	#ifndef THREAD_NO_TIMEDLOCK
		int ret=pthread_mutex_timedlock(rm, &tm);
		if(ret!=0) { return Tt(timeout); }
	#else
		int ret=pthread_mutex_lock(rm);
		if(ret!=0) { return Tt(nomutex); }
	#endif
	VP res=0;
	VP msgs=LIST_item(mbox,2);
	if(LEN(msgs)) {
		res=LIST_item(msgs,0);
		if(pop) ELl(mbox,2)=behead(msgs);
	}
	pthread_mutex_unlock(rm);
	return res;
}
VP mboxpeek(VP mbox) {
	if(!IS_mbox(mbox)) { return EXC(Tt(type),"mbox.recv needs a mailbox in x",mbox,0); }
	return mboxrecv0(mbox,0);
}
VP mboxrecv(VP mbox) {
	if(!IS_mbox(mbox)) { return EXC(Tt(type),"mbox.recv needs a mailbox in x",mbox,0); }
	return mboxrecv0(mbox,1);
}
VP mboxquery(VP mbox,VP msg) {
	XRAY_log("mboxquery\n");XRAY_emit(mbox);XRAY_emit(msg);
	VP me=mboxnew(XI0), tmp=xln(2,msg,me);
	mboxsend(mbox,tmp);
	while (1) {
		VP resp=mboxwait(me); if(resp) { xfree(tmp); xfree(me); return resp; }
	}
	return NULL; // todo timeouts
}
VP mboxwait(VP mbox) {
	XRAY_log("mboxwait\n");XRAY_emit(mbox);
	VP msg;
	tag_t ping=Ti(ping);
	int empties=0,founds=0;
	do {
		msg=mboxrecv(mbox);
		if(msg!=NULL) {
			if(LIST(msg) && LEN(msg)==2 && IS_t(LIST_item(msg,0)) && IS_mbox(LIST_item(msg,1)) && AS_t(LIST_item(msg,0),0)==ping) {
				VP tmp=xln(4,Tt(pong),mbox,xi(founds),xi(empties));
				mboxsend(LIST_item(msg,1),tmp);
				xfree(tmp);
			} else return msg;
		}
		else {
			sched_yield();
			empties++; if(empties>2) usleep(MAX(empties*100,MBOX_usleep)); // arb
		}
	} while (1);
	return NULL; // cant happen
}
VP mboxwatch0(VP args) {
	VP exitmsg=Tt(exit);
	VP mbox=LIST_item(args,0); 
	VP cb=LIST_item(args,1);
	VP state=xl0(), newstate;
	VP msg;
	do {
		msg=mboxwait(mbox);
		newstate=apply2(cb, msg, state);
		xfree(state);
		state=newstate;
		xfree(msg);
	} while (state!=exitmsg);
	xfree(exitmsg);
	xfree(state);
	return NULL;
}
VP mboxwatch(VP mbox,VP callback) {
	if(!IS_mbox(mbox)) { return EXC(Tt(type),"Mbox.watch needs a mailbox in x",mbox,callback); }
	if(!CALLABLE(callback)) { return EXC(Tt(type),"Mbox.watch needs a callback in y",mbox,callback); }
	thr_run1(x1(&mboxwatch0),xln(2,mbox,callback));
	return mbox;
}
#endif
#endif 

#ifdef STDLIBXD
VP xdget0_(VP fname,int fd) {
	int sz;
	if(read(fd,&sz,4)<4) return EXC(Tt(write),"could not read file size",fname,0);
	if(sz!=sizeof(struct V)) return EXC(Tt(read),"could not read file structure due to size",fname,0);
	VP data=calloc(sz,1);
	if(read(fd,data,sz)<sz) return EXC(Tt(read),"could not read file structure",fname,0);
	data->rc=1;
	data->alloc=1;
	data->dyn=calloc(data->sz,1);
	if(CONTAINER(data)) {
		int i=0, dn=LEN(data);
		for(i=0; i<dn; i++) ELl(data,i)=xdget0_(fname,fd);
	} else read(fd,BUF(data),data->sz);
	return data;
}
VP xdget(const VP fn) {
	VP fname=fn;
	if(!IS_c(fname)) fname=filepath(fn);
	if(!IS_c(fname)) return EXC(Tt(type),"xdget filename must be string or pathlist",fn,0);
	char* fns=sfromxA(fname);
	int fd=open(fns,O_RDONLY);
	if(!fd) perror("open");
	free(fns);
	if(fd<0) return EXC(Tt(open),"could not open file for reading",fname,0);
	char hdr[4];
	if(read(fd,hdr,4)<4) return EXC(Tt(read),"could not write file header",fname,0);
	VP res=xdget0_(fname,fd);
	close(fd);
	return res;
}
VP xdset0_(VP fname,int fd, const VP data) {
	int sz=sizeof(struct V);
	if(write(fd,&sz,4)<4) return EXC(Tt(write),"could not write file size",fname,data);
	if(write(fd,data,sz)<sz) return EXC(Tt(write),"could not write file structure",fname,data);
	if(CONTAINER(data)) {
		int i=0, dn=LEN(data);
		for(i=0; i<dn; i++) xdset0_(fname,fd,LIST_item(data,i));
	} else write(fd,BUF(data),data->sz);
}
VP xdset(const VP data,const VP fn) {
	VP fname=fn;
	if(!IS_c(fname)) fname=filepath(fn);
	if(!IS_c(fname)) return EXC(Tt(type),"xdset filename must be string or pathlist",fn,0);
	char* fns=sfromxA(fname);
	int fd=open(fns,O_CREAT|O_WRONLY,0600);
	if(!fd) perror("open");
	free(fns);
	if(fd<0) return EXC(Tt(open),"could not open file for writing",fname,data);
	if(write(fd,"XXL0",4)<4) return EXC(Tt(write),"could not write file header",fname,data);
	VP res=xdset0_(fname,fd,data);
	close(fd);
	if(IS_EXC(res)) return res;
	else return data;
}
#endif

