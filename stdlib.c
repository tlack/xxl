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
	VP res=xfroms(basename(sfromx(fname)));
	xfree(fname);
	return res;
}
VP filedirname(VP fn) {
	VP fname=fn;
	if(!IS_c(fname)) fname=filepath(fn);
	if(!IS_c(fname)) return EXC(Tt(type),"dirname filename must be string or pathlist",fn,0);
	VP res=xfroms(dirname(sfromx(fname))), slash=xc('/');
	res=append(res,slash);
	xfree(fname);
	return res;
}
VP filecwd(VP dummy) {
	char cwd[1024];
	if(getcwd(cwd,sizeof(cwd))!=NULL) 
		return xfroms(cwd);
	else
		return EXC(Tt(open),"couldnt get current working directory",0,0);	
}
VP fileget(VP fn) {
	int r, fd; char buf[IOBLOCKSZ]; VP fname=fn;
	if(!IS_c(fname)) fname=filepath(fn);
	if(!IS_c(fname)) return EXC(Tt(type),"readfile filename must be string or pathlist",fn,0);
	//PF_LVL++;PF("fileget\n");DUMP(fn);PF_LVL--;
	if(LEN(fname)==1 && AS_c(fname,0)=='-') fd=STDIN_FILENO; 
	else fd=open(sfromx(fname),O_RDONLY);
	if(fd<0) return EXC(Tt(open),"could not open file for reading",fname,0);
	VP acc=xcsz(IOBLOCKSZ);
	do {
		r=read(fd,buf,IOBLOCKSZ);
		//printf("fileget read %d\n",r);
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
	PF("filepath returning\n");DUMP(acc);
	return acc;
}
VP fileset(VP str,VP fn) {
	VP fname=fn;
	if(!IS_c(fname)) fname=filepath(fn);
	if(!IS_c(fname)) return EXC(Tt(type),"writefile filename must be string or pathlist",fn,0);
	if(!IS_c(str)) return EXC(Tt(type),"writefile only deals writes strings right now",str,fn);
	int fd=open(sfromx(fname),O_WRONLY);
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
	const char* fns=sfromx(fn);
	void* fp;
	fp=dlopen(fns,RTLD_LAZY);
	if(fp==NULL) { 
		printf("dlerr:%s\n",dlerror());
		return EXC(Tt(open),".sharedlib.get could not open shared library",fn,0);
	}
	void* indexp;
	indexp=dlsym(fp,"XXL_INDEX");
	if(indexp==NULL) return EXC(Tt(read),".sharedlib.get could not read index",fn,0);
	// PF_LVL=10;
	struct xxl_index_t* idx;
	idx=indexp;
	int i=0;
	VP contents=xd0();
	while(idx[i].arity != 0) {
		void* itemp;
		// printf("%s %d\n", idx[0].name, idx[0].arity);
		itemp=dlsym(fp,idx[i].implfunc);
		if(itemp==NULL) { xfree(contents); return EXC(Tt(read),".sharedlib.get could not read item",fn,0); }
		// printf("ptr=%p\n", itemp);
		if(idx[i].arity == 1)
			contents=assign(contents,xt(_tagnums(idx[0].name)),x1(itemp));
		else
			contents=assign(contents,xt(_tagnums(idx[0].name)),x2(itemp));
		idx++;
	}
	DUMP(contents);
	return contents;
}
VP sharedlibset(VP fn,VP funcs) {
	return EXC(Tt(nyi),".sharedlib.set nyi",fn,funcs);
}
#endif 

#ifdef STDLIBSHELL 
VP shellget(VP cmd) {
	if(!IS_c(cmd)) return EXC(Tt(type),".shell.get requires command arg as a string",cmd,0);
	char buf[IOBLOCKSZ]={0}; size_t r; const char* cmds=sfromx(cmd); 
	FILE* fp=popen(cmds,"r");
	if(fp==NULL) return EXC(Tt(popen),"popen failed",cmd,0);
	VP acc=xcsz(IOBLOCKSZ);
	while(fgets(buf,IOBLOCKSZ-1,fp)!=NULL)
		appendbuf(acc,(buf_t)buf,strlen(buf));
	fclose(fp);
	return acc;
}
#endif

