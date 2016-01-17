#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#ifdef THREAD
#include <pthread.h>
#endif

/* macros and helpers, function prototypes */
#include "def.h"
#include "proto.h"

/* global variables :( */
static I8 PF_ON=0;
static I8 PF_LVL=0;
#define GOBBLERSZ 50
static VP MEM_RECENT[GOBBLERSZ] = {0};
static I8 MEM_W=0; //watching memory?
#define N_MEM_PTRS 1024
static VP MEM_PTRS[N_MEM_PTRS]={0};
static I32 MEM_ALLOC_SZ=0,MEM_FREED_SZ=0;
static I32 MEM_ALLOCS=0, MEM_REALLOCS=0, MEM_FREES=0, MEM_GOBBLES=0;
static VP TAGS=NULL;

#ifdef THREAD
static pthread_mutex_t mutmem=PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t muttag=PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutthr=PTHREAD_MUTEX_INITIALIZER;
#define MAXTHR 128
static int NTHR=0;
static pthread_t THR[MAXTHR]={0};
#define WITHLOCK(name,code) ({ pthread_mutex_lock(&mut##name); code; pthread_mutex_unlock(&mut##name); })
#else
#define WITHLOCK(name,code) ({ code; })
#endif

#include "accessors.h"
#include "vary.h"

/*
char* repr_1(VP x,char* s,size_t sz) {
	APF(sz,"[ unary func = %p ],",x);
	return s;
}
char* repr_2(VP x,char* s,size_t sz) {
	APF(sz,"[ binary func = %p ],",x);
	return s;
}
char* repr_p(VP x,char* s,size_t sz) {
	APF(sz,"[ projection = %p ],",x);
	return s;
}
*/
char* repr0(VP x,char* s,size_t sz) {
	type_info_t t;
	if(x==NULL) { APF(sz,"/*null*/",0); return s; }
	t=typeinfo(x->t);
	if(0&&DEBUG) {
		APF(sz," /*%p %s tag=%d#%s itemsz=%d n=%d rc=%d*/ ",x,t.name,
			x->tag,(x->tag!=0 ? sfromx(tagname(x->tag)) : ""),
			x->itemsz,x->n,x->rc);
	}
	if(x->tag!=0) 
		APF(sz, "`%s(", sfromx(tagname(x->tag)));
	if(t.repr) (*(t.repr)(x,s,sz));
	if(x->tag!=0)
		APF(sz, ")", 0);
	return s;
}
char* reprA(VP x) {
	#define BS 1024
	char* s = calloc(1,BS);
	s = repr0(x,s,BS);
	APF(BS,"\n",0);
	return s;
}
char* repr_l(VP x,char* s,size_t sz) {
	int i=0, n=x->n;VP a;
	APF(sz,"[",0);
	for(i=0;i<n;i++){
		a = ELl(x,i);
		repr0(a,s,sz);
		if(i!=n-1)
			APF(sz,", ",0);
		// repr0(*(EL(x,VP*,i)),s,sz);
	}
	APF(sz,"]",0);
	return s;
}
char* repr_c(VP x,char* s,size_t sz) {
	int i=0,n=x->n,ch;
	APF(sz,"\"",0);
	for(;i<n;i++){
		ch = AS_c(x,i);
		if(ch=='"') APF(sz,"\\", 0);
		APF(sz,"%c",ch);
		// repr0(*(EL(x,VP*,i)),s,sz);
	}
	APF(sz,"\"",0);
	return s;
}
char* repr_t(VP x,char* s,size_t sz) {
	int i=0,n=x->n,tag;
	if(n>1) APF(sz,"[",0);
	for(;i<n-1;i++){
		tag = AS_t(x,i);
		APF(sz,"`%s",sfromx(tagname(tag)));
		if(i!=n-1)
			APF(sz,",",0);
		// repr0(*(EL(x,VP*,i)),s,sz);
	}
	if(n>1) APF(sz,"]",0);
	return s;
}
char* repr_x(VP x,char* s,size_t sz) {
	int i;VP a;
	APF(sz,"[ ",0);
	for(i=0;i<x->n;i++){
		a = ELl(x,i);
		APF(sz,"%d:",i);
		repr0(a,s,sz);
		APF(sz,", ",0);
		// repr0(*(EL(x,VP*,i)),s,sz);
	}
	APF(sz," ]",0);
	return s;
}
char* repr_d(VP x,char* s,size_t sz) {
	int i;
	VP k=KEYS(x),v=VALS(x);
	if (!k || !v) { APF(sz,"[null]",0); return s; }
	APF(sz,"[ keys (%d): ",k->t);
	repr0(k,s,sz);
	APF(sz," ],[ values (%d): ",v->t);
	repr0(v,s,sz);
	APF(sz," ]",0);
	return s;
}
#include "repr.h"
#include "types.h"

type_info_t typeinfo(type_t n) { ITER(TYPES,sizeof(TYPES),{ IF_RET(_x.t==n,_x); }); }
type_info_t typechar(char c) { ITER(TYPES,sizeof(TYPES),{ IF_RET(_x.c==c,_x); }); }
VP xalloc(type_t t,I32 initn) {
	VP a; int g,i,itemsz,sz; 
	initn = initn < 4 ? 4 : initn;
	itemsz = typeinfo(t).sz; sz=itemsz*initn;
	//PF("%d\n",sz);
	a=NULL;g=0;
	if (GOBBLERSZ > 0) {
		WITHLOCK(mem, {
		for(i=0;i<GOBBLERSZ;i++) {
			/*
			if(DEBUG && MEM_W &&
				 MEM_RECENT[i]!=0) {
				printf("%d. %d\n", i, (VP)MEM_RECENT[i]->sz);
			}
			*/
			if(MEM_RECENT[i]!=0 && 
				 ((VP)MEM_RECENT[i])->sz > sz &&
				 ((VP)MEM_RECENT[i])->sz < (sz * 20)) {
				a=MEM_RECENT[i];
				MEM_RECENT[i]=0;
				MEM_GOBBLES++;
				g=i;
				memset(BUF(a),0,a->sz);
				break;
			}
		}
		});
	} 
	if(a==NULL)
		a = calloc(sizeof(struct V)+sz,1);
	if (MEM_W) {
		WITHLOCK(mem, {
		MEMPF("%salloc %d %p %d (%d * %d) (total=%d, freed=%d, bal=%d)\n",(g==1?"GOBBLED! ":""),t,a,sizeof(struct V)+sz,initn,itemsz,MEM_ALLOC_SZ,MEM_FREED_SZ,MEM_ALLOC_SZ-MEM_FREED_SZ);
		MEM_ALLOC_SZ += sizeof(struct V)+sz;
		MEM_ALLOCS++;
		for(i=0;i<N_MEM_PTRS;i++) {
			if (MEM_PTRS[i]==0)
				MEM_PTRS[i]=a;
		}
		});
	}
	a->t=t;a->tag=0;a->n=0;a->rc=1;a->cap=initn;a->sz=sz;a->itemsz=itemsz;
	return a;
}
VP xprofile_start() {
	MEM_W=1;
}
VP xprofile_end() {
	int i;
	VP ctx;
	VP res; 
	MEM_W=0;
	printf("allocs: %d (%d), gobbles: %d, reallocs: %d, frees: %d\n", MEM_ALLOC_SZ, MEM_ALLOCS, MEM_GOBBLES, MEM_REALLOCS, MEM_FREES);
	for(i=0;i<N_MEM_PTRS;i++)
		if(MEM_PTRS[i]!=0) {
			xfree(MEM_PTRS[i]);
			MEM_PTRS[i]=0;
		}
	
	// 0..999:        4953
	// 1000..1999:    24
	// 2000..2999:    293
	// 3000..3999:    100
	//
	// count each group 1000 xbar capacity each MEM_PTRS
	/*
	xxl(
		(ctx,"sizes",xxl(MEMPTRS,x2(&map),x1(&capacity),x2(&map),x2(&xbar),xi(1000),0))
	*/
}
VP xrealloc(VP x,I32 newn) {
	// PF("xrealloc %p %d\n",x,newn);
	if(newn>x->cap) {
		buf_t newp; I32 newsz;
		newn = (newn < 10*1024) ? newn * 4 : newn * 1.25;
		newsz = newn * x->itemsz;
		if(x->alloc) {
			// PF("realloc %p %d %d %d\n", x->dyn, x->sz, newn, newsz);
			newp = realloc(x->dyn, newsz);
		} else {
			// PF("calloc sz=%d, n=%d, newn=%d, newsz=%d\n", x->sz, x->n, newn, newsz);
			newp = calloc(newsz,1);
			memmove(newp,BUF(x),x->sz);
		}
		if(MEM_W) {
			// MEMPF("realloc %d %p -> %d\n", x->t, x, newsz);
			MEM_ALLOC_SZ += newsz;
			MEM_REALLOCS++;
		}
		// PF("realloc new ptr = %p\n", newp);
		if(newp==NULL) perror("realloc");
		x->dyn=newp;
		x->cap=newn;
		x->sz=newsz;
		x->alloc=1;
		// PF("post realloc\n"); DUMP(x);
	}
	return x;
}
VP xfree(VP x) {
	int i;
	if(x==NULL)return x;
	//PF("xfree(%p)\n",x);
	//printf("
	x->rc--; 
	if(x->rc==0){
		if(LISTDICT(x))
			ITERV(x,xfree(ELl(x,_i)));
		if(MEM_W) {
			MEM_FREED_SZ+=sizeof(struct V) + x->sz;
			MEM_FREES+=1;
			MEMPF("free %d %p %d (%d * %d) (total=%d, freed=%d, bal=%d)\n",x->t,x,x->sz,x->itemsz,x->cap,MEM_ALLOC_SZ,MEM_FREED_SZ,MEM_ALLOC_SZ-MEM_FREED_SZ);
		}
		if (GOBBLERSZ > 0) {
			for(i=0;i<GOBBLERSZ;i++)
				if(MEM_RECENT[i]==0) {
					MEM_RECENT[i]=x;
					return x;
				}
		}
		free(x);
		//PF("free%p\n",x);free(x);
	} return x; }
VP xref(VP x) { if(MEM_W){MEMPF("ref %p\n",x);} x->rc++; return x; }
VP xfroms(const char* str) {  // character value from string - strlen helper
	size_t len = strlen(str)+1; type_info_t t = typechar('c');
	VP a = xalloc(t.t,len); memcpy(BUF(a),str,len); a->n=len; return a; }
const char* sfromx(VP x) { 
	if(x==NULL)return "null";
	return BUF(x); }

// RUNTIME!!
VP appendbuf(VP x,buf_t buf,size_t nelem) {
	int newn;buf_t dest;
	newn = x->n+nelem;
	x=xrealloc(x,newn);
	// PF("after realloc"); DUMP(x);
	dest = ELi(x,x->n);
	memmove(dest,buf,x->itemsz * nelem);
	x->n=newn;
	// PF("appendbuf newn %d\n", newn); DUMPRAW(dest, x->itemsz * newn);
	return x;
}
VP append(VP x,VP y) { // append all items of y to x. if x is a general list, append pointer to y, and increase refcount.
	// PF("append %p %p\n",x,y); DUMP(x); DUMP(y);
	ASSERT(LISTDICT(x) || (x->t==y->t), "append(): x must be list, or types must match");
	if(IS_d(x)) {
		ASSERT(y->n % 2 == 0, "append to a dict with [`key;`value]");
		VP k=KEYS(x),v=VALS(x),y1,y2; int i;
		y1=ELl(y,0);
		y2=ELl(y,1);
		// tough decisions
		if(k==NULL) { // create dict
			if(0 && SCALAR(y1)) {
				k=xalloc(y1->t, 4);
			} else {
				k=xl0();
			}
			v=xl0();
			xref(k);xref(v);
			EL(x,VP,0)=k;
			EL(x,VP,1)=v;
			// PF("dict kv %p %p\n", k, v);
			DUMP(k);
			PF("NEW DICT VALUES\n");
			DUMP(v);
			i=-1;
		} else 
			i=_find(k,y1);
		if(i==-1) {
			xref(y1);xref(y2);
			EL(x,VP,0)=append(k,y1);
			EL(x,VP,1)=append(v,y2);
		} else {
			xref(y2);
			ELl(v,i)=y2;
		}
		return x;
	}
	if(LIST(x)) { 
		// PF("append %p to list %p\n", y, x); DUMP(x);
		x=xrealloc(x,x->n+1);
		xref(y);
		EL(x,VP,x->n)=y;
		x->n++;
		// PF("afterward:\n"); DUMP(x);
	} else {
		buf_t dest;
		dest = BUF(x) + (x->n*x->itemsz);
		x=xrealloc(x,x->n + y->n);
		memmove(ELsz(x,x->itemsz,x->n),BUF(y),y->sz);
		x->n+=y->n;
	}
	return x;
}
VP appendfree(VP x,VP y) {
	append(x,y); xfree(y); return x;
}
VP upsert(VP x,VP y) {
	if(_find(x,y)==-1) append(x,y); return x;
}
int _upsertidx(VP x,VP y) {
	int idx = _find(x,y);
	if(idx>-1) return idx;
	append(x,y); return x->n-1;
}
inline VP assign(VP x,VP k,VP val) {
	if(DICT(x)) {
		return append(x,xln(2,k,val));
	}
	ASSERT(1,"assign() bad types");
}
inline VP assigns(VP x,const char* key,VP val) {
	return assign(x,xfroms(key),val);
}
VP flatten(VP x) {
	int i,t=-1;VP res;
	if(!LIST(x))return x;
	if(x->n) {
		t=ELl(x,0)->t; res=xalloc(t,x->n);
		for(i=0;i<x->n;i++) {
			if(ELl(x,i)->t!=t) {
				xfree(res); return x;
			} else
				append(res,ELl(x,i));
		}
	}
	return res;
}
VP take_(VP x,int i) {
	VP res = xalloc(x->t,x->n-i);
	memcpy(ELi(res,0),ELi(x,i),i*x->itemsz);
	return res;
}
VP take(VP x,VP y) {
	VP res;
	int typerr=-1;
	size_t st,end; //TODO slice() support more than 32bit indices
	// PF("take args\n"); DUMP(x); DUMP(y);
	IF_RET(!NUM(y), EXC(Tt(type),"slice arg must be numeric",x,y));	
	VARY_EL(y, 0, {if (_x<0) { st=x->n+_x; end=x->n; } else { st=0; end=_x; }}, typerr);
	IF_RET(typerr>-1, EXC(Tt(type),"cant use y as slice index into x",x,y));  
	IF_RET(end>x->n, xalloc(x->t, 0));
	res=xalloc(x->t,end-st); res=appendbuf(res,ELi(x,st),end-st);
	// PF("take result\n"); DUMP(res);
	return res;
}
VP replaceleft(VP x,int n,VP replace) { // replace first i values with just 'replace'
	int i;
	ASSERT(LIST(x),"replaceleft arg must be list");
	for(i=0;i<n;i++) xfree(ELl(x,i));
	if(n>1) {
		memmove(ELi(x,1),ELi(x,n),x->itemsz*(x->n-n));
	}
	EL(x,VP,0)=replace;
	x->n=x->n-i;
	return x;
}
VP split(VP x,VP tok) {
	PF("split");DUMP(x);DUMP(tok);
	VP tmp,tmp2;int locs[1024]; int typerr=-1;

	// special case for empty or null tok.. split vector into list
	if(tok->n==0) {
		tmp = xl0();
		VARY_EACH(x,({
			PF("in split vary_each %c\n",_x);
			tmp2=xalloc(x->t, 1);
			tmp2=appendbuf(tmp2,(buf_t)&_x,1);
			tmp=append(tmp,tmp2);
			DUMP(tmp2);
			DUMP(tmp);
		}),typerr);
		IF_RET(typerr>-1, EXC(Tt(type),"can't split that type", x, tok));
		PF("split returning\n");DUMP(tmp);
		return tmp;
	}
	VARY_EACH(x,({
		// if this next sequence matches tok,
		//tmp = match(_x, tok);
		//DUMP(tmp); // nyi
	}), typerr);
}
inline int _equalm(VP x,int xi,VP y,int yi) {
	// PF("comparing %p to %p\n", ELi(x,xi), ELi(y,yi));
	// PF("_equalm\n"); DUMP(x); DUMP(y);
	if(ENLISTED(x)) { PF("equalm descend x");
		return _equalm(ELl(x,xi),0,y,yi);
	}
	if(ENLISTED(y)) { PF("equalm descend y");
		return _equalm(x,xi,ELl(y,yi),0);
	}
	if(memcmp(ELi(x,xi),ELi(y,yi),x->itemsz)==0) return 1;
	else return 0;
}	
int _equal(VP x,VP y) {
	// TODO _equal() needs to handle comparison tolerance and type conversion
	// TODO _equal should use the new VARY_*() macros, except for general lists
	// PF("_equal\n"); DUMP(x); DUMP(y);
	// if the list is a container for one item, we probably want to match the inner one
	if(LIST(x) && SCALAR(x)) x=ELl(x,0);
	if(LIST(y) && SCALAR(y)) y=ELl(y,0);
	IF_RET(x->n != y->n, 0);
	if(LIST(x) && LIST(y)) { ITERV(x,{ IF_RET(_equal(ELl(x,_i),ELl(y,_i))==0, 0); }); return 1; }
	ITERV(x,{ IF_RET(memcmp(ELb(x,_i),ELb(y,_i),x->itemsz)!=0,0); });
	//PF("_equal=1!\n");
	return 1;
}
int _findbuf(VP x,buf_t y) {
	if(LISTDICT(x)) { ITERV(x,{ 
		IF_RET(_findbuf(ELl(x,_i),y)!=-1,_i);
	}); } else {
		ITERV(x,{ IF_RET(memcmp(ELi(x,_i),y,x->itemsz)==0,_i); });
	}
	return -1;
}
int _find(VP x,VP y) {
	ASSERT(LIST(x) || (x->t==y->t && y->n==1), "_find(): x must be list, or types must match with right scalar");
	// PF("find %p %p\n",x,y); DUMP(x); DUMP(y);
	if(LISTDICT(x)) { ITERV(x,{ 
		VP xx;
		xx=ELl(x,_i);
		if(xx!=NULL) 
			IF_RET(_equal(xx,y)==1,_i);
	}); }
	else {
		ITERV(x,{ IF_RET(memcmp(ELi(x,_i),ELi(y,0),x->itemsz)==0,_i); });
	}
	return -1;
}
VP find(VP x,VP y) {
	return xi(_find(x,y));
}
int _contains(VP x,VP y) {
	return _find(x,y)==-1 ? 0 : 1;
}
VP contains(VP x,VP y) {
	return xi(_contains(x,y));
}
inline VP times(VP x,VP y) {
	VP r = xalloc(x->t, x->n);
	ITER2(x,y,{ append(r,xi(_x*_y)); });
	return r;
}
inline VP each(VP verb,VP noun) {

}
VP cast(VP x,VP y) { 
	// TODO cast() should short cut matching kind casts 
	#define BUFSZ 128
	VP res; I8 buf[BUFSZ]={0}; int typetag=-1;type_t typenum=-1; 
	// right arg is tag naming a type, use that.. otherwise use y's type
	if(y->t==T_t) typetag=AS_t(y,0); else typenum=y->t;
	#include"cast.h"
	DUMPRAW(buf,BUFSZ);
	return res;
}
VP len(VP x) {
	return xin(1,x->n);
}
VP capacity(VP x) {
	return xin(1,x->cap);
}
VP itemsz(VP x) {
	return xin(1,x->itemsz);
}
VP til(VP x) {
	VP acc;int i;int typerr=-1;
	PF("TIL!!!!\n"); DUMP(x);
	VARY_EL(x, 0, 
		{ __typeof__(_x) i; acc=xalloc(x->t,_x); acc->n=_x; for(i=0;i<_x;i++) { EL(acc,__typeof__(i),i)=i; } }, 
		typerr);
	IF_RET(typerr>-1, EXC(Tt(type), "til arg must be numeric", x, 0));
	DUMP(acc);
	return acc;
}
VP plus(VP x,VP y) {
}
VP sum(VP x) {
	PF("sum");DUMP(x);
	I128 val=0;int i;
	if(!IS_i(x)) return EXC(Tt(type),"sum argument should be numeric",x,0);
	for(i=0;i<x->n;i++) val+=AS_i(x,i);
	return xo(val);
}
VP labelitems(VP label,VP items) {
	VP res;
	PF("labelitems\n");DUMP(label);DUMP(items);
	res=flatten(items);res->tag=_tagnums(sfromx(label));
	DUMP(res);
	return res;
}
VP mkproj(int type, void* func, VP left, VP right) {
	Proj p;
	VP pv=xpsz(1);
	p.type=type;
	if(type==1) {
		p.f1=func; p.left=left; p.right=0;
	} else {
		p.f2=func; p.left=left; p.right=right;
	}
	EL(pv,Proj,0)=p;
	pv->n=1;
	return pv;
}
VP apply(VP x,VP y) {
	VP res=NULL;int i,typerr=-1;
	//PF("apply\n");DUMP(x);DUMP(y);
	if(DICT(x)) {
		VP k=KEYS(x),v=VALS(x);I8 found;
		if(k==NULL || v==NULL) return NULL;
		res=xi0();
		if(IS_x(y)) {
			ITERV(y,{
				res=match(ELl(y,_i),k);
				PF("context match\n");DUMP(res);
				if(res->n) {
					VP rep;
					rep=ELl(v,_i);
					if(IS_1(rep) || IS_2(rep))
						rep=apply(rep,apply(y,res));
					y=replaceleft(y,res->n,rep);
				}
			});
		} else {
			ITERV(y,{ 
				int idx;
				PF("searching %d\n",_i);
				if(LIST(y)) idx = _find(k,ELl(y,_i));
				else idx = _findbuf(k,ELi(y,_i));
				if(idx>-1) {
					found=1;
					PF("found at idx %d\n", idx); append(res,xi(idx));
				}
			});
		}
		if(res->n==0) {
			if(x->next!=0) res=apply((VP)x->next, res);
		}
		if(res->n==0) return xl0(); else return apply(v,res);
	}
	if(IS_p(x)) { 
		// if its dyadic
		   // if we have one arg, add y, and call - return result
			 // if we have no args, set y as left, and return x
		// if its monadic
			// if we have one arg already, call x[left], and then apply result with y
			// i think this is right..
		Proj* p; p=ELi(x,0);
		if(!p->left) p->left=y; else if (!p->right) p->right=y;
		if(p->type==1 && p->left) return (*p->f1)(p->left);
		if(p->type==2 && p->left && p->right) return (*p->f2)(p->left,p->right);
	}
	if(IS_1(x)) {
		unaryFunc* f; f=AS_1(x,0); return (*f)(y);
	}
	if(IS_2(x)) {
		return mkproj(2,AS_2(x,0),y,0);
	}
	if(NUM(y)) {
		// index a value with an integer 
		if(y->n==1 && LIST(x)) {
			// special case for generic lists:
			// if you index with one item, return just that item
			// generally you would receive a list back
			// this may potentially become painful later on 
			i = AS_i(y,0); 
			IF_RET(i>=x->n, EXC(Tt(index),"index out of range",x,y));
			VP tmp = ELl(x,i); xref(tmp); return tmp;
		} else {
			res=xalloc(x->t,y->n);
			VARY_EACH(y,appendbuf(res,ELi(x,_x),1),typerr);
			//PF("apply VARY_EACH after\n"); DUMP(res);
			if(typerr>-1) return EXC(Tt(type),"cant use y as index into x",x,y);
			return res;
		}
	}
}

// TAG STUFF:

VP tagwrap(VP tag,VP x) {
	return entag(xln(1, x),tag);
}
VP tagv(const char* name, VP x) {
	return entags(xln(1,x),name);
}
inline VP entag(VP x,VP t) {
	if(IS_c(t))
		x->tag=_tagnum(t);
	else if (IS_i(t))
		x->tag=AS_i(t,0);
	return x;
}
inline VP entags(VP x,const char* name) {
	x->tag=_tagnums(name);
	return x;
}
inline VP tagname(const I32 tag) {
	VP res;
	// PF("tagname(%d)\n", tag);
	// DUMP(TAGS);
	if(TAGS==NULL) { TAGS=xl0();TAGS->rc=INT_MAX; }
	if(tag>=TAGS->n) return xfroms("unknown");
	res = ELl(TAGS,tag);
	// PF("tagname res\n");
	// DUMP(res);
	return res;
}
const char* tagnames(const I32 tag) {
	return sfromx(tagname(tag));
}
inline int _tagnum(const VP s) {
	int i;
	WITHLOCK(tag, {
		if(TAGS==NULL) { TAGS=xl0();TAGS->rc=INT_MAX;upsert(TAGS,xfroms("")); PF("new tags\n"); DUMP(TAGS); }
		i=_upsertidx(TAGS,s);
	});
	// PF("tagnum %s -> %d\n",name,i);
	// DUMP(TAGS);
	return i;
}
int _tagnums(const char* name) {
	int t;VP s;
	s=xfroms(name);
	t=_tagnum(s);
	xfree(s);
	return t;
}
inline void matchanyof_(const VP obj,const VP pat,const int max_match,int* n_matched,int* matchidx) {
	int i,j;VP item,rule,tmp;
	int submatches;
	int submatchidx[1024];
	tmp=xi(0);
	for(j=0;i<obj->n;j++) {
		for(i=0;i<pat->n;i++) {
			EL(tmp,int,0)=i;
			item=apply(obj,tmp);
			rule=apply(obj,tmp);
			xfree(item);
		}
	}
}

inline void match_(const VP obj_,const VP pat_,const int max_match,int* n_matched,int* matchidx) {
	int anyof=_tagnums("anyof"),exact=_tagnums("exact"),greedy=_tagnums("greedy"),start=_tagnums("start");
	int matchopt;
	int io, ip;
	int done,found;
	VP obj,pat,item=NULL,rule=NULL,iov,ipv;
	int submatches;
	int submatchidx[1024];
	int i,tag;
	int gstart=0;

	#define MATCHOPT(tag) ({ \
		if(tag != 0 && \
			 (tag==anyof||tag==exact||tag==greedy||tag==start)) { \
			PF("adopting matchopt %d %s\n", tag, tagnames(tag)); \
			matchopt = tag; \
			tag = 0; \
		} }) 

	pat=pat_; obj=obj_;
	tag=pat->tag;

	PF("match_ ////////////////\n");DUMP(obj);DUMP(pat);

	matchopt=0;
	MATCHOPT(tag);
	if(ENLISTED(pat)) {
		PF("picked up enlisted pat\n");
		pat=ELl(pat,0);
	}

	if(matchopt==exact&&obj->n!=pat->n) {
		PF("exact match sizes don't match"); goto fin;
	}


	io=0; ip=0; iov=xi(0); ipv=xi(0);
	done=0; found=0; *n_matched=0;

	/*
	if(pat->t != obj->t) {
		PF("type mismatch at top level\n"); found=0; goto fin;
	}
	*/

	if(pat->tag != obj->tag) {
		PF("type mismatch at top level\n"); found=0; goto fin;
	}
	
	if(pat->n==0) { found=1; done=1; goto done; } // empty rules are just type/tag checks
	
	if(0 &&matchopt==anyof) {
		//matchanyof_(obj,pat,max_match,n_matches,matchidx);
		//return;
	}
	if(matchopt==greedy) {
	}

	while (!done && (*n_matched) < max_match) {
		PF("match loop top. type=%s obj=%d pat=%d\n",tagnames(matchopt),io,ip);
		EL(iov,int,0)=io; EL(ipv,int,0)=ip;
		if(item)xfree(item); if(rule)xfree(rule);
		item=apply(obj, iov); rule=apply(pat, ipv); found=0;
		DUMP(item);DUMP(pat);

		if(LIST(rule)) { 
			PF("Attempting submatch..\n"); DUMP(item); DUMP(rule);
			PFIN();
			match_(item,rule,sizeof(submatchidx)/sizeof(int),&submatches,&submatchidx);
			PFOUT();
			if(submatches) {
				PF("Got submatches for obj=%d pat=%d\n",io,ip);found=1;
			}
			goto done;
		}

		if(item->t != rule->t) {
			PF("type mismatch\n"); found=0; goto done;
		}

		if(rule->tag != 0 && item->tag != rule->tag) {
			PF("tag mismatch\n"); found=0; goto done;
		}

		if(rule->n==0) { found=1; goto done; } // empty rules are just type/tag checks

		if(_equal(item,rule)) {
			PF("match_ equal!\n"); found=1; goto done;
		}

		done:
		if(found) {
			PF("Found! result #%d, pos=%d.. so far = ", *n_matched, io);
			matchidx[*n_matched]=io;
			if(PF_LVL) {
				FOR(0,(*n_matched)+1,printf("%d ",matchidx[_i]));
				printf("\n");
			}
			(*n_matched)=(*n_matched)+1;
			if(matchopt==anyof) {
				done=1;
				io++;
				ip++;
			} else if (matchopt==exact) {
				io++;
				ip++;
			} else if (matchopt==greedy) {
				io++;
				gstart=io;
				PF("advancing io and marking gstart=%d\n",gstart);
			} else if (matchopt==start) {
				io++;
				ip++;
			} else {
				io++;
				ip++;
			}
		} else {
			PF("NOT found! obj=%d pat=%d (result pos #%d)\n", io, ip, *n_matched);
			if(matchopt==anyof) {
				ip++;
				if(ip==pat->n) {
					PF("restarting anyof on next obj, resetting ip\n");
					io++;
					ip=0;
				}
			} else if (matchopt==exact) {
				goto fin;
			} else if (matchopt==greedy) {
				if(gstart){PF("popping back to %d\n",gstart);io=gstart;}
				else io++;
				gstart=0;
			} else if (matchopt==start) {
				goto fin;
			} else {
				io++;
			}
		}

		PF("match loop end.. type=%s, found=%d, obj=%d/%d, pat=%d/%d\n", tagnames(matchopt), found, io, obj->n, ip, pat->n);

		/*
		if (io == obj->n || ip == pat->n)
			done=1;
		*/
		if(ip==pat->n||io==obj->n) done=1;
	}

	fin:
	if(item)xfree(item); if(rule)xfree(rule);
	PF("done in match_ with %d matches ", *n_matched);
	if(PF_LVL) { FOR(0,(*n_matched),printf("%d ",matchidx[_i])); printf("\n"); }
	return;
}
VP match(VP obj,VP pat) {
	int n_matches=0;
	int matchidx[1024];
	VP acc;
	match_(obj,pat,sizeof(matchidx)/sizeof(int),&n_matches,&matchidx);
	acc=xisz(n_matches);
	if(n_matches)
		appendbuf(acc,(buf_t)&matchidx,n_matches);
	PF("match() obj, pat, and result:\n");
	DUMP(obj);
	DUMP(pat);
	DUMP(acc);
	return acc;
}

VP matchold(VP obj,VP pat) { // TODO should be const
	int anyof, exact, greedy;
	int i, j, matchtag, found, mtype, pati, obji;
	VP acc,tmp,oo,pp,oi,pi;
	anyof=_tagnums("anyof"); exact=_tagnums("exact"); greedy=_tagnums("greedy");

	mtype=pat->tag;
	matchtag=0;
	PF("mtype = %d %s\n",mtype,tagnames(mtype));
	PF("match %d %d\n", obj->n, pat->n);
	DUMP(obj);
	DUMP(pat);
	if(pat->tag) mtype=pat->tag;
	if(ENLISTED(pat)) { 
		pat=ELl(pat,0);
		PF("breaking out enlisted pat");
		DUMP(pat);
		if(pat->tag) {
			PF("grabbed tag");
			matchtag=pat->tag;
		}
	}
	if(mtype==exact &&
		 pat->n!=obj->n)
		return xi0();
	if(matchtag!=0 && obj->tag!=matchtag) {
		PF("tag mismatch (%d vs %d - bombing)\n", matchtag, obj->tag);
		return xi0();
	}
	pati = 0; obji = 0; pi=xi(0); oi=xi(0);
	if(mtype==anyof) {
	}
	acc = xi0();
	while(pati < pat->n && obji < obj->n) {
		EL(pi,int,0)=pati; EL(oi,int,0)=obji;
		oo=apply(obj,oi); pp=apply(pat,pi);
		found=0;tmp=0;
		PF("%s inner loop pat %d of %d, obj %d of %d\n", tagnames(mtype), pati, pat->n, obji, obj->n);
		DUMP(oo);
		DUMP(pp);
		if(!SCALAR(pp) || !SCALAR(oo)) {
			PF("doing submatch\n");
			tmp=match(oo,pp);
			if(tmp->n>0) { found=1; appendbuf(acc,(buf_t)&obji,1); goto done; }
		}
		PF("doing raw _equal\n");
		if(_equal(oo,pp)==1) {
			PF("items appear to match %d %d\n", pati, obji);
			found=1; appendbuf(acc,(buf_t)&obji,1);
		}
		done:
		if(tmp)xfree(tmp);
		PF("found = %d\n", found);
		if (found) {
			obji++;
			if (mtype==greedy) {
			} else
				pati++;
		}
		if (!found) {
			if (mtype==anyof) {
				pati++;
				if(pati==pat->n-1) {
					obji++;
					pati=0;
				}
			} else {
				if (mtype==greedy && obji==obj->n-1)
					obji=0;
				else
					obji++;
			}
		}
		xfree(oo);xfree(pp);
	}
	PF("final pati %d obji %d\n", pati, obji);
	DUMP(acc);
	return acc;
}
VP matchexec(VP obj,const VP pats) {
	int i,j;VP res,res2,sel;
	ASSERT(LIST(pats)&&pats->n%2==0,"pats should be a list of [pat1,fn1,pat2,fn2..]");
	PF("matchexec\n");
	DUMP(obj);
	DUMP(pats);
	for(i=0;i<pats->n;i+=2) {
		PF("matchexec %d\n", i);
		res=match(obj,ELl(pats,i));
		PF("matchexec match\n");
		DUMP(res);
		if(res->n) {
			res2=apply(ELl(pats,i+1),apply(obj,res));
			obj=replaceleft(obj,res->n,res2);
		}
	}		
	return obj;
}
VP eval0(VP ctx,VP code,int level) {
	int i,j; 
	VP k,v,cc,kk,vv,matchres,sel,rep;
	PF("eval0 level %d\n",level);
	DUMP(ctx);
	DUMP(code);
	ASSERT(LIST(code),"eval0 list for now");
	k=KEYS(ctx);v=VALS(ctx);
	ASSERT(k!=NULL&&v!=NULL,"eval0: empty ctx");

	// TODO eval0() should probably be implemented in terms of generic tree descent
	for(i=0;i<code->n;i++) { 
		cc=ELl(code,i);
		if (LIST(cc)) 
			eval0(ctx,cc,level+1);
		// try to match each key in ctx
		for (j=0;j<k->n;j++) {
			kk = ELl(k,j);
			PF("eval0 inner %p\n", kk);DUMP(kk);
			matchres = match(cc,kk);
			if(matchres->n==kk->n) {
				PF("got match\n");DUMP(kk);
				sel = apply(cc,matchres);
				PF("sel\n");DUMP(sel);
				vv=ELl(v,j);
				if(IS_1(vv)) 
					rep=apply(vv,ELl(sel,1));
				PF("rep\n");DUMP(rep);
				EL(code,VP,i)=rep;
			}
		}
	}
	return code;
}
VP mklexer(const char* chars, const char* label) {
	VP res = xlsz(2);
	return xln(2,
		entags(xln(1,
			entags(xln(1,entags(xfroms(chars),"anyof")),"raw")
		),"greedy"),
		mkproj(2,&labelitems,xfroms(label),0)
	);
}
VP mkint(VP x) {
	PF("mkname\n");DUMP(x);
	return entags(flatten(x),"name");
}
VP mkname(VP x) {
	PF("mkname\n");DUMP(x);
	return entags(flatten(x),"name");
}
VP mkcomment(VP x) {
	return entags(flatten(x),"comment");
}
VP mkstr(VP x) {
	return entags(flatten(x),"string");
}

// CONTEXTS:

VP mkctx() {
}
VP ctx_resolve(VP ctx) {

}
VP eval(VP code) {
	VP ctx = xd0();VP tmp;
	ASSERT(LIST(code),"eval(): code must be list");
	tmp=xl0();
	tmp=append(tmp,xt(_tagnums("til")));
	tmp=append(tmp,xi0());
	ASSERT(tmp->n==2,"eval tmp n");
	ctx=append(ctx,xln(2, tmp, x1(&til) ));
	tmp=xl0();
	tmp=append(tmp,xi0());
	tmp=append(tmp,xt(_tagnums("+")));
	tmp=append(tmp,xi0());
	ctx=append(ctx,xln(2, tmp, x2(&plus) ));
	return eval0(ctx,code,0);
}
VP evalstr(const char* str) {
	VP lex,pats,acc,t1;size_t l=strlen(str);int i;
	str=" 123";
	acc=xlsz(l);
	for(i=0;i<l;i++)
		append(acc,entags(xc(str[i]),"raw"));
	DUMP(acc);
	pats=xl0();
	/* identifiers */
	/*
	append(pats,entags(
		xln(1,
			entags(xfroms("abcdefghjijklmnoprstuvwxyz"),"anyof")
		),"greedy"));
	append(pats,x1(&mkname));
	*/
	/*
	 * lex=mklexer("abcdefghijklmnopqrstuvwxyz","name");
	append(pats,ELl(lex,0));
	append(pats,ELl(lex,1));
	xfree(lex);
	*/
	lex=mklexer(" \n\t\r","ws");
	append(pats,ELl(lex,0));
	append(pats,ELl(lex,1));
	xfree(lex);
	lex=mklexer("0123456789","int");
	append(pats,ELl(lex,0));
	append(pats,ELl(lex,1));
	xfree(lex);
	t1=matchexec(acc,pats);
	PF("evalstr result\n");
	DUMP(t1);
	exit(1);
}

// THREADING

void thr_start() {
	// TODO threading on Windows
	#ifndef THREAD
	return;
	#endif
	NTHR=0;
}
void* thr_run0(void *fun(void*)) {
	#ifndef THREAD
	return;
	#endif
	(*fun)(NULL); pthread_exit(NULL);
}
void thr_run(void *fun(void*)) {
	#ifndef THREAD
	return;
	#endif
	pthread_attr_t a; pthread_attr_init(&a); pthread_attr_setdetachstate(&a, PTHREAD_CREATE_JOINABLE);
	// nthr=sysconf(_SC_NPROCESSORS_ONLN);if(nthr<2)nthr=2;
	WITHLOCK(thr,pthread_create(&THR[NTHR++], &a, &thr_run0, fun));
}
void thr_wait() {
	#ifndef THREAD
	return;
	#endif
	void* _; int i; for(i=0;i<NTHR;i++) pthread_join(THR[i],&_);
}

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

void test_basics() {
	printf("TEST_BASICS\n");
	#include "test-basics.h"
}
void test_proj() {
	VP a,b,c,n;
	printf("TEST_PROJ\n");
	n=xi(1024*1024);
	//a=mkproj(1,&til,n,0);
	a=x1(&til);
	b=apply(a,n);
	PF("b\n");DUMP(b);
	c=apply(mkproj(1,&sum,b,0),0);
	PF("result\n");DUMP(c);
	printf("%lld\n", AS_o(c,0));
	xfree(a);xfree(b);xfree(c);xfree(n);
	//DUMP(c);
}
void test_proj_thr0(void* _) {
	VP a,b,c,n; int i;
	for (i=0;i<1024;i++) {
		printf("TEST_PROJ %d\n", pthread_self());
		n=xi(1024*1024);
		//a=mkproj(1,&til,n,0);
		a=x1(&til);
		b=apply(a,n);
		PF("b\n");DUMP(b);
		c=apply(mkproj(1,&sum,b,0),0);
		PF("result\n");DUMP(c);
		printf("%lld\n", AS_o(c,0));
		xfree(a);xfree(b);xfree(c);xfree(n);
	}
	return NULL;
}
void test_proj_thr() {
	int n = 2, i; void* status;
	/*
	pthread_attr_t a;
	pthread_t thr[n];
	pthread_attr_init(&a);
	pthread_attr_setdetachstate(&a, PTHREAD_CREATE_JOINABLE);
	for(i=0;i<n;i++) {
		pthread_create(&thr[i], &a, test_proj_thr0, NULL);
	}
	for(i=0; i<n; i++) {
		pthread_join(&thr[i], &status);
	}
	*/
	thr_start();
	for(i=0;i<n;i++) thr_run(test_proj_thr0);
	thr_wait();
}
void test_context() {
	VP a,b,c;
	printf("TEST_CONTEXT\n");
	a=xd0();
	assigns(a,"til",x1(&til));
	assign(a,tagv("greedy",xln(3,xfroms("\""),xc0(),xfroms("\""))),x1(&mkstr));
	b=xxn(7,xfroms("\""),xfroms("a"),xfroms("b"),xfroms("c"),xfroms("\""),xfroms("til"),xfroms("1024"));
	c=apply(a,b);
	DUMP(a);
	DUMP(b);
}
void test_match() {
	#include"test-match.h"
}
void test_eval() {
	/*
	VP code,tmp1,tmp2,tmp3;
	PF("test_eval\n");
	code=entags(xl0(),"code");
	ASSERT(code->tag==_tagnums("code"),"entags 1");
	tmp1=xln(2,
		xt(_tagnums("til")),
		xi(100)
	);
	tmp1->tag=_tagnums("value");
	append(code,tmp1);
	eval(code);
	*/
	evalstr("til 1024");
}
void test_json() {
	VP mask, jsrc, res; char str[256]={0};
	I8 cc[256] = {0};
	#define CC(n,ch_) ({ const char* x=ch_;FOR(0,strlen(x),cc[x[_i]]=n); })
	CC('a',"abcdefghjijklmnopqrstuvwxyz");
	CC('n',"0123456789");
	CC('[',"[");CC(']',"]");CC('{',"{");CC('}',"}");
	CC('c',",");CC('q',"\"");
	FOR(0,256,printf("%d,",cc[_i]));
	mask=xcsz(256);
	appendbuf(mask,(buf_t)&cc,256);
	DUMP(mask);
	strncpy(str,"[[\"abc\",5,[\"def\"],6,[7,[8,9]]]]",256);
	jsrc=xfroms(str);
	DUMP(jsrc);
	PFW({
	res=apply(mask,cast(jsrc,Tt(byte)));
	});
	DUMP(res);
}
void tests() {
	int i;
	test_basics();
	net();
	exit(1);
	test_json();
	test_match();
	test_proj_thr();
	if(MEM_W) {
		PF("alloced = %llu, freed = %llu\n", MEM_ALLOC_SZ, MEM_FREED_SZ);
	}
	exit(1);
}
int main(void) {
	VP code;
	tests();
}
/*
*/ 
