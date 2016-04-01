// XXL - a minimalistic programming language
// (C) Copyright 2016 Thomas Lackner; 3 clause BSD-L

#include "compile.h"                   // make xxl aware of its own compilation settings
#include "def.h"                       // helper macros and main type defs
#include "proto.h"                     // prototypes
#include "accessors.h"
#include "vary.h"

// GLOBALS (dangggerous)

// commonly used "static" values, set in init()
VP TTPARENT=NULL, XB0=NULL,XB1=NULL,XI0=NULL,XI1=NULL;
tag_t TIEXCEPTION=0, TINULL=0;
THREADLOCAL VP XXL_SYS=NULL, XXL_CUR_CTX=NULL; 

I8 PF_ON=0; I8 XRAY_LVL=0;               // controls debugging output on/off/nesting depth
THREADLOCAL I8 IN_OUTPUT_HANDLER=0;       // used to prevent some debugging info while debugging

#define N_RETAINS 30
#define RETAIN_MAX 1024
static VP MEM_RETAIN[N_RETAINS] = {0}; // retain buffer to reuse VP pointers we recently freed

I8 MEM_WATCH=0;                        // monitor/report on memory use (see 'memwatch' in repl)
#define N_MEM_PTRS 1024
static VP MEM_PTRS[N_MEM_PTRS]={0};
static I32 MEM_ALLOC_SZ=0,MEM_FREED_SZ=0;
static I32 MEM_ALLOCS=0, MEM_REALLOCS=0, MEM_FREES=0, MEM_RETAINED=0;

#ifdef THREAD                          // in threaded scenarios we carefully lock some actions
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

// REPRESENTATION

MEMO_make(REPR_SEEN);
char* repr0(VP x,char* s,size_t sz) {
	type_info_t t; int i;
	// XRAY_log("repr0\n");
	if(x==NULL) { FMT_into_s(sz,"null",0); return s; }
	if(x->t < 0 || x->t > MAX_TYPE) { FMT_into_s(sz,"/*unknown*/",0); return s; }
	if(!SIMPLE(x)) {
		VP existing=NULL;
		MEMO_check(REPR_SEEN, x, ({ existing=memo_val; }), i);
		if(existing!=NULL) {
			s=(char*)existing;
			// printf("cycle %p found after %d iters\n", x, i);
			FMT_into_s(sz,"..cycle%p..",x);
			return s;
		}
		MEMO_set(REPR_SEEN,x,(VP)s,i);
	}
	t=typeinfo(x->t);
	IN_OUTPUT_HANDLER++;
	if(x->tag!=0) FMT_into_s(sz, "'%s#", tagnames(x->tag));
	if(t.repr) (*(t.repr)(x,s,sz));
	//FMT_into_s(sz,"(r%d)",x->rc);
	IN_OUTPUT_HANDLER--;
	return s;
}
char* reprA(VP x) {
	MEMO_clear(REPR_SEEN);
	#define BS 1024*65
	char* s = calloc(1,BS);
	s = repr0(x,s,BS-1);
	//FMT_into_s(BS,"\n",0);
	return s;
}
VP repr(VP x) {
	char* s = reprA(x);
	return xfroms(s);
}
char* repr_c(VP x,char* s,size_t sz) {
	int i=0,n=x->n,ch;
	FMT_into_s(sz,"\"",0);
	for(;i<n;i++){
		ch = AS_c(x,i);
		if(ch=='"') FMT_into_s(sz,"\\\"", 0);
		else if(ch=='\n') FMT_into_s(sz,"\\n", 0);
		else if(ch=='\r') FMT_into_s(sz,"\\r", 0);
		else FMT_into_s(sz,"%c",ch);
		// repr0(*(EL(x,VP*,i)),s,sz);
	}
	FMT_into_s(sz,"\"",0);
	return s;
}
char* repr_d(VP x,char* s,size_t sz) {
	int i, n;
	VP k=KEYS(x),v=VALS(x);
	// if(!sz) return s;
	if (!k || !v) { FMT_into_s(sz,"[null]",0); return s; }
	FMT_into_s(sz,"[",0);
	n=k->n;
	if(n==0) {
		FMT_into_s(sz,":",0);
	} else {
		for(i=0;i<n;i++) {
			repr0(DICT_key_n(x,i), s, sz-1);
			FMT_into_s(sz,":",0);
			repr0(DICT_val_n(x,i), s, sz-2);
			if(i!=n-1)
				FMT_into_s(sz,", ",0);
		}
	}
	FMT_into_s(sz,"]",0);
	return s;
}
char* repr_a(VP x,char* s,size_t sz) { // table
	int i, j, kn, vn;
	VP k=KEYS(x), v=VALS(x), tmp, tmp2;
	// if(!sz) return s;
	if (!k || !v) { FMT_into_s(sz,"[null]",0); return s; }
	kn=k->n; vn=LEN(v) ? ELl(v,0)->n : 0;
	repr0(k, s, sz-1);
	FMT_into_s(sz,":[\n",0);
	for(i=0; i<vn; i++) {
		FMT_into_s(sz,"[",i);
		for(j=0; j<kn; j++) {
			tmp=apply_simple_(ELl(v,j),i);
			repr0(tmp, s, sz-2);
			if(j!=kn-1) FMT_into_s(sz,", ",0);
			xfree(tmp);
		}
		if(i!=vn-1) FMT_into_s(sz,"],\n",0);
	}
	FMT_into_s(sz,"]]",0);
	return s;
}
char* repr_l(VP x,char* s,size_t sz) {
	int i=0, n=x->n;VP a;
	FMT_into_s(sz,"[",0);
	for(i=0;i<n;i++){
		if(REPR_MAX_ITEMS && i==(REPR_MAX_ITEMS/2)) {
			FMT_into_s(sz,".. (%d omitted) ..", n-REPR_MAX_ITEMS);
			i+=REPR_MAX_ITEMS;
			continue;
		}
		a = ELl(x,i);
		if (a==NULL) FMT_into_s(sz,"null",0); 
		else repr0(a,s,sz);
		if(i!=n-1)
			FMT_into_s(sz,", ",0);
	}
	FMT_into_s(sz,"]",0);
	return s;
}
char* repr_o(VP x,char* s,size_t sz) {
	int i=0,n=x->n;tag_t tag;
	if(n>1) FMT_into_s(sz,"(",0);
	for(;i<n;i++){
		char* buf=sfromxA(numelem2base(x,i,10));
		FMT_into_s(sz,"%s",buf);
		free(buf);
		if(i!=n-1)
			FMT_into_s(sz,",",0);
		// repr0(*(EL(x,VP*,i)),s,sz);
	}
	if(n>1) FMT_into_s(sz,")",0);
	return s;
}
char* repr_p(VP x,char* s,size_t sz) {
	Proj p = EL(x,Proj,0);
	FMT_into_s(sz,"'projection#[%p,%d,",x,p.type);
	if(p.type<0) {
		repr0(p.ctx, s, sz);
		FMT_into_s(sz,",",0);
	}
	if(p.left!=NULL) 
		repr0(p.left, s, sz);
	else
		FMT_into_s(sz,"()",0);
	FMT_into_s(sz,",",0);
	if(p.right!=NULL) 
		repr0(p.right, s, sz);
	else
		FMT_into_s(sz,"()",0);
	FMT_into_s(sz,"]",0);
	return s;
}
char* repr_t(VP x,char* s,size_t sz) {
	int i=0,n=x->n;tag_t tag;
	if(n>1) FMT_into_s(sz,"(",0);
	for(;i<n;i++){
		tag = AS_t(x,i);
		FMT_into_s(sz,"'%s",tagnames(tag));
		if(i!=n-1)
			FMT_into_s(sz,",",0);
		// repr0(*(EL(x,VP*,i)),s,sz);
	}
	if(n>1) FMT_into_s(sz,")",0);
	return s;
}
char* repr_xlambda(VP keys,VP vals,char* s,size_t sz) {
	VP k, item, kk, vv;
	int i, kn;
	FMT_into_s(sz,"{",0);
	if (keys) {
		k=keys; kn=LEN(KEYS(k));
		for(i=0; i<kn; i++) {
			kk=apply_simple_(KEYS(k),i);
			vv=apply_simple_(VALS(k),i);
			if(!IS_EXC(kk) && !IS_EXC(vv)) {
				if (kk==TTPARENT) {
					FMT_into_s(sz,"'parent is \"%p\";",vv);
				} else {
					repr0(kk,s,sz);
					FMT_into_s(sz," is ",0);
					repr0(vv,s,sz);
					FMT_into_s(sz,"; ",0);
				}
				xfree(kk); xfree(vv);
			}
		}
	}
	if (vals && LIST(vals) && LEN(vals)==2) {
		k=ELl(vals,0); kn=LEN(k);
		for(i=0; i<kn; i++) {
			item=ELl(k,i);
			if(TAGGED(item,Ti(lambda))) s=repr_xlambda(NULL,item,s,sz);
			else repr0(item,s,sz);
			if(kn-1!=i) FMT_into_s(sz,",",0);
		};
	}
	FMT_into_s(sz,"}",0);
	return s;
}
char* repr_x(VP x,char* s,size_t sz) {
	FMT_into_s(sz,"'context#%p",x);
	if(x->n==2) {
		s=repr_xlambda(KEYS(x), VALS(x),s,sz);
	}
	return s;
}
#include "repr.h"
#include "types.h"

// LOW LEVEL

static inline type_info_t typeinfo(const type_t n) { 
	if(n <= MAX_TYPE) return TYPES[n];
	else return (type_info_t){0}; 
}
static inline type_info_t typechar(const char c) { 
	ITER(TYPES,sizeof(TYPES),{ IF_RET(_x.c==c,_x); }); 
	return (type_info_t){0}; }

VP xalloc(const type_t t,const I32 initn) {
	VP a; int g,i,itemsz,sz; 
	int finaln = initn < 4 ? 4 : initn;
	itemsz = typeinfo(t).sz; sz=itemsz*finaln;
	//XRAY_log("%d\n",sz);
	a=NULL;g=0;
	if (sz < RETAIN_MAX && N_RETAINS > 0) {
		WITHLOCK(mem, {
			FOR(0,N_RETAINS,({
				if(MEM_RETAIN[_i]!=0 && 
					 ((VP)MEM_RETAIN[_i])->sz > sz) { // TODO xalloc gobbler should bracket sizes
					a=MEM_RETAIN[_i];
					MEM_RETAIN[_i]=0;
					MEM_RETAINED++;
					g=_i;
					XRAY_log("xalloc recycling retained %p\n",a);
					// XRAY_emit(a);
					memset(a,0,sizeof(struct V)+a->sz);
					break;
				}
			}));
		});
	} 
	if(a==NULL) {
		a = calloc(sizeof(struct V)+sz,1);
		a->sz=sz; 
	}
	// note that we're careful not to set sz here, because if we are reclaiming
	// a pointer from the retain pool, it will already have its own sz (which we know is
	// better than what we need). we will however set cap, based on that sz/itemsz
	// also note that we're setting alloc, tag and n to 0 for clarity, but they should have
	// already been zeroed by calloc or memset above
	a->alloc=0; a->t=t; a->tag=0; a->n=0; a->rc=1; a->itemsz=itemsz;
	a->cap=a->sz / itemsz;
	if (MEM_WATCH) {
		WITHLOCK(mem, {
			XRAY_memlog("%salloc %d %p %d (%d * %d) (total=%d, freed=%d, bal=%d)\n",(g==1?"GOBBLED! ":""),t,a,sizeof(struct V)+sz,finaln,itemsz,MEM_ALLOC_SZ,MEM_FREED_SZ,MEM_ALLOC_SZ-MEM_FREED_SZ);
			MEM_ALLOC_SZ += sizeof(struct V)+sz;
			MEM_ALLOCS++;
			for(i=0;i<N_MEM_PTRS;i++) {
				if (MEM_PTRS[i]==0)
					MEM_PTRS[i]=a;
			}
		});
	}
	return a;
}
VP xprofile_start() {
	MEM_WATCH=1;
	return xl0();
}
VP xprofile_end() {
	int i;
	VP ctx;
	VP res; 
	MEM_WATCH=0;
	printf("allocs: %d (%d), gobbles: %d, reallocs: %d, frees: %d\n", MEM_ALLOC_SZ, MEM_ALLOCS, MEM_RETAINED, MEM_REALLOCS, MEM_FREES);
	for(i=0;i<N_MEM_PTRS;i++)
		if(MEM_PTRS[i]!=0) {
			printf("freeing mem ptr\n");
			xfree(MEM_PTRS[i]);
			MEM_PTRS[i]=0;
		}
	return xl0();
}
VP xrealloc(VP x,I32 newn) {
	// XRAY_log("xrealloc %p t=%d isz=%d %d/%d n=%d a=%d\n",x,x->t,x->itemsz,newn,x->cap,x->n,x->alloc);
	if(newn>=x->cap) {
		buf_t newp; I32 newsz;
		newn = (newn < 10*1024) ? newn * 4 : newn * 1.25; // TODO there must be research about realloc bins no?
		newsz = newn * x->itemsz;
		if(x->alloc && x->dyn) {
			// XRAY_log("realloc %p %d %d %d\n", x->dyn, x->sz, newn, newsz);
			newp = realloc(x->dyn, newsz);
		} else {
			// XRAY_log("calloc sz=%d, n=%d, newn=%d, newsz=%d\n", x->sz, x->n, newn, newsz);
			newp = calloc(newsz,1);
			memmove(newp,BUF(x),x->sz);
		}
		if(MEM_WATCH) {
			// XRAY_memlog("realloc %d %p -> %d\n", x->t, x, newsz);
			MEM_ALLOC_SZ += newsz;
			MEM_REALLOCS++;
		}
		// XRAY_log("realloc new ptr = %p\n", newp);
		if(newp==NULL) PERR("realloc");
		x->dyn=newp;
		x->cap=newn;
		x->sz=newsz;
		x->alloc=1;
		// XRAY_log("post realloc\n"); XRAY_emit(x);
	}
	return x;
}
VP xfree(VP x) {
	int i;
	if(UNLIKELY(x==NULL)) return x;
	//XRAY_log("XFREE (%p)\n",x);//XRAY_emit(x);//XRAY_emit(info(x));
	x->rc--; 
	if(LIKELY(x->rc==0)) {
		if(CONTAINER(x)) {
			ITERV(x,xfree(ELl(x,_i)));
			x->n=0;
		}
		if(MEM_WATCH) {
			MEM_FREED_SZ+=sizeof(struct V) + x->sz;
			MEM_FREES+=1;
			XRAY_memlog("free %d %p %d (%d * %d) (total=%d, freed=%d, bal=%d)\n",x->t,x,x->sz,x->itemsz,x->cap,MEM_ALLOC_SZ,MEM_FREED_SZ,MEM_ALLOC_SZ-MEM_FREED_SZ);
		}
		if (x->alloc==0 && x->sz < RETAIN_MAX && N_RETAINS > 0) {
			for(i=0;i<N_RETAINS;i++)
				if(MEM_RETAIN[i]==x || MEM_RETAIN[i]==0) {
					MEM_RETAIN[i]=x;
					return x;
				}
		}
		XRAY_log("xfree(%p) really dropping type=%d n=%d alloc=%d\n",x,x->t,x->n,x->alloc);
		// XRAY_emit(x);
		if(x->alloc && x->dyn) free(x->dyn);
		free(x);
	} return x; }
VP xref(VP x) { if(!x) return x; if(MEM_WATCH){XRAY_memlog("ref %p\n",x);} x->rc++; return x; }
VP xreplace(VP x,VP newval) {
	XRAY_log("xreplace\n");XRAY_emit(x);XRAY_emit(newval);
	int oldrc=x->rc;
	if(x->alloc && x->dyn) free(x->dyn);
	memmove(x,newval,sizeof(struct V));
	x->dyn=calloc(newval->sz,1);
	memmove(x->dyn,BUF(newval),newval->sz);
	x->alloc=1;
	x->rc=newval->rc+x->rc; // arb

	return x;
}
VP xfroms(const char* str) {  
	// character vector from C string. allocates new VP. free it when done.
	// NB. returned VP does not contain the final \0 
	// (i.e., its result->n = strlen(str)-1)
	size_t len = strlen(str); type_info_t t = typechar('c');
	VP a = xalloc(t.t,len); memcpy(BUF(a),str,len); a->n=len; return a; }
const char* bfromx(VP x) {
	// pointer to bytes of x. does NOT allocate - do not free returned value. NOT necessarily null terminated.
	if(x==NULL)return "null";
	return (char*)BUF(x); }
char* sfromxA(VP x) {
	// C-style string from bytes of x. DOES allocate string buffer. null terminated. free after use.
	if(x==NULL)return "null";
	char* buf=calloc(1,LEN(x)+1);
	memcpy(buf,BUF(x),LEN(x));
	return buf; }
static inline int _equalm(const VP x,const int xi,const VP y,const int yi) {
	if(x==NULL||y==NULL) return 0;
	// XRAY_log("comparing %p to %p\n", ELi(x,xi), ELi(y,yi));
	// XRAY_log("_equalm\n"); XRAY_emit(x); XRAY_emit(y);
	if(ENLISTED(x)) { XRAY_log("equalm descend x");
		return _equalm(ELl(x,xi),0,y,yi);
	}
	if(ENLISTED(y)) { XRAY_log("equalm descend y");
		return _equalm(x,xi,ELl(y,yi),0);
	}
	if(memcmp(ELi(x,xi),ELi(y,yi),x->itemsz)==0) return 1;
	else return 0;
}	
int _equal_container(const VP x,const VP y) {
	ITERV(x,{ IF_RET(_equal(ELl(x,_i),ELl(y,_i))==0, 0); }); 
	return 1; 
}
int _equal(const VP x,const VP y) {
	// this is the most common call in the code
	// TODO _equal() needs to handle comparison tolerance and type conversion
	// TODO _equal should use the new VARY_*() macros, except for general lists
	// XRAY_log("_equal\n"); XRAY_emit(x); XRAY_emit(y);
	// if the list is a container for one item, we probably want to match the inner one
	if(x==NULL||y==NULL) return 0;
	VP a=x,b=y;
	if(LIST(a) && SCALAR(a)) a=ELl(a,0);
	if(LIST(b) && SCALAR(b)) b=ELl(b,0);
	IF_RET(a->n != b->n, 0);
	if(CONTAINER(a) && CONTAINER(b)) return _equal_container(a,b);
	if(a->t == b->t) {
		return memcmp(BUF(a),BUF(b),a->itemsz*a->n)==0;
	} else if (COMPARABLE(a)&&COMPARABLE(b)) {
		int typerr=-1;
		VARY_EACHBOTH(a,b,({ if (_x != _y) return 0; }),typerr);
		if(typerr>-1) return 0;
		else return 1;
	} else
		return 0;
}
VP equal(const VP x,const VP y) {
	return xb(_equal(x,y));
}
MEMO_make(CLONE);
VP clone0(const VP obj) {
	if(IS_EXC(obj)) return obj;
	XRAY_log("clone0 %p\n",obj);XRAY_emit(obj);
	int i, objn=obj->n; 
	MEMO_check(CLONE,obj,({ XRAY_log("found memoized %p\n", memo_val); return xref(memo_val); }),i);
	if(i==MEMO_sz) { return EXC(Tt(stack),__func__,0,0); }
	VP res=ALLOC_LIKE(obj);
	MEMO_set(CLONE,obj,res,i);
	if(objn) {
		if(CONTAINER(obj)) {
			res->n=objn;
			for(i=0;i<objn;i++) {
				VP elem=LIST_item(obj,i);
				if(elem==0) { res->n--; continue; };
				if(IS_t(elem) || IS_1(elem) || IS_2(elem)) EL(res,VP,i)=xref(elem);
				else EL(res,VP,i)=clone0(elem);
			}
		} else 
			res=appendbuf(res,BUF(obj),objn);
	}
	return res;
}
VP clone(const VP obj) { 
	// TODO keep a counter of clone events for performance reasons - these represent a concrete
	// loss over mutable systems
	XRAY_log("clone\n");XRAY_emit(obj);
	MEMO_clear(CLONE);
	VP res=clone0(obj);
	XRAY_log("clone returning %p\n",res);XRAY_emit(res);
	return res;
}
VP wherepred_(const VP x,const VP y,int removematches) {
	VP where=matchany(x,y);
	if(IS_EXC(where)) return where;
	if(LIST(where)) where=list2vec(where);
	VP invw;
	if(removematches) invw=not(where);
	else invw=where;
	VP wherec=condense(invw);
	if(IS_EXC(wherec)) return wherec;
	XRAY_log("except indices to apply for result\n");XRAY_emit(wherec);
	VP res=apply(x,wherec);
	xfree(wherec);
	xfree(where);
	if(removematches) xfree(invw); 
	if(TABLE(x) && DICT(res)) { 
		// tables when indexed with one int, like x@0, return
		// a single row as a dict. For our purposes, we want that
		// to remain a list.
		VP k=KEYS(res), v=xl(VALS(res)), t=dict(k,v);
		xfree(res); xfree(v);
		return t;
	} else 
		return res;
}

// RUNTIME 

inline VP appendbuf(VP x,const buf_t buf,const size_t nelem) {
	int newn;buf_t dest;
	//XRAY_log("appendbuf %d\n", nelem);XRAY_emit(x);
	newn=x->n+nelem;
	x=XREALLOC(x,newn);
	// XRAY_log("after realloc"); XRAY_emit(x);
	dest=ELi(x,x->n);
	memmove(dest,buf,x->itemsz * nelem);
	x->n=newn;
	// XRAY_log("appendbuf newn %d\n", newn); XRAY_emitRAW(dest, x->itemsz * newn);
	return x;
}
VP append(VP x,VP y) { 
	// append all items of y to x. if x is a general list, append pointer to y, and increase refcount.
	// XRAY_log("append %p %p\n",x,y); XRAY_emit(x); XRAY_emit(y);
	if(IS_EXC(x)) return x;
	if(!CONTAINER(x) && !(x->t==y->t)) { return EXC(Tt(type),"append x must be container or types must match", x, y); }
	if(TABLE(x)) return catenate_table(x,y);
	if(IS_x(x)) {
		if(VALS(x)) xfree(VALS(x)); VALS(x)=y; return x;
	}
	if(IS_d(x)) {
		// XRAY_log("append d %p\n", x); XRAY_emit(x);
		ASSERT(y->n % 2 == 0, "append to a dict with ['key;value]");
		VP k=KEYS(x),v=VALS(x),y1,y2; int i;
		y1=ELl(y,0);
		y2=ELl(y,1);
		if(k==NULL) {                      // create dict
			if(0 && SCALAR(y1)) k=ALLOC_LIKE_SZ(y1, 4);
			else k=xl0();
			v=xl0(); xref(k); xref(v); 
			EL(x,VP,0)=k; EL(x,VP,1)=v;
			x->n=2; i=-1;                    // not found so insert below
		} else i=_find1(k,y1);
		if(i==-1) {
			xref(y1); xref(y2); EL(x,VP,0)=append(k,y1); EL(x,VP,1)=append(v,y2);
		} else {
			xref(y2); ELl(v,i)=y2;
		}
		return x;
	}
	if(CONTAINER(x)) { 
		xref(y);
		x=XREALLOC(x,x->n+1); 
		EL(x,VP,x->n)=y; x->n++;
	} else {
		// XRAY_log("append disaster xn %d xc %d xi %d xsz %d yn %d yc %d yi %d ysz %d\n",x->n,x->cap,x->itemsz,x->sz,y->n,y->cap,y->itemsz,y->sz);
		// XRAY_emit(info(x));XRAY_emit(x);XRAY_emit(info(y));XRAY_emit(y);
		// dest = BUF(x) + (x->n*x->itemsz);
		x=XREALLOC(x,x->n + y->n);
		memmove(ELsz(x,x->itemsz,x->n),BUF(y),y->n*y->itemsz);
		x->n+=y->n;
	}
	return x;
}
VP appendfree(VP x,VP y) {
	append(x,y); xfree(y); return x;
}
VP upsert(VP x,VP y) {
	if(_find1(x,y)==-1) append(x,y); return x;
}
int _upsertidx(VP x,VP y) {
	int idx = _find1(x,y);
	if(idx>-1) return idx;
	append(x,y); return x->n-1;
}
VP amend_dict_dict(VP x,VP y) {
	// NB. amend supports callbacks as values but we
	// arent going to do that here - possibly needs rethink
	VP k=KEYS(y), v=VALS(y);
	int i=0, kn=k->n, vn=v->n;
	for(; i<kn; i++) x=assign(x,ELl(k,i),ELl(v,i));
	return x;
}
VP amend(VP x,VP y) {
	XRAY_log("amend\n");XRAY_emit(x);XRAY_emit(y);
	// TODO amend should create a new structure rather than modifying one in-place
	if(!SIMPLE(x) && !CONTAINER(x)) return EXC(Tt(type),"amend x must be a simple or container type",x,y);
	if(DICT(x) && DICT(y)) return amend_dict_dict(x,y);
	if(!CONTAINER(y) || y->n!=2) return EXC(Tt(type),"amend y should be [indices,replacements]",x,y);
	VP idx=ELl(y,0), val=ELl(y,1), acc=ALLOC_LIKE_SZ(x, idx->n), idxv, tmp=0; int i,fail=-1;
	if(CALLABLE(idx)) { 
		idx=matcheasy(x,idx);
		RETURN_IF_EXC(idx);
		idx=condense(idx);
	}
	for(i=0;i<idx->n;i++) { 
		idxv=apply_simple_(idx,i); // TODO really need a fast 0 alloc version of apply(simple,int)!
		if(UNLIKELY(CALLABLE(val))) {
			XRAY_log("amend calling val cb\n");XRAY_emit(val);
			XRAY_in();
			tmp=apply(val,apply(x,idxv));
			XRAY_out();
		} else {
			// handle the case of assigning one vector (ie a string) to a given index
			// or the case of assigning many indices to one value
			if (SCALAR(idx) || SCALAR(val)) tmp=xref(val); 
			else { XRAY_in(); tmp=apply_simple_(val,i); XRAY_out(); }
		}
		RETURN_IF_EXC(tmp);
		if(IS_EXC(tmp)) { xfree(idxv); return tmp; }
		if(UNLIKELY(!CONTAINER(x) && tmp->t!=x->t)) {
			if(NUM(x) && NUM(tmp)) tmp=make(tmp,x);
			else return EXC(Tt(value),"amend value type does not match x",x,tmp);
		}
		assign(x,idxv,tmp);
		xfree(idxv); xfree(tmp);
	}
	XRAY_log("amend returning\n");XRAY_emit(x);
	return x;
}
VP assign_table(VP x,VP k,VP val) {
	if(NUMSTRICT(k)) return EXC(Tt(nyi),"table numeric assign not yet implemented",x,k);
	XRAY_log("assign_table\n");XRAY_emit(x);XRAY_emit(k);XRAY_emit(val);
	int i=TABLE_col_num_for_name(x,k);
	int nr=TABLE_nrows(x);
	if(LEN(val)!=nr) return EXC(Tt(value),"table assign value length doesnt match table rows",x,val);
	VP oldcol=TABLE_col(x,i);
	TABLE_col(x,i)=clone(val);
	xfree(oldcol);
	return x;
}
inline VP assign(VP x,VP k,VP val) {
	// XRAY_log("assign\n");XRAY_emit(x);XRAY_emit(k);XRAY_emit(val);
	if (LIST(k) && k->n) {
		int i=0;VP res=x;
		if(IS_t(LIST_item(k,0)) && AS_t(LIST_item(k,0),0)==TINULL) {
			// .name - try to find a matching key somewhere up the stack
			// and update it with xreplace
			XRAY_log("assign uplevel\n",i);
			VP newk=LIST_item(k,1), tmp=resolvekey(x,newk,1);
			XRAY_emit(tmp);
			if(!IS_EXC(tmp)) return xreplace(tmp,val);
			else { return EXC(Tt(assign),"assign could not set uplevel",x,k); }
		}
		for(;i<k->n-1;i++) {
			// XRAY_log("assign at depth %d\n",i);
			ARG_MUTATING(x);
			res=apply(res,ELl(k,i));
			XRAY_emit(res);
			if(UNLIKELY(IS_EXC(res)) || res->n==0)
				return res;
		}
		// XRAY_log("assign at depth setting\n");
		assign(res,ELl(k,k->n-1),val);
		return res; // TODO: should this return the inner context affected? seems wrong
	}
	if(TABLE(x)) return assign_table(x,k,val);
	else if(DICT(x)) {
		xref(k); xref(val); return append(x,xln(2,k,val));
	} else if(SIMPLE(x) && NUM(k)) {
		// XRAY_log("assign");XRAY_emit(x);XRAY_emit(k);XRAY_emit(val);
		if(val->n == 0) return EXC(Tt(value),"assignment with empty values not supported on simple types",x,val);
		if(x->t != val->t) return EXC(Tt(type),"assign value and target types don't match",x,val);
		int typerr=-1, i=NUM_val(k);
		ARG_MUTATING(x);
		if (i>=x->n) { XREALLOC(x,i+1); x->n=i+1; } 
		VARY_EACHRIGHT_NOFLOAT(x,k,({
			EL(x,typeof(_x),_y) = EL(val,typeof(_x),_y%val->n); // TODO assign should create new return value
		}),typerr);
		// XRAY_log("assign num returning");XRAY_emit(x);
		return x;
	} else if(LIST(x) && NUM(k)) {
		int i=NUM_val(k);
		ARG_MUTATING(x);
		if(i>=x->n) { XREALLOC(x,i+1); x->n=i+1; }
		if(i<x->n && ELl(x,i)) xfree(ELl(x,i)); // if we're assigning over something that exists in a container, free it
		xref(val);
		EL(x,VP,i)=val;
		return x;
	}
	return EXC(Tt(type),"assign: bad types",x,0);
}
static inline VP assigns(VP x,const char* key,VP val) {
	return assign(x,xfroms(key),val);
}
VP behead(const VP x) {
	// XRAY_log("behead\n");XRAY_emit(x);
	return drop_(x,1);
}
VP from(const VP x,const VP y) {
	if(IS_EXC(x) || IS_EXC(y)) return EXC(Tt(value),"from can't use those values",x,y);
	if(CALLABLE(x)) return wherepred_(y,x,0);
	if(CALLABLE(y) && _arity(y)==2 && LIST(x) && LEN(x)==2) return apply2(y,ELl(x,0),ELl(x,1));
	return apply(y,x);
}
VP catenate_table(VP table, VP row) {
	XRAY_log("catenate_table\n"); XRAY_emit(table); XRAY_emit(row);
	int trows, tcols;
	if(table==NULL || LEN(table)==0 || LEN(KEYS(table))==0) {
		XRAY_log("table seems to be blank; creating table from this row");
		if(DICT(row)) return make_table(MUTATE_CLONE(KEYS(row)),MUTATE_CLONE(VALS(row)));
		else return EXC(Tt(value),"can't catenate an empty table with that",table,row);
	} else {
		trows=TABLE_nrows(table); tcols=TABLE_ncols(table);
	}
	if(TABLE(row)) {
		VP tmp; int i; 
		XRAY_log("catenating table\n");
		for(i=0; i<trows; i++) {
			tmp=table_row_dict_(row, i);
			ARG_MUTATING(table);
			table=catenate_table(MUTATE_CLONE(table),tmp);
			xfree(tmp);
		}
		return table;
	}
	if(DICT(row)) {
		VP rk=KEYS(row);
		ASSERT(LIST(KEYS(row))&&LIST(VALS(row)),"catenate_table: row keys or vals not list");
		VP fullrow;
		if(tcols != 0 && !_equal(rk,KEYS(table))) { // mismatching columns on existing table!
			XRAY_log("row keys/table keys mismatch, loading old row defaults\n");
			VP lastrow=table_row_dict_(table,trows-1);
			fullrow=catenate(lastrow,row);
			XRAY_emit(fullrow);
		} else fullrow=MUTATE_CLONE(row);
		rk=KEYS(fullrow);
		VP rv=VALS(fullrow), rktmp, rvtmp, vec; int i=0;
		for(; i<rk->n; i++) {
			rktmp=ELl(rk,i); rvtmp=ELl(rv,i);
			XRAY_log("catenate col\n");
			XRAY_emit(rktmp);
			XRAY_emit(rvtmp);
			XRAY_emit(KEYS(table));
			XRAY_emit(VALS(table));
			XRAY_emit(table);
			vec=TABLE_col_named(table,rktmp);
			XRAY_log("vec %p\n",vec);XRAY_emit(vec);
			ARG_MUTATING(table);
			if(vec==NULL) {
				// NB. unknown columns are added, but old rows
				// will get the same value as this one!
				KEYS(table)=append(KEYS(table),rktmp);
				VALS(table)=append(VALS(table),take_(rvtmp,trows+1));
				XRAY_log("new keys and vals:\n");
				XRAY_emit(KEYS(table));
				XRAY_emit(VALS(table));
			} else {
				vec=append(vec,rvtmp);
				XRAY_log("new vec\n");
				XRAY_emit(vec);
			}
		}
		return table;
	} else if (LIST(row)) {
		int i=0;
		row=MUTATE_CLONE(row);
		table=MUTATE_CLONE(table);
		// this could either be a single row, i.e., a list comprised of simple types,
		// or a list of lists, meaning a list of individual lists of simple types. we'll
		// check if the first member is a list to determine which we think it is.
		if(LIST_of_lists(row) && LEN(LIST_first(row)) == tcols) {
			XRAY_log("LOL\n");XRAY_emit(row);
			// TODO probably important not to recurse here
			for(; i<row->n; i++) table=catenate_table(table,MUTATE_CLONE(LIST_item(row,i)));
			return table;
		} else {
			VP colval;
			if(row->n != tcols) return EXC(Tt(value),"table definition doesn't match list",table,row);
			if(LEN(VALS(table))==0) {        // we have not yet setup the values records
				for(i=0; i<LEN(row); i++) {
					colval=LIST_item(row,i);
					xref(colval);
					// if any of the values of this dictionary are not scalar, we'll need
					// to make that column into a general list, or indexing row values will
					// become very confusing indeed
					if(!SCALAR(colval)) EL(row,VP,i)=xl(colval);
					// EL(row,VP,i)=xl(colval);
				}
				VALS(table) = xref(row);
			} else {
				for(; i<row->n; i++) {
					TABLE_col(table,i)=append(TABLE_col(table,i),ELl(row,i));
				}
			}
			return table;
		}
	}
	return EXC(Tt(type), "tables can't be joined with that type",table,row);
}
VP catenate(VP x,VP y) {
	VP res=0;
	XRAY_log("catenate\n");XRAY_emit(x);XRAY_emit(y);
	int n = x->n + y->n;
	if(!CONTAINER(x) && x->tag==0 && x->t==y->t) {
		XRAY_log("catenate2 - like simple objects\n");
		res=ALLOC_LIKE_SZ(x, n);
		appendbuf(res, BUF(x), x->n); appendbuf(res, BUF(y), y->n);
	} else {
		XRAY_log("catenate3 - container as x, or unlike objects\n");
		if(TABLE(x)) { return catenate_table(MUTATE_CLONE(x),y); }
		if(DICT(x)) { return dict(MUTATE_CLONE(x),y); }
		else if(LIST(x) && LEN(x)==0) { return xl(y); }
		else if(LIST(x)) { res=append(MUTATE_CLONE(x),y); }
		else {
			XRAY_log("catenate4 - create 2-item list with both items in it\n");
			res=xlsz(2);
			res=append(res,x);
			res=append(res,y);
		}
	}
	//res=list2vec(res);
	XRAY_log("catenate result");XRAY_emit(res);
	return res;
}
VP curtail(VP x) {
	// XRAY_log("curtail\n");XRAY_emit(x);
	return drop_(x,-1);
}
VP del_list_(VP x,int i) {
	if(!LIST(x) || i >= LEN(x)) return x;
	ARG_MUTATING(x);
	if(ELl(x,i)!=NULL) xfree(ELl(x,i));
	memmove(ELi(x,i),ELi(x,i+1),(sizeof(VP)*(LEN(x)-i-1)));
	x->n--;
	return x;
}
VP del_dict(VP x,VP y) {
	int i=_find1(x,y);
	if(i==-1) return x;
	KEYS(x)=del_list_(KEYS(x),i);
	VALS(x)=del_list_(VALS(x),i);
	return x;
}
VP del_ctx(VP x,VP y) {
	XRAY_log("del_ctx\n");XRAY_emit(x);XRAY_emit(y);
	KEYS(x)=del_dict(KEYS(x),y);
	XRAY_log("del_ctx returning\n");XRAY_emit(x);
	return x;
}
VP del(VP x,VP y) {
	XRAY_log("del\n");XRAY_emit(x);XRAY_emit(y);
	if(IS_EXC(x) || IS_EXC(y)) return x;
	if(IS_d(x) && IS_t(y)) return del_dict(x,y);
	if(IS_x(x) && IS_t(y)) return del_ctx(x,y);
	if(IS_l(x) && NUM(y) && !IS_c(y)) return del_list_(x,NUM_val(y));
	return EXC(Tt(nyi),"del not yet implemented for this type",x,y);
}
VP dict(VP x,VP y) {
	XRAY_log("dict\n");XRAY_emit(x);XRAY_emit(y);
	if(DICT(x)) {
		ARG_MUTATING(x);
		if(DICT(y)) {
			ASSERT(LIST(KEYS(x)) && LIST(VALS(x)),"dict() x keys or vals not list");
			int n = KEYS(y)->n;
			FOR(0,n,({ assign(x,DICT_key_n(y,_i),DICT_val_n(y,_i)); }));
			return x;
		} 
		if(KEYS(x)->n > VALS(x)->n) { // dangling dictionary detection
			append(VALS(x),y);
		} else {
			if(LIST(y) && y->n==2) {
				append(KEYS(x),first(y)); append(VALS(x),last(y));
			} else {
				append(KEYS(x),y);
			}
		}
		XRAY_log("dict already dict returning\n");XRAY_emit(x);
		return x;
	} else {
		if(x->n > 1 && LIST_of_lists(y)) return make_table(x,y);
		if(x->n > 1 && x->n != y->n) return EXC(Tt(value),"can't create dict from unlike vectors",x,y);
		VP d=xd0();
		if(LEN(x)==0 && LEN(y)==0) { KEYS(d)=xl0(); VALS(d)=xl0(); return d; }
		if(LIKELY(SCALAR(x))) {
			d=assign(d,DISCLOSE(x),DISCLOSE(y));
		} else {
			int i;VP tmp1,tmp2;
			for(i=0;i<x->n;i++) {
				tmp1=apply_simple_(x, i);
				tmp2=apply_simple_(y, i);
				d=assign(d,tmp1,tmp2);
			}
		}
		d->n=2;
		XRAY_log("dict new returning\n");XRAY_emit(d);
		return d;
	}
}
VP drop_(const VP x,int i) {
	VP res;
	int st, end;
	if(!x || x->n==0) return x;
	if(i<0) { st = 0; end=x->n+i; }
	else { st = i; end=x->n; }
	// XRAY_log("drop_(,%d) %d %d %d\n", i, x->n, st, end); XRAY_emit(x);
	res=ALLOC_LIKE_SZ(x,end-st);
	// XRAY_emit(info(res));
	if(end-st > 0) {
		appendbuf(res, ELi(x,st), end-st);
		if(LIST(x)) for(i=st;i<end;i++) {
			xref(ELl(res,i));
		}
	}
	XRAY_log("drop_ result\n"); XRAY_emit(res);
	return res;
}
VP drop(const VP x,const VP y) {
	VP res=0;
	int typerr=-1;
	// XRAY_log("drop args\n"); XRAY_emit(x); XRAY_emit(y);
	IF_RET(!NUM(y) || !SCALAR(y), EXC(Tt(type),"drop y arg must be single numeric",x,y));	
	if(TABLE(x)) return each(VALS(x),proj(2,x2(&drop),0,y));
	VARY_EL(y, 0, ({ return drop_(x,_x); }), typerr);
	return res;
}
VP enlist(const VP x) { // if x isnt a list, return [x]. else x.
	if(x==0) return xl0();
	if(LIST(x))return x;
	return xln(1,x);
}
VP except(const VP x,const VP y) {
	XRAY_log("except\n");XRAY_emit(x);XRAY_emit(y);
	return wherepred_(x,y,1);
}
VP extract(const VP data,const VP parts) {
	if(IS_EXC(data)) return data;
	if(IS_EXC(parts)) return parts;
	if(!LIST(data)) { return EXC(Tt(type),"extract x must be list",data,parts); }
	if(XXL_CUR_CTX==NULL) return EXC(Tt(context),"extract could not find current context",data,parts);
	int pl=MIN(LEN(data),LEN(parts)), i=0, cursor=0;
	VP res=xlsz(5); // arb
	VP acc=xd0(), px, dx;
	for(i=0; i<pl; i++) {
		px=apply_simple_(parts,i);
		dx=LIST_item(data,cursor);
		if(CALLABLE(px)) {
			VP tmp=apply(px,dx);
			res=append(res,tmp);
			xfree(tmp);
		} else if(IS_t(px) && AS_t(px,0)!=TINULL) {
			acc=assign(acc,px,dx);
		}
		xfree(px); // apply_simple_ always allocs
		cursor++;
	}
	for(; cursor<LEN(data); cursor++) {
		res=append(res,LIST_item(data, cursor));
	}
	return xln(2,acc,res);
}
VP extractas(const VP data,const VP parts) {
	VP res=extract(data,parts);
	VP vals=ELl(res,0), rest=ELl(res,1);
	int kl=LEN(KEYS(vals)), i;
	for(i=0; i<kl; i++) {
		set_is(DICT_key_n(vals,i),DICT_val_n(vals,i));
	}
	xref(rest);
	xfree(vals);
	xfree(res);
	return rest;
}
VP first(const VP x) {
	VP i,r;
	if(DICT(x)) { return EXC(Tt(type),"dict first/head doesn't make sense",x,0); }
	if(LIST(x)) { return xref(ELl(x,0)); }
	else return apply_simple_(x,0);
}
int _flat(const VP x) { // returns 1 if vector, or a list composed of vectors (and not other lists)
	// XRAY_log("flat\n");XRAY_emit(x);
	if(!CONTAINER(x)) return 1;
	else return 0; // lists are never flat
}
VP flatten0(VP x, int cont) {
	int i,t=-1;VP item,res=NULL;
	XRAY_log("flatten\n");XRAY_emit(x);
	if(!LIST(x))return x;
	if(x->n) {
		for(i=0;i<x->n;i++) {
			item=ELl(x,i);
			if(item==NULL) continue;
			if(LIST(item) && cont) item=flatten0(item,cont-1);
			if(!res) {
				t=item->t; res=ALLOC_LIKE(item);
			} else if(item->t!=t) {
				XRAY_log("gave up on");XRAY_emit(item);
				xfree(res); return x;
			}
			res=append(res,item);
		}
		XRAY_log("flatten returning\n");XRAY_emit(res);
		return res;
	} else return xl0();
}
VP flatten(VP x) {
	return flatten0(x, 1);
}
VP identity(VP x) {
	return x;
}
VP join(const VP list,const VP sep) {
	VP acc=NULL,x; int ln=LEN(list), i;
	if(IS_EXC(list) || ln==0) return list;
	if(SCALAR(list)) return apply_simple_(list,0);
	for(i=0;i<ln-1;i++) {
		x=apply_simple_(list,i);
		if(!acc) acc=ALLOC_LIKE(x);
		acc=append(acc,x);
		acc=append(acc,sep);
		xfree(x);
	}
	x=apply_simple_(list,i);
	acc=append(acc,x);
	xfree(x);
	return acc;
}
VP last(const VP x) {
	VP i,r;
	if(DICT(x)) { return EXC(Tt(type),"dict last/tail doesn't make sense",x,0); }
	if(LIST(x)) { return xref(ELl(x,LEN(x)-1)); }
	else return apply_simple_(x,_len(x)-1);
}
static inline VP list(const VP x) { // convert x to general list
	if(x==0) return xl0();
	if(LIST(x))return x;
	return split(x,xi0());
}
VP reverse(const VP x) {
	if(!(SIMPLE(x)||CONTAINER(x))) return EXC(Tt(type),"reverse arg must be simple or container",x,NULL);
	int i,typerr=-1; VP acc=ALLOC_LIKE(x);
	for(i=x->n-1;i>=0;i--) appendbuf(acc,ELi(x,i),1);
	return acc;
}
VP shift_(const VP x,int i) {
	// XRAY_log("shift_ %d\n",i);XRAY_emit(x);
	int n=x->n;
	if(i<0) return catenate(take_(x,i%n),drop_(x,i%n));
	else return catenate(drop_(x,i%n),take_(x,i%n));
}
VP shift(const VP x,const VP y) {
	XRAY_log("shift\n");XRAY_emit(x);XRAY_emit(y);
	if(!SIMPLE(x)) return EXC(Tt(type),"shr x must be a simple type",x,y);
	if(!NUM(y)) return EXC(Tt(type),"shr y must be numeric",x,y);
	int typerr=-1;
	VARY_EL(y,0,({return shift_(x,_x);}),typerr);
	return NULL;
}
VP show(const VP x) {
	char* p;
	XRAY_log("show\n");XRAY_emit(x);
	if(x==NULL) { printf("null\n"); return x; }
	if(IS_c(x)) p=sfromxA(x);
	else p=reprA(x);
	printf("%s\n",p); free(p);
	return x;
}
VP splice(const VP x,const VP idx,const VP replace) {
	int i, first=AS_i(idx,0), last=first+idx->n;
	VP acc;
	XRAY_log("splice (%d len) %d..%d",x->n, first,last);XRAY_emit(x);XRAY_emit(idx);XRAY_emit(replace);
	if(first==0 && last==x->n) return replace;
	acc=xl0();
	if(LIST(x)) {
		for(i=0;i<first;i++)
			acc=append(acc,ELl(x,i));
		append(acc,replace);
		for(i=last;i<x->n;i++)
			acc=append(acc,ELl(x,i));
	} else {
		if(first > 0) acc=append(acc, take_(x, first));
		acc=append(acc, replace);
		if (last < x->n) acc=append(acc, drop_(x, last));
		XRAY_log("splice calling over\n");XRAY_emit(acc);
		return over(acc, x2(&catenate));
	}
	XRAY_log("splice returning\n"); XRAY_emit(acc);
	return acc;
}
VP split(const VP x,const VP tok) {
	XRAY_log("split\n");XRAY_emit(x);XRAY_emit(tok);
	int typerr=-1;
	// special case for empty or null tok.. split vector into list
	if(tok->n==0) {
		VP tmp, tmp2;
		tmp=xl0();
		if(LIST(x)) return x;
		VARY_EACHLIST(x,({
			// XRAY_log("in split vary_each %c\n",_x);
			tmp2=ALLOC_LIKE_SZ(x, 1);
			tmp2=appendbuf(tmp2,(buf_t)&_x,1);
			tmp=append(tmp,tmp2);
		}),typerr);
		IF_RET(typerr>-1, EXC(Tt(type),"can't split that type", x, tok));
		XRAY_log("split returning\n");XRAY_emit(tmp);
		return tmp;
	} else if(x->t == tok->t) {
		int j, i=0, last=0, tokn=tok->n;
		VP rest=x, acc=0;
		for(; i<x->n; i++) {
			for(j=0; j<tokn; j++) {
				if(!_equalm(x,i+j,tok,j)) break;
				if(j==tokn-1) { // we made it to the end of the entire token!
					if(!acc)acc=xlsz(x->n/1);
					acc=append(acc,take_(rest,i-last));
					rest=drop_(rest,i+(tokn)-last);
					last=i+(tokn);
				}
			}
		}
		if(acc){ acc=append(acc,rest); return acc; }
		else return x;
	} else if(LIST(tok) && LIST_item(tok,0)->t == x->t) {
		// "/add?item=mongoose&sex=male" split "?" :: {split "&" :: {split "="}}
		// "/add?item=mongoose&sex=male" split ["?", "&", "="]
		VP res, newres, item; int i;
		if(!LIST(x)) res=xl(x);
		else res=x;
		for(i=0; i<LEN(tok); i++) {
			item=LIST_item(tok,i);
			newres=each(res,proj(2,&split,0,item));
			XRAY_log("split-deep loop\n");XRAY_emit(newres);
			if(newres!=res) xfree(res);
			if(IS_EXC(newres)) { return newres; }
			if(ENLISTED(newres)) { res=xref(LIST_item(newres,0)); xfree(newres); }
			else res=newres;
		}
		return res;
	} else 
		return EXC(Tt(nyi),"split with that type of data not yet implemented",x,tok);
}
VP take_(const VP x, const int i) {
	VP res;
	int xn=_len(x), st, end, j;
	if(!x || xn==0) return x;
	if(i<0) { st=ABS((xn+i)%xn); end=ABS(i)+st; } else { st=0; end=i; }
	res=ALLOC_LIKE_SZ(x,end-st);
	if(LIKELY(SIMPLE(x))) { 
		FOR(st,end,({ res=appendbuf(res,ELi(x,_i % xn),1); }));
	} else {
		if(!LIST(x)&&!TABLE(x)) 
			return EXC(Tt(nyi),"take not yet implemented for that type",x,xi(i));
		VP tmp;
		FOR(st,end,({ tmp=apply_simple_(x,_i%xn); res=append(res,tmp); xfree(tmp); }));
	}
	XRAY_log("take_ returning\n");XRAY_emit(res);
	return res;
}
VP take(const VP x, const VP y) {
	int typerr=-1;
	size_t st, end; //TODO slice() support more than 32bit indices
	XRAY_log("take args\n"); XRAY_emit(x); XRAY_emit(y);
	IF_RET(!NUM(y) || !SCALAR(y), EXC(Tt(type),"take y arg must be single numeric",x,y));	
	VARY_EL(y, 0, ({ return take_(x,_x); }), typerr);
	return NULL;
}
int _findbuf(const VP x, const buf_t y) {   // returns index or -1 on not found
	// XRAY_log("findbuf\n");XRAY_emit(x);
	if(LISTDICT(x)) { ITERV(x,{ 
		IF_RET(_findbuf(ELl(x,_i),y)!=-1,_i);
	}); } else {
		ITERV(x,{ IF_RET(memcmp(ELi(x,_i),y,x->itemsz)==0,_i); });
	}
	return -1;
}
inline int _find1(const VP x, const VP y) {        // returns index or -1 on not found
	// probably the most common, core call in the code. worth trying to optimize.
	// XRAY_log("_find1\n",x,y); XRAY_emit(x); XRAY_emit(y);
	ASSERT(x && (LISTDICT(x) || x->t==y->t), "_find1(): x must be list, or types must match");
	VP scan;
	if(UNLIKELY(DICT(x))) scan=KEYS(x);
	else scan=x;
	int sn=LEN(scan), yn=LEN(y), i;
	if(LIST(scan)) {
		int yt=y->t; VP item;
		for(i=0; i<sn; i++) {
			item=ELl(scan,i);

			if(item && ( (item->t == yt && item->n == yn) ||
									 (LIST(item) && LIST_item(item,0) && LIST_item(item,0)->t == yt) ))
				if (_equal(item,y)==1) return i;
		}
		return -1;
	} else {
		if(sn<yn) return -1;
		for(i=0; i<sn-(yn-1); i++) {
			if(memcmp(ELi(x,i), ELi(y,0), yn * x->itemsz)==0)
				return i;
		}
	}
	return -1;    // The code of this function reminds me of Armenia, or some war torn place
}
VP find1(const VP x, const VP y) {
	if(!x || !(LISTDICT(x) || x->t==y->t))
		return EXC(Tt(type),"find x must be list, or types must match",x,y);
	return xi(_find1(x,y));
}
int _contains(VP x, VP y) {
	return _find1(x,y)==-1 ? 0 : 1;
}
VP contains(VP x, VP y) {
	return xi(_contains(x,y));
}
VP condense(VP x) {
	// equivalent to k's & / where - condenses non-zeroes into their positions (ugh english)
	// always returns an int vector for now; we generally rely on ints too much for this
	int typerr=-1; int j; VP acc=xi0();
	// XRAY_log("condense\n");XRAY_emit(x);
	VARY_EACH(x,({ if(_x) { j=_i; FOR(0,_x,appendbuf(acc,(buf_t)&j,1)); } }),typerr);
	// XRAY_log("condense returning\n");XRAY_emit(acc);
	return acc;
}
VP table_row_list_(VP tbl, int row) { 
	int tc=TABLE_ncols(tbl), i;
	VP lst=xlsz(tc), res; 
	for(i=0; i<tc; i++) { 
		res=apply_simple_(TABLE_col(tbl,i), row); 
		lst=append(lst,DISCLOSE(res)); 
	}
	XRAY_log("table_row_list_ #%d returning\n");
	XRAY_emit(lst);
	return lst;
}
VP table_row_dict_(VP tbl, int row) {
	return dict(KEYS(tbl), table_row_list_(tbl, row));
}
VP make_table(VP keys,VP vals) {
  VP res, newvals; int i;
	XRAY_log("make_table\n");XRAY_emit(keys);XRAY_emit(vals);
	// approach: create empty table, then apply all these row values to it.
	// catenate_table already handles lists-of-lists correctly.
  res=xasz(2);
	EL(res,VP,0)=MUTATE_CLONE(keys);
	int sz=vals && LEN(vals) ? LEN(ELl(vals,0)) : 0;
	EL(res,VP,1)=xlsz(sz);
	res->n=2;
	return catenate_table(res, MUTATE_CLONE(vals));
}
VP make_many(VP x,VP y) {
	int i, j;
	int xn=LEN(x), yn=LEN(y);
	VP chars=xfroms("ist"); 
	VP funs=xln(3,
		proj(2,&base,0,xi(10)),
		proj(1,&str,0,0),
		proj(2,&make,0,Tt(tag))
	);
	VP d=dict(chars,funs), row=xlsz(yn);
	for(j=0; j<yn; j++) {
		char ch=AS_c(y,j);
		if(ch=='_') continue;
		VP tmpc=xc(ch); VP fun=DICT_find(d,tmpc); xfree(tmpc);
		VP item=apply_simple_(x,j);
		if(fun!=NULL) { VP newitem=apply(fun, item); if (newitem!=item) { xfree(item); item=newitem; } }
		row=append(row,item); xfree(item);
	}
	return row;
}
VP make(VP x, VP y) { 
	// TODO make() should short cut matching kind casts 
	// TODO make() should be smarter about lists in x
	XRAY_log("make\n");XRAY_emit(x);XRAY_emit(y);
	VP res=0; type_t typenum=-1; tag_t typetag=-1;
	// right arg is tag naming a type, use that.. otherwise use y's type
	if(IS_t(y)) {
		if(LEN(y)==0) typetag=TINULL;
		else typetag=AS_t(y,0); 
		if(LEN(x)==0) return make(xi(0),y);
	} else typenum=y->t;
	// convenience mode: ["123","abc","xyz"] $ "ist" -> [123,"abc",'xyz]
	if(LIST(x) && IS_c(y)) return make_many(x,y);
	if(IS_c(x) && (typetag==TINULL || typetag==Ti(tag))) {
		return xt(_tagnum(x));
	}
	if(typetag==Ti(table)) {
		if(!DICT(x)) return EXC(Tt(type), "can only create table from dict", x, y);
		return make_table(KEYS(x),VALS(x));
	}
	#include"cast.h"
	// XRAY_emitRAW(buf,BUFSZ);
	if(res) return res;
	if(typetag!=-1) {
		ARG_MUTATING(x); x->tag=typetag;
	}
	return x;
}
VP pin(VP x, VP y) { 
	// TODO cast() should short cut matching kind casts 
	XRAY_log("pin\n");XRAY_emit(x);XRAY_emit(y);
	tag_t newt = 0;
	if(IS_t(x)) 
		newt=AS_t(x,0);
	else if(x->tag != 0)
		newt=x->tag;
	if (newt) {
		/*
		if (newt==Ti(Pointer)) {
			VP ret=NULL;
			if (sizeof(void*)==4 && IS_i(y)) { ret=(VP)AS_i(y,0); }
			if (sizeof(void*)==8 && IS_j(y)) { ret=(VP)AS_j(y,0); }
			xref(ret); return ret;
		}
		*/
		ARG_MUTATING(y);
		y->tag=newt;
		return y;
	} else 
		return EXC(Tt(type),"pin x arg should be tag name or tagged value",x,y);
}
int _len(VP x) {
	int n = LEN(x);
	if(TABLE(x)) n=TABLE_nrows(x);
	if(DICT(x)) n=LEN(KEYS(x));
	return n;
}
VP len(VP x) {
	return xi(_len(x));
}
VP capacity(VP x) {
	return xin(1,x->cap);
}
VP itemsz(VP x) {
	return xi(x->itemsz);
}
VP info(VP x) {
	VP res; type_info_t t;
	t=typeinfo(x->t);
	res=xd0();
	res=assign(res,Tt(typenum),xi(x->t));
	res=assign(res,Tt(type),xfroms(t.name));
	res=assign(res,Tt(rc),xi(x->rc));
	res=assign(res,Tt(len),len(x));
	res=assign(res,Tt(capacity),capacity(x));
	res=assign(res,Tt(itemsz),itemsz(x));
	res=assign(res,Tt(alloced),xi(x->alloc));
	res=assign(res,Tt(baseptr),xj((long long)BUF(x)));
	res=assign(res,Tt(dataptr),xj((long long)BUF(x)));
	if(DICT(x)) {
		res=assign(res,Tt(keyinfo),info(KEYS(x)));
		res=assign(res,Tt(valinfo),info(VALS(x)));
	}
	if(TABLE(x)) {
		res=assign(res,Tt(keyinfo),info(KEYS(x)));
		res=assign(res,Tt(valinfo),each(VALS(x),x1(&info)));
	}
	return res;
}
VP type(VP x) {
	if(x==0 || x->t<0 || x->t>MAX_TYPE) return Tt(null);
	if(IS_EXC(x)) return xt(_tagnums("exception"));
	type_info_t t=typeinfo(x->t);
	return xt(_tagnums(t.name));
}
VP deal(VP range, VP amt) {
	XRAY_log("deal\n");XRAY_emit(range);XRAY_emit(amt);
	IF_EXC(!LIST(range) && !NUM(range),Tt(type),"deal: left arg must be numeric", range, amt);
	IF_EXC(!IS_i(amt) || !SCALAR(amt),Tt(type),"deal: single right arg must be int", range, amt);
	int typerr=-1;
	if(LIST(range)) {
		int i, rn=range->n, amtt=NUM_val(amt); 
		VP acc=xlsz(amtt);
		for(i=0; i<amtt; i++) acc=append(acc,ELl(range,rand()%rn));
		return acc;
	} else {
		VP acc=NULL;
		VARY_EL(amt, 0, ({ typeof(_x)amt=_x; acc=ALLOC_LIKE_SZ(range,_x); // TODO rethink deal in terms of more types
			VARY_EL_NOFLOAT(range, 0, ({
				FOR(0, amt, ({ if(_x==0)_x=1; EL(acc, typeof(_x), _i)=rand()%_x; }));
				acc->n=amt;
			}), typerr);}), typerr);
		return acc;
	}
}

// APPLICATION, ITERATION AND ADVERBS

static inline VP applyexpr(VP parent, VP code, VP xarg, VP yarg) {
	XRAY_log("applyexpr (code %p, xarg %p, yarg %p):\n", code, xarg, yarg);XRAY_emit(code);XRAY_emit(xarg);XRAY_emit(yarg);
	// if(!LIST(code))return EXC(Tt(code),"expr code not list",code,xarg);
	if(SIMPLE(code) || !LIST(code)) return code;
	char ch; int i; 
	VP left=xarg,item=0,oldleft=0,old_cur_ctx;
	tag_t tag, tcom=Ti(comment), texc=Ti(exception), texpr=Ti(expr), tlam=Ti(lambda), 
				tlistexpr=Ti(listexpr), tname=Ti(name), toper=Ti(oper),
				traw=Ti(raw), tstr=Ti(string), tws=Ti(ws);

	VP curframe,restore_left=0,stack=xl0();
	int stack_i=-1, start_i=0, use_existing_item=0, use_existing_left=0, return_expr_type=0, return_to=0;

	old_cur_ctx=XXL_CUR_CTX;
	stack=append(stack,xln(9,parent,code,xarg,yarg,left,XI0,xi(-1),xi(0),left));

	#define MAYBE_RETURN(value) \
		if (return_to!=-1) { \
			XRAY_log("applyexpr returning to frame %d\n",return_to); \
			XRAY_emit(value); \
			if(return_expr_type==1) { \
				XRAY_log("setting left in caller frame..\n"); \
				use_existing_left=1; \
				left=value; \
			} else if (return_expr_type==2) { \
				XRAY_log("setting item and left in caller frame..\n"); \
				XRAY_emit(value); XRAY_emit(restore_left); \
				use_existing_left=1; \
				use_existing_item=1; \
				if(code->tag==tlistexpr && !(value && DICT(value))) \
					item=list(value); \
				else item=value; \
				left=restore_left; \
			} \
			xfree(curframe); \
			EL(stack,VP,stack_i)=NULL; \
			stack_i=return_to; \
			start_i=AS_i(ELl(curframe,5),0); \
			goto applyexprtop; \
		} else { \
			XRAY_log("applyexpr actually returning\n"); \
			if(value==NULL) { XRAY_log("null\n"); } \
			else XRAY_emit(value); \
			if(old_cur_ctx!=NULL) { \
				XRAY_log("restoring XXL_CUR_CTX = %p\n", old_cur_ctx); XXL_CUR_CTX=old_cur_ctx; \
			} \
			return value; \
		} 

	applyexprtop:

	if (stack_i==-1) {
		stack_i=stack->n-1;
		XRAY_log("applyexpr picking fresh stack_i=%d of %d\n", stack_i, stack->n-1);
	} else {
		XRAY_log("applyexpr returning to stack #%d, i%d.. left/item:\n", stack_i, start_i);
		XRAY_emit(left);
		XRAY_emit(item);
	}
	curframe=ELl(stack,stack_i);
	XRAY_emit(curframe);

	parent=ELl(curframe,0);
	code=ELl(curframe,1);
	xarg=ELl(curframe,2);
	yarg=ELl(curframe,3);
	return_to=AS_i(ELl(curframe,6),0);
	return_expr_type=AS_i(ELl(curframe,7),0);
	restore_left=ELl(curframe,8);
	// we have to unpack the return_to stuff first, because
	// we use MAYBE_RETURN to immediately return exceptions
	// when adopting a left value from another frame, and 
	// MAYBE_RETURN needs to consider return_to*
	if (!use_existing_item && !use_existing_left)
		start_i=0;	
	if(!use_existing_left) left=ELl(curframe,4);
	else {
		XRAY_log("using existing left\n");XRAY_emit(left);
		// if(left!=0 && UNLIKELY(IS_EXC(left))) { MAYBE_RETURN(left); }
		use_existing_left=0;
	}

	XXL_CUR_CTX=parent; XRAY_log("setting XXL_CUR_CTX=%p\n",parent);

	if(SIMPLE(code)) { MAYBE_RETURN(code); }

	if(LIST(code) && code->tag==tlam && code->n == 2) {
		XRAY_log("unpacking lambda\n");
		int arity=LAMBDAARITY(code);
		if(arity==1 && xarg==0) {
			item=proj(-1,parent,xarg,yarg); MAYBE_RETURN(item);
		} 
		if(arity==2 && (xarg==0||yarg==0)) {
			item=proj(-2,parent,xarg,yarg); MAYBE_RETURN(item);
		}
		code=ELl(code,0);
	}
	
	XRAY_log("applyexpr code body:\n");
	XRAY_emit(code);

	for(i=start_i;i<code->n;i++) {
		XRAY_log("applyexpr #%d/%d\n",i,code->n-1);
		if (use_existing_item) {
			XRAY_log("using existing item\n"); XRAY_emit(item);
			use_existing_item=0;
			if(item==0) continue;
			// if(UNLIKELY(IS_EXC(item))) { MAYBE_RETURN(item); }
			goto evalexpr;
		} else XRAY_log("picking up item from code %d\n", i); 
		item=xref(ELl(code,i));
		XRAY_log("item=\n");XRAY_emit(item);
		tag=item->tag;
		// consider storing these skip conditions in an array
		if(tag==tws) continue;
		if(tag==tcom) continue;
		if(tag==tlam) { // create a context for lambdas
			/*
			VP itemr;
			itemr=clone(item);
			ELl(itemr,0)=wide(ELl(itemr,0),proj(2,&resolve,parent,0));
			VP newctx=CTX_make_subctx(parent,itemr); 
			item=newctx;
			*/
			item=CTX_make_subctx(parent,item);
			// printf("created new lambda context %p for item=\n%s\n",item,reprA(item));
		// } else if (tag==texpr || tag==tlistexpr) {
		// } else if (LIST(item) && !(LEN(item) && IS_c(LIST_first(item)) && LEN(LIST_first(item))==0)) { 
		} else if (LIST(item) && (item->tag==texpr || item->tag==tlistexpr)) {
			// we've reached a paren expr or list expr that we must resolve before considering
			// what to do with this term of the parse tree. many expressions are simple and
			// don't require complex evaluation.. but some do require recursion
			if (!SIMPLE(item) && LEN(item)>1) {
				XRAY_log("applying subexpression\n");
				VP newframe = xln(9,parent,item,xarg,yarg,NULL,xi(i),xi(stack_i),xi(2),left);
				XRAY_log("trying stack hack for expression (expr/listexpr)\n");
				XRAY_emit(newframe);
				stack=append(stack,newframe);
				stack_i=-1;
				goto applyexprtop;
			} else {
				// so we've come across a simple expression like `listexpr[2]. normally we'd just
				// move right along instead of recursing. however, this thing MAY be a name (string), and
				// because of the way the parser works, the parse tree may look like simply
				// "'listexpr("p")" with the 'name part stripped out. so let's reconsider item when
				// we reach this case so that we can resolve the name in the scope
				item->tag=(tag_t)0;
				item=flatten(item);
				/*
				if(IS_c(item))
					goto consideritem;
				else if(tag==tlistexpr)
					item=list(item);
				*/
			}
		} else if(LIKELY(IS_c(item)) && (tag==tname || tag==toper || tag==traw)) {
			ch = AS_c(item,0);
			// end of expr; cleanse expression
			if(ch==';') { 
				// if something on this line generated an exception, return it 
				if(IS_EXC(left)) { MAYBE_RETURN(left); }
				left=NULL; continue; 
			}
			XRAY_log("much ado about\n");XRAY_emit(item);
			if(item->n==1 && ch=='x') {
				if (LIKELY(xarg!=0)) 
					item=xarg;
				else {
					MAYBE_RETURN(proj(_arity(parent)*-1, parent, xarg, yarg));
				}
			} else if(item->n==1 && ch=='y') {
				if (LIKELY(yarg!=0)) {
					XRAY_log("picking up y arg\n");XRAY_emit(yarg);
					item=yarg;
				} else {
					// should return a projection here, but right now there is no way
					// to return a projection to a context
					// item=EXC(Tt(undef),"undefined y",xarg,yarg);
					// this exception in item is further handled below
					MAYBE_RETURN(proj(-2,parent,xarg,yarg));
				}
			}
			else if(item->n==2 && ch=='a' && AS_c(item,1)=='s') {
				left=proj(2,&set_as,left,NULL);
				XRAY_log("created set projection (as)\n"); XRAY_emit(left);
				continue;
			} else if(item->n==2 && ch=='i' && AS_c(item,1)=='s') {
				left=proj(2,&set_is,left,NULL);
				XRAY_log("created set projection (is)\n"); XRAY_emit(left);
				continue;
			} else if(item->n == 4 && ch == 'l' 
							&& AS_c(item,1)=='o' && AS_c(item,2)=='a' && AS_c(item,3)=='d') {
				XRAY_log("performing load");
				item=proj(2,&loadin,0,parent);
			} else if(item->n == 4 && ch == 's' 
							&& AS_c(item,1)=='e' && AS_c(item,2)=='l' && AS_c(item,3)=='f') {
				XRAY_log("using self");
				// item=clone(parent);
				item=parent;
			} else
				item=lookup(parent,item);
			XRAY_log("decoded string identifier\n");XRAY_emit(item);
			// if(IS_EXC(item)) { MAYBE_RETURN(left!=NULL && CALLABLE(left)?left:item); }
			// NB. i dont remember why we did the callable check there :/ note to self dont be evil
			if(IS_EXC(item)) { MAYBE_RETURN(item); }
		} else if(tag==tname) {
			XRAY_log("non-string name encountered");
			item=lookup(parent,item);
			RETURN_IF_EXC(item);
		} else if(tag==tstr) {
			// strip off the 'string tag right before we use this value. this is because the
			// 'string part is more of an indication of its role in parsing, but we dont want users'
			// code to see this tag applied - they didnt ask for it.
			item->tag=0;
		}
		
		evalexpr:

		XRAY_log("before evalexpr (left,item):\n");
		XRAY_emit(left); XRAY_emit(item);

		int leftarity=-1;
		if(left!=NULL) {leftarity=_arity(left); XRAY_log("left arity=%d\n", leftarity);}

		if(left!=0 && (left->tag==Ti(proj) || (CALLABLE(left) && leftarity>0))) {
			// they seem to be trying to call a unary function, though it's on the
			// left - NB. possibly shady
			//
			// note about IS_x use here: contexts are often passed around as values,
			// and this behavior is overriden to not support them in the left
			// position. this came about while testing ". get 'zebra" because "." is
			// a context which is callable and get is a verb which is (obviously)
			// callable. 
			//
			// regarding the first position in an expression: if you pass a
			// projection as the xargument, we should NOT immediately pass the next
			// value to it, even though it appears as "left". xused acts as a gate
			// for that "are we still possibly needing the passed-in (left) value?"
			// logic to not allow this behavior in first position inside expression
			Proj p;
			XRAY_log("applying dangling left callable\n");
			XRAY_emit(left);
			oldleft=left;
			if(left->tag==Ti(proj)) left=apply2(ELl(left,1),ELl(left,0),item);
			else left=apply(left,item);
			XRAY_log("applyexpr considering new apply(left,item)\n");
			XRAY_emit(left);
			xfree(oldleft);
			if(left == 0 || left->tag==texc) { MAYBE_RETURN(left); }
			if(IS_p(left)) {
				p=AS_p(left,0);
				if(yarg && p.type==2 && (!p.left || !p.right)) { 
					// NB this is the code that handles cases like 2 {*} 5 this should
					// happen only for simple expressions.. i feel like we need to have a
					// different class of expressions that work differently; slash
					// commands, implicit x, and here implicit y all work in that space,
					// but are confusing out of it. these are our versions of
					// compositions in k i suppose.
					oldleft=left;
					XRAY_log("applyexpr consuming y:\n");XRAY_emit(yarg);
					left=apply(oldleft,yarg);
					xfree(oldleft);
				} else {
					XRAY_log("applyexpr no y to consume\n");
				}
			} else {
				XRAY_log("applyexpr non-projection but callable left\n");
			}
		} else if(!CALLABLE(item) && (left!=0 && !CALLABLE(left))) {
			XRAY_log("applyexpr adopting left =\n");XRAY_emit(item);
			xfree(left);
			left=item;
		} else {
			if(left) {
				if (_arity(left)==2) {
					left=xln(2,left,item); left->tag=Ti(proj);
				} else {
					XRAY_log("applyexpr calling apply(item,left)\n");XRAY_emit(item);XRAY_emit(left);
					if(IS_x(item)) {
						VP newframe = xln(9,item,ELl(item,item->n-1),left,NULL,left,xi(i+1),xi(stack_i),XI1,left);
						XRAY_log("trying stack hack\n");
						XRAY_emit(newframe);
						stack=append(stack,newframe);
						stack_i=-1;
						goto applyexprtop;
					} else {
						XRAY_in();
						oldleft=left;
						left=xref(apply(item,left));
						xfree(oldleft); 
						XRAY_out();
					}
					// if(IS_EXC(left)) { MAYBE_RETURN(left); }
				}
				XRAY_log("applyexpr apply returned\n");XRAY_emit(left);
			} else {
				XRAY_log("no left, so continuing with item..\n");
				xfree(left);
				left=item;
			}
		}

		// recall that [1,2,3] goes into the parse tree as 'listexpr([1, 2, 3]), which is fine,
		// but as we evaluate [1, 2, 3], we don't have a way of forcing 1 into being a list before
		// that , occurs.. so after the first item, we force it to a list if it isnt one. :)
		/*
		if(i==0 && code->tag==tlistexpr && !LIST(left)) {
			XRAY_log("forcing left as list due to listexpr\n");
			left=xl(left);
		}
		*/

		XRAY_log("bottom\n");
	}
	XRAY_log("applyexpr done\n");
	XRAY_emit(left);
	MAYBE_RETURN(left);
}
static inline int _arity(const VP x) {
	int a=0;
	if(!CALLABLE(x)) return 0;
	if(CONTAINER(x)) {
		if (x->n >= 2 && x->tag==Ti(lambda)) 
			return LAMBDAARITY(x); // second item in lambda is arity
		else {
			int i;
			for(i=0; i<x->n;i++) 
				a+=_arity(ELl(x,i));
			return a;
		}
	}
	switch (x->t) {
		CASE(T_1,a+=1);
		CASE(T_2,a+=2);
		CASE(T_p,{ Proj p = AS_p(x,0); a+=MAX(abs(p.type) - (p.left!=0?1:0) - (p.right!=0?1:0), 0); });
	}
	return a;
}
static inline VP arity(const VP x) {
	int i=_arity(x);
	XRAY_log("arity = %d\n",i);XRAY_emit(x);
	VP res=xi(i);
	return res;
}
VP applyctx(VP ctx,const VP x,const VP y) {
	if(!IS_x(ctx)) return EXC(Tt(type),"context not a context",x,y);
	XRAY_log("applyctx\n");XRAY_emit(ctx);XRAY_emit(x);XRAY_emit(y);
	int a=_arity(ctx);
	if(a == 2 && (x==0 || y==0)) return proj(a*-1, ctx, x, y);
	if(a == 1 && (x==0 && y==0)) return proj(a*-1, ctx, x, y);
	int i,tlam=Ti(lambda);
	VP code,res=NULL;
	code=VALS(ctx);
	if(LIST(code)) res=applyexpr(ctx,code,x,y);
	XRAY_log("applyctx returning\n"); XRAY_emit(res);
	return res;
}
VP apply_simple_(VP x,int i) {   // a faster way to select just one item of x
	if(x==0 || x->tag==Ti(exception)) return x;
	// XRAY_log("apply_simple_ %d\n", i);
	if(TABLE(x)) return table_row_dict_(x,i);
	if(!LIST(x) && !SIMPLE(x)) { VP ii=xi(i), res=apply(x,ii); xfree(ii); return res; };
	if(i > LEN(x)) return EXC(Tt(index),"index out of range",x,xi(i));
	// see "special case for generic lists" below
	if(LIST(x)) return xref(ELl(x,i)); // XXX bug hunting maybe UNDO
	else {
		VP res=ALLOC_LIKE_SZ(x,1);
		res=appendbuf(res,ELi(x,i),1);
		return res;
	}
}
VP apply_table(VP x, VP y) {
	int tc=TABLE_ncols(x);
	int i;
	XRAY_log("apply_table\n");XRAY_emit(x);XRAY_emit(y);
	if(x==NULL || y==NULL) return NULL;
	if(NUMSTRICT(y)) {
		if(SCALAR(y)) {
			int yi=NUM_val(y);
			return table_row_dict_(x, yi);
		} else {
			// create a new table using the keys in y we could do this with less code
			// by using apply table with scalar indices (returning a dict) and then
			// joining them, but this code is pretty performance critical for some
			// operations
			VP newtbl=xasz(2), newvals=xlsz(LEN(y)), vec; int j;
			EL(newtbl,VP,0)=MUTATE_CLONE(KEYS(x));
			for(i=0; i<tc; i++) {
				XRAY_log("apply_table column #%d\n",i);
				vec=TABLE_col(x,i);
				newvals=append(newvals,apply(vec,y));
			}
			EL(newtbl,VP,1)=newvals;
			newtbl->n=2;
			XRAY_log("apply_table returning\n");XRAY_emit(newtbl);
			return newtbl;
		}
	} else {
		// as usual strings get special treatment.. doh
		int j, yn=LEN(y);
		VP res=xlsz(yn);
		for(j=0; j<yn; j++) {
			VP tcol;
			if(IS_c(y)) tcol=TABLE_col_named(x,y);
			else tcol=TABLE_col_named(x,apply_simple_(y, j));
			if(!tcol) return EXC(Tt(value),"unknown table column",x,y);
			XRAY_emit(tcol);
			int tr=TABLE_nrows(x);
			VP vals=xlsz(tr);
			for(i=0; i<tr; i++) {
				vals=append(vals,apply_simple_(tcol, i));
			}
			if(IS_c(y) || SCALAR(y)) { xfree(res); return vals; }
			else res=append(res,vals);
		}
		return res;
	}
}
VP apply(VP x,VP y) {
	// this function is everything.
	VP res=NULL;int i=0,typerr=-1;
	// XRAY_log("apply\n");XRAY_emit(x);XRAY_emit(y);
	if(x==0 || x->tag==Ti(exception))return x;
	if(IS_p(x)) { 
		// if its dyadic
		   // if we have one arg, add y, and call - return result
			 // if we have no args, set y as left, and return x
		// if its monadic
			// if we have one arg already, call x[left], and then apply result with y
		XRAY_log("apply proj\n");XRAY_emit(x);XRAY_emit(y);
		Proj* p; p=(Proj*)ELi(x,0);
		// this logic needs a simplification and rethink
		if(p->type < 0) {
			int a=p->type*-1;
			if(!p->left && y)
				p->left=y;
			else if (a==2 && !p->right && y) 
				p->right=y;
			return applyctx(p->ctx, p->left, p->right);
		} else if(p->type==1) {
			if (p->left) // not sure if this will actually happen - seems illogical
				return (*p->f1)(p->left);
			else
				return (*p->f1)(y);
		} else if(p->type==2) {
			XRAY_log("proj2\n"); 
			XRAY_in();
			if(p->left && p->right) y=(*p->f2)(p->left,p->right);
			else if(y && p->left) y=(*p->f2)(p->left, y);
			else if(y && p->right) y=(*p->f2)(y,p->right);
			else if(y) y=proj(2,p->f2,y,0);
			else y=x;
			XRAY_out(); XRAY_log("proj2 done\n");XRAY_emit(y);
			return y;
		}
		return xp(*p);
	}
	if(IS_1(x)) {
		// XRAY_log("apply 1\n");XRAY_emit(x);XRAY_emit(y);
		unaryFunc* f; f=AS_1(x,0); return (*f)(y);
	}
	if(IS_2(x)) {
		res=proj(2,AS_2(x,0),y,0);
		// XRAY_log("apply f2 returning\n");XRAY_emit(res);
		return res;
	}
	if(TABLE(x)) return apply_table(x,y);
	if(!CALLABLE(x) && UNLIKELY(y && LIST(y) && !SCALAR(y))) { 
		// indexing at depth - never done for callable types 1, 2, and p (but we do
		// use it for x).  we should think about when applying with a list really
		// means apply-at-depth. it would seem to make sense to have a list of
		// operations and a list of values and apply them to each but any kind of
		// logic likes this leads to ambiguity when the thing on the left is a
		// function that takes a general list as an argument (pretty common).
		// perhaps apply-at-depth should be a different operator, or only work when
		// using the @ form of apply (f@x rather than f x)
		XRAY_log("indexing at depth\n");
		XRAY_emit(x);
		XRAY_emit(y);
		res=x;
		for(;i<y->n;i++) {
			res=apply(res,ELl(y,i));
			XRAY_log("index at depth result %d\n",i);XRAY_emit(res);
			if(UNLIKELY(IS_EXC(res)) || res->n==0) 
				return res;
		}
		return res;
	}
	if(IS_x(x)) {
		// XRAY_log("apply ctx\n");XRAY_emit(x);XRAY_emit(y);
		return applyctx(x,y,0);
	}
	if(!y) return NULL; // this is the first point where we require y to be non-null, i believe;
	// TODO apply() is called most often in XXL; maybe worth transforming this if cascade into a case?	
	if(DICT(x)) {
		VP k=KEYS(x),v=VALS(x);I8 found;
		if(k==NULL || v==NULL) return NULL;
		res=xi0();
		if(LIST(k) && IS_c(y)) { // special case for strings as dictionary keys - common
			int idx;
			XRAY_log("searching for string\n");
			if ((idx=_find1(k,y))>-1)
				append(res,xi(idx));
		} else {
			ITERV(y,{ 
				int idx;
				// XRAY_log("searching %d\n",_i); XRAY_emit(y); XRAY_emit(k);
				if(LIST(y)) idx = _find1(k,ELl(y,_i));
				else idx = _findbuf(k,ELi(y,_i));
				if(idx>-1) {
					XRAY_log("found at idx %d\n", idx); 
					found=1; append(res,xi(idx));
					break;
				}
			});
		}
		// nyi
		// if(res->n==0 && x->next!=0) res=apply((VP)x->next, res);
		if(res->n==0) return NULL; else return apply(v,res);
	}
	if(NUM(y)) { // index a value with an integer 

		if(SCALAR(y)) {
			return apply_simple_(x, NUM_val(y));
		/*
			y->n==1 && LIST(x)) {
			// special case for generic lists: if you index with one item, return
			// just that item generally you would receive a list back this may
			// potentially become painful later on 
			i = AS_i(y,0); 
			XRAY_log("apply with number %d, x is list\n", i);
			IF_RET(i>=x->n, ({ EXC(Tt(index),"index out of range",x,y);	}));
			VP tmp = ELl(x,i); xref(tmp); return tmp;
		*/
		} else {
			res=xalloc(x->t,y->n);
			VARY_EACH_NOFLOAT(y,appendbuf(res,ELi(x,_x),1),typerr);
			//XRAY_log("apply VARY_EACH after\n"); XRAY_emit(res);
			if(typerr>-1) return EXC(Tt(type),"cant use y as index into x",x,y);
			return res;
		}
	}
	// sleep(3);printf("%s %s\n",reprA(x),reprA(y));
	return EXC(Tt(apply),"apply failure",x,y);
}
VP apply2(const VP f,const VP x,const VP y) {
	if(IS_2(f) && x && y)
		return AS_2(f,0)(x,y);
	else if(IS_x(f))
		return applyctx(f,x,y);
	else
		return apply(apply(f,x),y);
}
VP deepinplace(const VP obj,const VP f) {
	// TODO perhaps deep() should throw an error with non-list args - calls each() now
	int i;
	// XRAY_log("deep\n");XRAY_emit(info(obj));XRAY_emit(obj);XRAY_emit(f);
	VP acc,subobj;
	if(!CONTAINER(obj)) return each(obj,f);
	if(_flat(obj)) {
		// XRAY_log("deep flat\n");
		acc=apply(f,obj);
		if(obj->tag) acc->tag=obj->tag;
		return acc;
	}
	XRAY_in();
	FOR(0,obj->n,({
		subobj=ELl(obj,_i);
		if(LIST(subobj)) subobj=deepinplace(subobj,f);
		else subobj=apply(f,subobj);
		EL(obj,VP,_i) = subobj; 
		// NB. this may not be safe, but append() is overriden for dicts, so we cant simply append the list
		// NB. check this for ref count or clone errors.
	}));
	XRAY_out();
	XRAY_log("deep returning\n");XRAY_emit(obj);
	return obj;
}
VP deep(const VP obj,const VP f) {
	// TODO perhaps deep() should throw an error with non-list args - calls each() now
	int i;
	// XRAY_log("deep\n");XRAY_emit(info(obj));XRAY_emit(obj);XRAY_emit(f);
	VP acc,subobj;
	if(!CONTAINER(obj)) return each(obj,f);
	if(_flat(obj)) {
		acc=apply(f,obj);
		if(obj->tag) acc->tag=obj->tag;
		return acc;
	}
	acc=ALLOC_LIKE(obj);
	XRAY_in();
	FOR(0,obj->n,({
		subobj=ELl(obj,_i);
		if(LIST(subobj)) subobj=deep(subobj,f);
		else subobj=apply(f,subobj);
		EL(acc,VP,_i) = subobj; 
		// NB. this may not be safe, but append() is overriden for dicts, so we cant simply append the list
		// NB. check this for ref count or clone errors.
	}));
	acc->n=obj->n;
	XRAY_out();
	// XRAY_log("deep returning\n");XRAY_emit(acc);
	return acc;
}
static inline VP eachdict(const VP obj,const VP fun) {
	if(KEYS(obj)->n==0) return 0;
	if(VALS(obj)->n==0) return 0;
	VP vals=each(VALS(obj),fun);
	if(!vals) return 0;
	return dict(KEYS(obj),vals);
}
static inline VP eachtable(const VP obj, const VP fun) {
	XRAY_log("eachtable\n"); XRAY_emit(obj); XRAY_emit(fun);
	int objnr=TABLE_nrows(obj);
	VP acc=NULL, tmpdict=NULL, res, item, tmpi; int i; 
	for(i=0; i<objnr; i++) {
		tmpdict=table_row_dict_(obj, i);
		XRAY_log("calling eachtable fun\n"); XRAY_emit(tmpdict);
		XRAY_in();
		res=apply(fun,tmpdict);
		XRAY_out();
		XRAY_log("eachtable fun result\n"); XRAY_emit(res);
		if(res==NULL) continue;
		if(IS_EXC(res)) { if(acc) xfree(acc); return res; }
		if(!acc) acc=xalloc(SCALAR(res) ? res->t : 0,obj->n); 
		else if (!LIST(acc) && res->t != acc->t) {
			VP newacc=xl(acc); xfree(acc); acc=newacc;
		}
		acc=append(acc,res);
		xfree(res); xfree(tmpdict); 
	}
	XRAY_log("eachtable returning\n"); XRAY_emit(acc);
	return acc;
}
VP each(const VP obj,const VP fun) { 
	// each returns a list if the first returned value is the same as obj's type
	// and has one item
	VP tmp, res, acc=NULL; 
	if(DICT(obj)) return eachdict(obj,fun);	
	if(TABLE(obj)) return eachtable(obj,fun);
	int n=LEN(obj), i;
	if(n==0) return obj;
	// we normally want to try to create vectors from each, but not if the
	// object isn't a vector in the first place, so pre-set acc here
	if(!SIMPLE(obj)) acc=xlsz(n); 
	// XRAY_log("each\n");XRAY_emit(obj);XRAY_emit(fun);
	for(i=0; i<n; i++) {
		// XRAY_log("each #%d\n",n);
		tmp=apply_simple_(obj, i); 
		if(IS_EXC(tmp)) { if(acc) xfree(acc); return tmp; }
		res=apply(fun,tmp); 
		if(res==0) continue; // no idea lol
		if(IS_EXC(res)) { if(acc) xfree(acc); return res; }
		// delay creating return type until we know what this func produces
		if (!acc) acc=xalloc(SCALAR(res) ? res->t : 0,obj->n); 
		else if (!LIST(acc) && res->t != acc->t) acc = xl(acc);
		// XRAY_log("each tmp (rc=%d)\n",tmp->rc);XRAY_emit(tmp);
		append(acc,res);
		xfree(tmp);
	}
	// XRAY_log("each returning\n");XRAY_emit(acc);
	return acc;
}
VP eachboth(const VP obj,const VP fun) { 
	if(!LIST(obj) || obj->n != 2) return EXC(Tt(type),"eachboth x must be [left,right]",obj,fun);
	VP tmpl, tmpr, res, acc=NULL, left=ELl(obj,0), right=ELl(obj,1); int n=left->n;
	if(right->n != n) return EXC(Tt(type),"eachboth x must be same-length items",obj,fun);
	FOR(0,n,({ 
		tmpl=apply(left, xi(_i)); tmpr=apply(right, xi(_i)); res=apply2(fun,tmpl,tmpr); 
		if(res==NULL) continue; 
		// delay creating return type until we know what this func produces
		if (!acc) acc=xalloc(SCALAR(res) ? res->t : 0,obj->n); 
		else if (!LIST(acc) && res->t != acc->t) acc = xl(acc);
		xfree(tmpl); xfree(tmpr); append(acc,res); }));
	return acc;
}
VP eachleft(const VP obj,const VP fun) { 
	if(!LIST(obj) || obj->n != 2) return EXC(Tt(type),"eachleft x must be [left,right]",obj,fun);
	VP tmpl, res, acc=NULL, left=ELl(obj,0), right=ELl(obj,1); int n=left->n;
	// it can be a little finnicky with , to build a list composed of a list and
	// a vector, because right now [1,2,3],4 is [1,2,3,4] not [[1,2,3],[4]] -
	// which is usually quite helpful, but not in this case:
	if (ENLISTED(right)) right=ELl(right,0);  
	FOR(0,n,({ 
		tmpl=apply(left, xi(_i)); res=apply2(fun,tmpl,right); 
		if(res==NULL) continue;
		// delay creating return type until we know what this func produces
		if (!acc) acc=xalloc(SCALAR(res) ? res->t : 0,obj->n); 
		else if (!LIST(acc) && res->t != acc->t) acc = xl(acc);
		xfree(tmpl); append(acc,res); }));
	return acc;
}
VP eachright(const VP obj,const VP fun) { 
	if(!LIST(obj) || obj->n != 2) return EXC(Tt(type),"eachright x must be [left,right]",obj,fun);
	VP tmpr, res, acc=NULL, left=ELl(obj,0), right=ELl(obj,1); int n=right->n;
	if (ENLISTED(left)) left=ELl(left,0);  
	FOR(0,n,({ 
		tmpr=apply(right, xi(_i)); res=apply2(fun,left,tmpr);
		if(res==NULL) continue;
		// delay creating return type until we know what this func produces
		if (!acc) acc=xalloc(SCALAR(res) ? res->t : 0,obj->n); 
		else if (!LIST(acc) && res->t != acc->t)  acc = xl(acc);
		xfree(tmpr); append(acc,res); }));
	return acc;
}
static inline VP eachpair(VP obj,VP fun) {
	ASSERT(1,"eachpair nyi");
	return NULL;
}
VP exhaust0(const VP x,const VP y,int collect) {
	int i;
	VP last=x,this=NULL,acc=NULL;
	if(collect) acc=xl0();
	for(i=0;i<MAXSTACK;i++) {
		XRAY_log("exhaust calling #%d\n",i);XRAY_emit(y);XRAY_emit(this);XRAY_emit(last);
		XRAY_in();
		this=apply(y,last);
		XRAY_out();
		XRAY_log("exhaust result #%d\n",i);XRAY_emit(last);XRAY_emit(this);
		if(IS_EXC(this) || UNLIKELY(_equal(this,x) || _equal(this,last))) {
			xfree(last); 
			if(collect) return acc;
			else return last;
		} else {
			if(collect) acc=append(acc,this);
			xfree(last);
			last=this;
		}
	}
	return EXC(Tt(exhausted),"exhaust hit stack limit",x,last);
}
VP exhaust(const VP x,const VP y) {
	int i;
	XRAY_log("+++EXHAUST\n");XRAY_emit(x);XRAY_emit(y);
	VP res=exhaust0(x,y,0);
	XRAY_log("exhaust returning\n");XRAY_emit(res);
	return res;
}
VP over(const VP x,const VP y) {
	//XRAY_log("over\n");XRAY_emit(x);XRAY_emit(y);
	if(!INDEXABLE(y)) { return EXC(Tt(type),"over y must be indexable",x,y); }
	if(LEN(x)==0) return ALLOC_LIKE_SZ(x,0);
	VP first=NULL,values,last,next;
	if(LIST(x) && x->tag!=0) {
		if(LEN(x)==2 && x->tag==Ti(init)) {
			first=LIST_item(x,0); values=LIST_item(x,1);
		}
	}
	if(!first) { values=behead(x); first=apply_simple_(x,0); }
	last=first;
	FOR(0,values->n,({
		next=apply_simple_(values, _i);
		last=apply2(y,last,next);    // TODO over should use apply2
		xfree(next);
	}));
	// TODO free first if alloced
	return last;
}
VP scan(const VP x,const VP y) {       // returns a list if result vals dont match
	// XRAY_log("scan\n");XRAY_emit(x);XRAY_emit(y);
	IF_RET(!INDEXABLE(y), EXC(Tt(type),"scan y must be indexable",x,y));
	VP last,next,acc=0; int xn=LEN(x),i;
	if(xn<2) return apply(y,x);
	last=apply_simple_(x,0);
	acc=ALLOC_LIKE(x);
	acc=append(acc,last);
	for(i=0; i<xn; i++) {
		next=apply_simple_(x,i);
		VP tmp=apply(y,last);              // TODO scan should use apply2
		if(tmp!=last) xfree(last);
		last=apply(tmp,next);
		if(last!=next) xfree(next);
		XRAY_log("scan step\n");XRAY_emit(last);
		acc=append(acc,last);
	}
	XRAY_log("scan result\n");XRAY_emit(acc);
	return acc;
}
VP recurse(const VP x,const VP y) {
	// note: monadic scan in q returns the argument as the first
	// member of the result. seems counterintuitive. we do not. 
	// q)(neg\)1
	// 1 -1
	// xxl: 0. 1 recurse neg
	// -1
	int i;
	XRAY_log("+++RECURSE\n");XRAY_emit(x);XRAY_emit(y);
	VP res=exhaust0(x,y,1);
	XRAY_log("recurse returning\n");XRAY_emit(res);
	return res;
}
VP wide0(const VP obj,const VP f,int listsonly) {
	int i; VP acc;
	XRAY_log("wide0\n");XRAY_emit(obj);XRAY_emit(f);
	if(!CONTAINER(obj)) return apply(f, obj);
	// XRAY_log("wide top level\n");XRAY_emit(obj);
	acc=apply(f,obj);
	if(IS_EXC(acc)) return acc;
	if(CONTAINER(acc)) {
		for(i=0;i<acc->n;i++) {
			//XRAY_log("wide #%d\n",i);XRAY_in();
			if(!listsonly || LIST(ELl(acc,i))) EL(acc,VP,i)=wide0(ELl(acc,i),f,listsonly);
			//XRAY_out();
		}
	}
	return acc;
}
VP wide(const VP obj,const VP f) {
	if(IS_EXC(obj)) return obj;
	XRAY_log("wide\n");XRAY_emit(obj);XRAY_emit(f);
	return wide0(obj,f,1);
}

// MATHY/LOGICY STUFF:

VP abss(const VP x) {
	if(!SIMPLE(x)) return EXC(Tt(type),"abs only supports simple types",x,0);
	VP acc=ALLOC_LIKE(x); int typerr=-1;
	VARY_EACH(x,({ _x=abs(_x); appendbuf(acc,(buf_t)&_x, 1); }),typerr);
	if(typerr!=-1) return EXC(Tt(type),"abs type not valid",x,0);
	return acc;
}
VP and(const VP x,const VP y) {
	int typerr=-1;
	VP acc;
	XRAY_log("and\n"); XRAY_emit(x); XRAY_emit(y); // TODO and() and friends should handle type conversion better
	if(IS_EXC(x) || LEN(x)==0) return x; // NB. IS_EXC checks if x==NULL
	if(IS_EXC(y) || LEN(y)==0) return y;
	if(LIST(x) || LIST(y)) return EXC(Tt(nyi),"and on lists not yet implemented",x,y);
	IF_EXC(x->n > 1 && y->n > 1 && x->n != y->n, Tt(len), "and arguments should be same length", x, y);	
	if(SIMPLE(x) && SIMPLE(y)) acc=ALLOC_BEST(x,y);
	// XRAY_log("and acc\n");XRAY_emit(acc);
	if(x->t > y->t) {
		VARY_EACHBOTH(x,y,({ 
			if (_x > _y) _x=_y; appendbuf(acc, (buf_t)&_x, 1); }), typerr);
  } else {
		VARY_EACHBOTH(x,y,({ 
			if (_x < _y) _y=_x; appendbuf(acc, (buf_t)&_y, 1); }), typerr);
	}
	IF_EXC(typerr != -1, Tt(type), "and arg type not valid", x, y);
	// XRAY_log("and result\n"); XRAY_emit(acc);
	return acc;
}
int _any(VP x) {
	int typerr=-1;
	VP acc;
	XRAY_log("_any\n"); XRAY_emit(x);
	if(IS_EXC(x)) return 0;              // IS_EXC checks if it's 0 also
	if(LIST(x)) x=list2vec(deep(x,x1(&any)));
	VARY_EACH(x,({ 
		if(_x!=0) return 1;
	}),typerr);
	// since this routine returns an int we can't return an exception!
	ASSERT(typerr==-1, "_any noniterable arg");
	// XRAY_log("_any returning 0\n");
	return 0;
}
VP any(VP x) {
	//XRAY_log("any\n");XRAY_emit(x);
	IF_EXC(!SIMPLE(x) && !LIST(x), Tt(type), "any arg must be list or simple type", x, 0);
	if(LIST(x)) return deep(x,x1(&any));
	return xb(_any(x));
}
VP aside(VP x,VP y) {
	// used to do some work with the chaining value, but in such a way as to not
	// disrupt what we're building up:
	// 100 count % 2 aside {['mods,x]show} * 5..
	// without aside, the ['mods,x] would be passed on to * 5..
	// luckily a very simple implementation
	if(CALLABLE(y)) {
		VP res=apply(y,x);
		if(res!=x && res!=y) xfree(res);
	}
	return x;
}
VP base(VP x,VP y) {
	// two different functions in one:
	// 1. convert string x to number, assuming it's a number formatted in base y.
	// 2. convert an XXL number x to a string in base y
	if(NUM(y) && !IS_c(y)) {
		int b=NUM_val(y), xn=LEN(x), i;
		if(IS_c(x)) {
			if(b==16 && (xn < 3 || AS_c(x,0)!='0' || AS_c(x,1)!='x')) {
				return str2num(catenate(xfroms("0x"),x));
			} 
			if(b==10 || b==16) return str2num(x);
			return EXC(Tt(nyi), "base not yet implemented", x, y);
		}
		if(NUM(x)) {
			VP res=xcsz(5); // ARB
			for(i=0; i<xn; i++) res=append(res,numelem2base(x, i, b));
			return res;
		}
		return NULL;
	} else {
		int yn=LEN(y);
		VP res;
		if(NUM(x) && !IS_c(x)) {
			res=ALLOC_LIKE_SZ(y,5); //arb
			VP tmp;
			auto n=NUM_val(x);
			do {
				tmp=apply_simple_(y,n%yn);
				res=append(res,tmp);
				n=n/yn;
				xfree(tmp);
			} while (n>0);
			res=reverse(res);
		} else {
			int i, n;
			VP tmp;
			I128 acc=0;
			for(i=0; i<LEN(x); i++) {
				tmp=apply_simple_(x,i);
				n=_find1(y, tmp);
				xfree(tmp);
				if(n==-1) return EXC(Tt(value),"base can't decode token",x,y);
				acc=(acc*yn)+n;
			}
			res=xo(acc);
		}
		return res;
	}
}
VP casee(VP x,VP y) {
	if(!LIST(y) || LEN(y) < 2) return EXC(Tt(type),"case y arg should be [cond1,result1,cond2,result2,elseresult]",x,y);
	int i, yn=LEN(y), haselse=(yn%2==1); VP cond=NULL, res=NULL;
	for(i=0; i<(yn-haselse); i+=2) {
		cond=LIST_item(y,i);
		XRAY_log("case considering\n");XRAY_emit(cond);
		if(CALLABLE(cond)) {
			VP testcond=apply(cond,x);
			if(_any(testcond)) {
				XRAY_log("case found match\n");
				xfree(testcond);
				res=LIST_item(y,i+1);
				break;
			}
			xfree(testcond);
		} else if (_equal(x,cond))
			res=LIST_item(y,i+1);
	}
	if(res==NULL && haselse) res=LIST_item(y,yn-1);
	if(res) {
		if(CALLABLE(res)) return apply(res,x);
		else return res;
	} else return x;
}
static inline VP divv(VP x,VP y) { 
	int typerr=-1; VP acc=ALLOC_BEST(x,y);
	// XRAY_log("div");XRAY_emit(x);XRAY_emit(y);XRAY_emit(acc);
	if(UNLIKELY(!SIMPLE(x))) return EXC(Tt(type),"div argument should be simple types",x,0);
	VARY_EACHBOTH(x,y,({
		if(_y==0) return EXC(Tt(divzero),"divide by zero in mod",x,y);
		if(LIKELY(x->t > y->t)) { _x=_y/_x; appendbuf(acc,(buf_t)&_x,1); }
		else { _y=_y/_x; appendbuf(acc,(buf_t)&_y,1); }
		if(!SCALAR(x) && SCALAR(y)) _j=-1; // NB. AWFUL!
	}),typerr);
	IF_EXC(typerr > -1, Tt(type), "div arg wrong type", x, y);
	XRAY_log("div result\n"); XRAY_emit(acc);
	return acc;
}
VP ifelse(VP x,VP y) {
	XRAY_log("ifelse\n");XRAY_emit(x);XRAY_emit(y);
	if(y->n!=2) return EXC(Tt(len),"ifelse y argument must be (truecond,falsecond)",x,y);
	VP tcond=apply(y,XI0),fcond=apply(y,XI1),res=0;
	if(x->n == 0) res=fcond;
	else if(NUM(x) && !_any(x)) res=fcond;
	else res=tcond;
	if(CALLABLE(res))
		return apply(res,x);
	else
		return res;
}
VP iftrue(VP x,VP y) {
	if(x->n == 0) return x;
	else if(NUM(x) && !_any(x)) return x;
	else {
		if(CALLABLE(y)) return apply(y,x);
		else return y;
	}
}
VP greater(VP x,VP y) {
	int typerr=-1;
	VP acc,v0=xb(0),v1=xb(1);
	XRAY_log("greater\n"); XRAY_emit(x); XRAY_emit(y); 
	if(!SIMPLE(x) || !SIMPLE(y)) return EXC(Tt(type), "> args should be simple types", x, y);
	IF_EXC(x->n > 1 && y->n > 1 && x->n != y->n, Tt(len), "> arguments should be same length", x, y);	
	acc=xbsz(MAX(x->n,y->n));
	VARY_EACHBOTH(x,y,({ 
		if (_x <= _y) append(acc, v0); 
		else append(acc, v1); 
		if(!SCALAR(x) && SCALAR(y)) _j=-1; // NB. AWFUL!
	}), typerr);
	IF_EXC(typerr != -1, Tt(type), "> arg type not valid", x, y);
	XRAY_log("> result\n"); XRAY_emit(acc);
	xfree(v0);xfree(v1);
	return acc;
}
VP lesser(VP x,VP y) {
	int typerr=-1;
	VP acc,v0=xb(0),v1=xb(1);
	XRAY_log("lesser\n"); XRAY_emit(x); XRAY_emit(y); 
	if(!SIMPLE(x) || !SIMPLE(y)) return EXC(Tt(type), "< args should be simple types", x, y);
	IF_EXC(x->n > 1 && y->n > 1 && x->n != y->n, Tt(len), "< arguments should be same length", x, y);	
	acc=xbsz(MAX(x->n,y->n));
	VARY_EACHBOTH(x,y,({ 
		if (_x >= _y) append(acc, v0); 
		else append(acc, v1); 
		if(!SCALAR(x) && SCALAR(y)) _j=-1; // NB. AWFUL!
	}), typerr);
	IF_EXC(typerr != -1, Tt(type), "< arg type not valid", x, y);
	XRAY_log("< result\n"); XRAY_emit(acc);
	xfree(v0);xfree(v1);
	return acc;
}
VP min(VP x) { 
	if (!SIMPLE(x)) return over(x, x2(&and));
	if(x->n==1)return x;
	VP res=ALLOC_LIKE_SZ(x,1); int typerr=-1;
	VARY_EACH(x,({
		if(UNLIKELY(_i==0) || _x < _xtmp) 
			EL(res,typeof(_x),0)=_xtmp=_x;
	}),typerr); // no need to check typerr due to SIMPLE check above
	res->n=1;
	return res;
}
VP max(VP x) { 
	// TODO implement max() as loop instead of over - used in parsing
	// XRAY_log("max\n");XRAY_emit(x);
	if (!SIMPLE(x)) return over(x, x2(&or));
	if(x->n==1)return x;
	VP res=ALLOC_LIKE_SZ(x,1); int typerr=-1;
	VARY_EACH(x,({
		if(UNLIKELY(_i==0) || _x > _xtmp) 
			EL(res,typeof(_x),0)=_xtmp=_x;
	}),typerr); // no need to check typerr due to SIMPLE check above
	res->n=1;
	return res;
}
VP minus(VP x,VP y) {
	int typerr=-1;
	XRAY_log("minus\n");XRAY_emit(x);XRAY_emit(y);
	IF_EXC(!SIMPLE(x) || !SIMPLE(y), Tt(type), "minus args should be simple types", x, y); 
	VP acc=ALLOC_BEST(x,y);
	VARY_EACHBOTH(x,y,({
		if(LIKELY(x->t > y->t)) { _x=_x-_y; appendbuf(acc,(buf_t)&_x,1); }
		else { _y=_x-_y; appendbuf(acc,(buf_t)&_y,1); }
		if(!SCALAR(x) && SCALAR(y)) _j=-1; // NB. AWFUL!
	}),typerr);
	IF_EXC(typerr > -1, Tt(type), "minus arg wrong type", x, y);
	// XRAY_log("minus result\n"); XRAY_emit(acc);
	return acc;
}
VP mod(VP x,VP y) {
	// TODO mod probably *doesnt* need type promotion
	int typerr=-1;
	XRAY_log("mod\n");XRAY_emit(x);XRAY_emit(y);
	IF_EXC(!SIMPLE(x) || !SIMPLE(y), Tt(type), "mod args should be simple types", x, y); 
	VP acc=ALLOC_BEST(x,y);
	if(LIKELY(x->t > y->t)) {
		VARY_EACHBOTH_NOFLOAT(x,y,({ 
			if(_x==0)_x=0; else _x=_x%_y;
			appendbuf(acc,(buf_t)&_x,1); if(!SCALAR(x) && SCALAR(y)) _j=-1; // NB. AWFUL!
		}),typerr);
	} else {
		VARY_EACHBOTH_NOFLOAT(x,y,({ 
			if(_y==0) return EXC(Tt(divzero),"divide by zero in mod",x,y);
			if(_x==0)_y=0; else _y=_x%_y;
			appendbuf(acc,(buf_t)&_y,1); if(!SCALAR(x) && SCALAR(y)) _j=-1; // NB. AWFUL!
		}),typerr);
	}
	IF_EXC(typerr > -1, Tt(type), "mod arg wrong type", x, y);
	XRAY_log("mod result\n"); XRAY_emit(acc);
	return acc;
}
VP neg(VP x) {
	int typerr=-1;
	VP acc;
	// XRAY_log("and\n"); XRAY_emit(x); XRAY_emit(y); // TODO and() and friends should handle type conversion better
	acc=ALLOC_LIKE(x);
	VARY_EACH(x,({ 
		_x=-_x; appendbuf(acc,(buf_t)&_x,1);
	}),typerr);
	IF_EXC(typerr != -1, Tt(type), "not arg type not valid", x, 0);
	// XRAY_log("and result\n"); XRAY_emit(acc);
	return acc;
}
VP not(VP x) {
	int typerr=-1;
	VP acc;
	// XRAY_log("and\n"); XRAY_emit(x); XRAY_emit(y); // TODO and() and friends should handle type conversion better
	acc=ALLOC_LIKE(x);
	VARY_EACH(x,({ 
		_x=!_x; appendbuf(acc,(buf_t)&_x,1);
	}),typerr);
	IF_EXC(typerr != -1, Tt(type), "not arg type not valid", x, 0);
	// XRAY_log("and result\n"); XRAY_emit(acc);
	return acc;
}
VP or(VP x,VP y) { // TODO most of these primitive functions have the same pattern - abstract?
	int typerr=-1;
	VP acc;
	// XRAY_log("or\n"); XRAY_emit(x); XRAY_emit(y); // TODO or() and friends should handle type conversion better
	if(IS_EXC(x) || LEN(x)==0) return y;
	if(IS_EXC(y) || LEN(y)==0) return x;
	if(DICT(x) && DICT(y)) return unionn(x,y);
	if(LIST(x) || LIST(y)) return EXC(Tt(nyi),"or does not currently support lists",x,y);
	IF_EXC(x->n > 1 && y->n > 1 && x->n != y->n, Tt(len), "or arguments should be same length", x, y);	
	acc=xalloc(x->t, x->n);
	VARY_EACHBOTH(x,y,({ if (_x > _y) appendbuf(acc, (buf_t)&_x, 1); 
		else appendbuf(acc, (buf_t)&_y, 1); }), typerr);
	IF_EXC(typerr != -1, Tt(type), "or arg type not valid", x, y);
	// XRAY_log("or result\n"); XRAY_emit(acc);
	return acc;
}
VP orelse(VP x,VP y) {
	XRAY_log("orelse\n");XRAY_emit(x);XRAY_emit(y);
	if(!IS_EXC(x) && LEN(x)>0) return x;
	else if(NUM(x) && _any(x)) return x;
	else {
		if(CALLABLE(y)) return apply(y,x);
		else return y;
	}
}
VP plus(VP x,VP y) {
	int typerr=-1;
	// XRAY_log("plus\n");XRAY_emit(x);XRAY_emit(y);
	IF_EXC(!SIMPLE(x) || !SIMPLE(y), Tt(type), "plus args should be simple types", x, y); 
	VP acc=ALLOC_BEST(x,y);
	VARY_EACHBOTH(x,y,({
		if(LIKELY(x->t > y->t)) { _x=_x+_y; appendbuf(acc,(buf_t)&_x,1); }
		else { _y=_y+_x; appendbuf(acc,(buf_t)&_y,1); }
		if(!SCALAR(x) && SCALAR(y)) _j=-1; // NB. AWFUL!
	}),typerr);
	IF_EXC(typerr > -1, Tt(type), "plus arg wrong type", x, y);
	// XRAY_log("plus result\n"); XRAY_emit(acc);
	return acc;
}
VP ravel(VP x,VP y) {
	if(!LIST(y) || LEN(y)!=2) return EXC(Tt(type), "ravel y should be [many,single]", x, y);
	VP cond;
	if(LEN(x)!=1 || DICT(x)) cond=LIST_item(y,0);
	else cond=LIST_item(y,1);
	if(CALLABLE(cond)) return apply(cond,x);
	else return cond;
}
static inline VP str2num(VP x) {
	// TODO optimize str2int
	// NB. str2num creates int at the minimum
	double d; I128 buf=0; char* s=sfromxA(flatten(x));
	XRAY_log("str2num %p\n",s);XRAY_emit(x);
	IF_EXC(!IS_c(x),Tt(type),"str2int arg should be char vector",x,0);
	if(strchr(s,'.')!=0 && (d=strtod(s,NULL))!=0) {
		free(s);
		return xf(d);
	} else if(LEN(x)>2 && s[0]=='0' && s[1]=='x') {
		if (sscanf(s,"%llx",&buf)==1) {
			free(s);
			if(buf<MAX_i)
				return xi((CTYPE_i)buf);
			if(buf<MAX_j)
				return xj((CTYPE_j)buf);
			return xo((CTYPE_o)buf);
		} else 
			return x;
	} else if (sscanf(s,"%lld",&buf)==1) { // should probably use atoi or strtol
		free(s);
		if(buf<MAX_i)
			return xi((CTYPE_i)buf);
		if(buf<MAX_j)
			return xj((CTYPE_j)buf);
		return xo((CTYPE_o)buf);
	} 
	else if(strncmp(s,"0.0",3)==0) {
		free(s);
		return xf(0.0);
	} else  {
		free(s);
		return xref(x);
	}
	// return EXC(Tt(value),"str2int value could not be converted",x,0);
}
VP sum(VP x) {
	XRAY_log("sum");XRAY_emit(x);
	I128 val=0;int out,typerr=-1;
	if(UNLIKELY(!SIMPLE(x))) return EXC(Tt(type),"sum argument should be simple types",x,0);
	VARY_EACH(x,({ val += _x; }),typerr);
	IF_EXC(typerr > -1, Tt(type), "sum arg wrong type", x, 0);
	out=BEST_NUM_FIT(val);
	switch(out) {
		CASE(T_b,return xb(val));
		CASE(T_i,return xi(val));
		CASE(T_j,return xj(val));
		default: return xo(val);
	}
}
VP sums(VP x) {
	XRAY_log("sums\n");XRAY_emit(x);
	if(UNLIKELY(!SIMPLE(x))) return EXC(Tt(type),"sums argument should be simple types",x,0);
	VP acc=ALLOC_LIKE(x); int typerr=-1;
	VARY_EACH(x,({ _xtmp += _x; appendbuf(acc,(buf_t)&_xtmp,1); }),typerr);
	IF_EXC(typerr > -1, Tt(type), "sums arg wrong type", x, 0);
	XRAY_log("sums result\n"); XRAY_emit(acc);
	return acc;
}
VP count(VP x) {
	VP acc=0;int i;int typerr=-1;
	XRAY_log("count\n"); XRAY_emit(x);
	VARY_EL_NOFLOAT(x, 0, 
		{ __typeof__(_x) i; acc=xalloc(x->t,MAX(_x,1)); acc->n=_x; for(i=0;i<_x;i++) { EL(acc,__typeof__(i),i)=i; } }, 
		typerr);
	IF_RET(typerr>-1, EXC(Tt(type), "count arg must be numeric", x, 0));
	XRAY_emit(acc);
	return acc;
}
VP range_(int start, int end) {
	int n=ABS(end-start), i, j;
	VP res=xisz(n); 
	int inc=(end<start)?-1:1, realend=end+inc;
	XRAY_log("range_ %d %d %d %d\n", start, end, n, inc);
	j=0;
	for(i=start; i!=realend; i+=inc)
		EL(res,int,j++)=i;
	res->n=j;
	return res;
}
VP range(VP start,VP end) {
	VP res=range_(NUM_val(start),NUM_val(end));
	if(res->t != end->t) res=make(res,end);
	return res;
}
static inline VP times(VP x,VP y) {
	int typerr=-1; VP acc=ALLOC_BEST(x,y);
	//XRAY_log("times\n");XRAY_emit(x);XRAY_emit(y);XRAY_emit(info(acc));
	if(UNLIKELY(!SIMPLE(x))) return EXC(Tt(type),"times argument should be simple types",x,0);
	VARY_EACHBOTH(x,y,({
		XRAY_log("%d %d %d %d\n", _i, _j, _x, _y);
		if(LIKELY(x->t > y->t)) { _x=_x*_y; appendbuf(acc,(buf_t)&_x,1); }
		else { _y=_y*_x; appendbuf(acc,(buf_t)&_y,1); }
		if(!SCALAR(x) && SCALAR(y)) _j=-1; // NB. AWFUL!
	}),typerr);
	IF_EXC(typerr > -1, Tt(type), "times arg wrong type", x, y);
	XRAY_log("times result\n"); XRAY_emit(acc);
	return acc;
}
VP xor(VP x,VP y) { 
	int typerr=-1;
	VP acc;
	XRAY_log("xor\n"); XRAY_emit(x); XRAY_emit(y); // TODO or() and friends should handle type conversion better
	if(UNLIKELY(!SIMPLE(x) && !SIMPLE(y))) return EXC(Tt(type),"xor argument should be simple types",x,0);
	IF_EXC(x->n > 1 && y->n > 1 && x->n != y->n, Tt(len), "xor arguments should be same length", x, y);	
	if(x->t == y->t) acc=xalloc(x->t, x->n);
	else acc=xlsz(x->n);
	VARY_EACHBOTH_NOFLOAT(x,y,({ _x = _x ^ _y; appendbuf(acc, (buf_t)&_x, 1); }), typerr);
	IF_EXC(typerr != -1, Tt(type), "xor arg type not valid", x, y);
	// XRAY_log("or result\n"); XRAY_emit(acc);
	return acc;
}

// MANIPULATING LISTS AND VECTORS (misc):

VP key(const VP x) {
	if(IS_EXC(x)) return x;
	if(DICT(x)||TABLE(x)) return clone(KEYS(x));
	if(IS_x(x)) return clone(KEYS(x)); // locals for context
	if(SIMPLE(x)) return count(xi(x->n));
	return EXC(Tt(type),"key can't operate on that type",x,0);
}
VP val(const VP x) {
	if(TABLE(x) || DICT(x)) return clone(VALS(x));
	if(IS_x(x)) return clone(VALS(x)); // func body for context
	return EXC(Tt(type),"val can't operate on that type",x,0);
}
VP callclass(const VP ctx,const VP verbname,const VP value) {
	ASSERT(DICT(ctx)||IS_x(ctx),"callclass");
	if(!IS_t(verbname)) { return EXC(Tt(type),"callclass verb not name",verbname,value); }
	if(value->tag==0) { return EXC(Tt(type),"callclass value has no class",verbname,value); }
	XRAY_log("callclass\n");XRAY_emit(ctx);XRAY_emit(verbname);XRAY_emit(value);
	VP rootscope;
	if(IS_x(ctx)) rootscope=KEYS(ctx);
	else rootscope=ctx;
	if(!DICT(rootscope)) return 0;
	VP tag=xt(value->tag);
	VP res=apply(rootscope,tag); // need a fast-path dict lookup
	xfree(tag);
	if(res==NULL) return 0;
	res=apply(res,verbname);
	if(res==NULL) return 0;
	return apply(res,value);
}
VP resolvekey(const VP x,const VP y,int checkparents) {
	// printf("get %s from %p\n", reprA(y), x);
	if(!IS_d(x)) return apply(x,y);
	XRAY_log("resolvekey %d\n",checkparents);
	VP key=y, kremainder=0, res;
	int xn=LEN(x),kn=LEN(key); 
	if(LIKELY(key->tag==0 || !TAG_is_class(key->tag)) 
		 && LIKELY(IS_c(key) || (LIST(key) && IS_c(ELl(key,0))))) {
		// convert 'name(['raw("file"),'raw("get")]) to ('file,'get)
		XRAY_log("converting\n");XRAY_emit(y);
		key=str2tag(key); kn=LEN(key);
	}
	if(IS_t(key) && kn>1) {
		key=list(key); // support depth queries with scalars like thing.item
		// if you are doing a depth query, and we are recursing into parents,
		// we need to start the query from the first matching item in a parent;
		// otherwise it is safe to rely on apply() here
		if(!checkparents) {
			XRAY_log("resolvekey depth query\n");XRAY_emit(key);
			return apply(x, key);
		} else {
			kremainder=drop_(key,1); key=first(key);    // separate out the parts
			res=resolvekey(x,key,1);                          // find the parent containing the first part
			if(!IS_EXC(res)) res=apply(res,kremainder); // then seek the rest
			return res;
		}
	} else {
		res=DICT_find(x,key);
		XRAY_emit(res);
		if(!IS_EXC(res)) {
			// printf("get finally found it\n"); if(SIMPLE(res)) { printf("%s\n",reprA(res)); }
			return res;
		}
	}
	if(checkparents) {
		res=DICT_find(x,TTPARENT);
		if(res!=NULL && res!=x) {
			XRAY_log("trying parent\n");
			res=resolvekey(KEYS(res),key,1);
			XRAY_emit(res);
			if(!IS_EXC(res)) {
				return res;
			}
			// if this is an exception, free it because we return our own to maintain context ref:
			else if(res) xfree(res); 
		}
	}
	return EXC(Tt(undef),"undefined",y,x);
}
VP lookup(VP x,VP y) {
	// get is used to resolve names in applyctx(). x is context, y is thing to
	// look up. scans tree of scopes/closures to get value. 
	//
	// in k/q, get is
	// overriden to do special things with files and other handles as well.
	// 
	// in our case, if you pass in ['tag,arg] on the right, it will look up
	// 'tag in the root scope, and then try to call its "get" member. 
	// see _getmodular()
	// printf("get %s in %p\n", reprA(y), x);
	XRAY_log("lookup\n");XRAY_emit(x);XRAY_emit(y);
	if(IS_x(x)) {
		if(x->n != 2) { return EXC(Tt(value),"context not fully formed",x,y); }
		CLASS_call(x,get,y);
		if(IS_t(y) && AS_t(y,0)==Ti(.)) return x;
		else return resolvekey(KEYS(x),y,1);
	}
	else if(IS_d(x)) return resolvekey(x,y,0);
	return apply(x,y);
}
VP get(VP x) {
	return lookup(XXL_CUR_CTX,x);
}
VP set(VP ctx,VP key,VP val) {
	// printf("set %s in %p:%s\n", reprA(key), ctx, reprA(val));
	XRAY_log("set in %p\n",ctx);XRAY_emit(ctx);XRAY_emit(key);XRAY_emit(val);
	if(val==NULL) return val;
	if(!IS_t(key)) return EXC(Tt(type),"set val must be symbol",key,val);
	if(IS_t(key) && key->n > 1) { 
		XRAY_log("set at depth\n");
		key=list(key);
	}
	ARG_MUTATING(ctx);
	VP dest=KEYS(ctx);
	XRAY_log("set assigning in %p\n", dest);XRAY_emit(dest);
	// dest=assign(dest,key,clone(val));
	// if(!IS_x(val)) val=clone(val);
	// val=clone(val);
	dest=assign(dest,key,val);
	// XRAY_log("set dest=%p val=%p\n",dest,val);
	// XRAY_emit(dest);
	return val;
}
VP set_as(VP x,VP y) {
	// TODO set needs to support nesting
	int i; VP res,ctx,val;
	XRAY_log("set_as\n");XRAY_emit(x);XRAY_emit(y);
	if(x==NULL || y==NULL) return x;
	return set(XXL_CUR_CTX, y, x);
}
VP set_is(VP x,VP y) {                   // exactly like set, but arguments are [[ctx,name],[value]]
	XRAY_log("set_is\n");
	if(x==NULL || y==NULL) return x;
	return set(XXL_CUR_CTX, x, y);
}
VP numelem2base(VP num,int i,int base) {
	// NB. this is called from repr_o. NO XRAY_emit() ALLOWED!
	if(!NUM(num)) return EXC(Tt(type),"numelem2base can't operate on that type",num,0);
	char ch[]=CH_SET_na;
	if(base > sizeof(ch)) return EXC(Tt(value),"numelem2base can't produce that base",num,0);
	if(i > LEN(num)) return EXC(Tt(value),"numelem2base bad index",num,xi(i));
	if(IS_f(num)) {
		char buf[20];
		snprintf(buf,20,"%0.04f",num);
		return xfroms(buf);
	}
	int typerr=-1; I8 buf; I128 rem; 	
	VARY_EL_NOFLOAT(num,i,({ rem = _x; }),typerr);
	VP res=xcsz(5);
	do {
		// XRAY_log("%lld\n", rem);
		buf=ch[rem % base];
		appendbuf(res,&buf,1);
		rem = rem / base;
	} while (rem>0);
	return reverse(res);
}
VP str(VP x) {
	XRAY_log("str\n");XRAY_emit(x);
	if(IS_c(x)) {
		VP res=clone(x);
		res->tag=0;
		return res;
	}
	if(NUM(x) || IS_t(x)) {
		int xn=LEN(x), i;
		VP acc=xlsz(xn),final;
		if(NUM(x)) for(i=0; i<xn; i++) acc=append(acc,numelem2base(x,i,10));
		else if(IS_t(x)) for(i=0; i<xn; i++) acc=append(acc,tagname(AS_t(x,i)));
		final=join(acc,xfroms(","));
		return final;
	}
	return repr(x);
}
VP sys(VP x) {
	XRAY_log("sys\n");XRAY_emit(x);
	if(XXL_SYS) {
		if(x==NULL || EMPTYLIST(x)) return clone(XXL_SYS); 
		else return DICT_find(XXL_SYS,x);
	}
	return NULL;
}
VP partgroups(VP x) { 
	// separate 1 3 4 5 7 8 -> [1, 3 4 5, 7 8]; always returns a list, even with one item
	VP acc,tmp;int n=0,typerr=-1;
	acc=xlsz(x->n/2);
	tmp=xalloc(x->t,4);
	VARY_EACHLIST(x,({
		if(_i==0) {
			_xtmp=_x;
		} else {
			// XRAY_log("pg inner %d %d\n", _xtmp, _x);
			if(ABS(_xtmp-_x) != 1) {
				acc=append(acc,tmp);
				xfree(tmp);
				tmp=xalloc(x->t,4);
			} 
			_xtmp=_x;
		}
		tmp=appendbuf(tmp,(buf_t)&_x,1);
	}),typerr);
	IF_EXC(typerr>-1, Tt(type), "partgroups args should be simple types", x, 0); 
	if(tmp->n) append(acc,tmp);
	// XRAY_log("partgroups returning\n");XRAY_emit(acc);
	xfree(tmp);
	return acc;
}

VP pick(VP x,VP y) { // select items of x[0..n] where y[n]=1
	VP acc;int n=0,typerr=-1;
	IF_EXC(!SIMPLE(x) || !SIMPLE(y), Tt(type), "pick args should be simple types", x, y); // TODO pick: gen lists
	XRAY_log("pick\n");XRAY_emit(x);XRAY_emit(y);
	acc=ALLOC_LIKE_SZ(x,x->n/2);
	VARY_EACHBOTHLIST(x,y,({
		if(_y) {
			// XRAY_log("p %d %d/%d %d %d/%d s%d\n", _x,_i,_xn,_y,_j,_yn,SCALAR(x));
			acc=appendbuf(acc,(buf_t)&_x,1);
			n++;
		}
	}),typerr);
	IF_EXC(typerr > -1, Tt(type), "pick arg wrong type", x, y);
	XRAY_log("pick result\n"); XRAY_emit(acc);
	return acc;
}
VP pickapart(VP x,VP y) { // select items of x[0..n] where y[n]=1, and divide non-consecutive regions
	VP acc, sub=NULL;int n=0,typerr=-1;
	IF_EXC(!SIMPLE(x) || !SIMPLE(y), Tt(type), "pickapart args should be simple types", x, y); // TODO pick: gen lists
	XRAY_log("pickapart\n");XRAY_emit(x);XRAY_emit(y);
	acc=xlsz(4);
	VARY_EACHBOTHLIST(x,y,({
		XRAY_log("%d ",_y);
		if(_y) {
			if (!sub) sub=ALLOC_LIKE_SZ(x,x->n/2);
			sub=appendbuf(sub,(buf_t)&_x,1);
		} else {
			if (sub) { acc=append(acc,sub); xfree(sub); sub=NULL; }
		}
	}),typerr);
	IF_EXC(typerr > -1, Tt(type), "pickapart arg wrong type", x, y);
	if (sub) { acc=append(acc,sub); xfree(sub); sub=NULL; }
	XRAY_log("pickapart result\n"); XRAY_emit(acc);
	if(acc->n==0) return ELl(acc,0);
	else return acc;
}
VP proj(int type, void* func, VP left, VP right) {
	Proj p;
	VP pv=xpsz(1);
	p.type=type;
	if(type<0) p.ctx=xref(func);
	else if(type==1) p.f1=func; 
	else p.f2=func;
	p.left=left; p.right=right;
	if(left) xref(left); if(right) xref(right); 
	EL(pv,Proj,0)=p;
	pv->n=1;
	return pv;
}
VP unionn(VP x,VP y) {
	XRAY_log("unionn\n");XRAY_emit(x);XRAY_emit(y);
	if(IS_EXC(x)) return x; if(IS_EXC(y)) return y;
	if(DICT(x) && DICT(y)) return catenate(x, y);
	return EXC(Tt(nyi), "union not yet implemented for that type", x, y);
}
VP xray(VP x) {
	XRAY_log("xray\n");XRAY_emit(x);
	if(!_any(x)) {
		XRAY_LVL=0;
		return Tt(xrayoff);
	} else {
		XRAY_LVL=2;
		return Tt(xrayon);
	}
}

// TAG STUFF:

static inline VP str2tag(VP str) { // turns string, or list of strings, into tag vector
	int i=0; VP acc=xtsz(str->n);
	// XRAY_log("str2tag\n");XRAY_emit(str);
	if(IS_c(str)) return xt(_tagnum(str));
	if(LIST(str)) {
		VP tmp, tagtmp;
		for(;i<str->n;i++) {
			tmp=ELl(str,i);
			if(!IS_c(tmp)) return EXC(Tt(type),"str2tag arg not string or list of strings",str,0);
			tagtmp=xt(_tagnum(tmp));
			// XRAY_log("append tag\n");XRAY_emit(tagtmp);
			acc=append(acc,tagtmp);
		}
		// XRAY_log("str2tag returning\n");XRAY_emit(acc);
		return acc;
	}
	return EXC(Tt(type),"str2tag arg not string or list of strings",str,0);
}
VP entag(VP x,VP t) {
	if(IS_c(t)) x->tag=_tagnum(t);
	else if(IS_i(t)) x->tag=AS_i(t,0);
	else if(IS_t(t)) x->tag=AS_t(t,0);
	return x;
}
VP entags(VP x,const char* name) {
	x->tag=_tagnums(name);
	return x;
}
VP tag(VP x) {                         // return tag of x
	return x->tag==0?xt0():xt(x->tag);
}
static inline VP tagname(const tag_t tag) {
	char buf[256]={0};
	memcpy(buf,&tag,sizeof(tag));
	return xfroms(buf);
}
static inline const char* tagnames(const tag_t tag) {
	char* buf = malloc(256);
	memcpy(buf,&tag,sizeof(tag));
	return buf;
}
static inline tag_t _tagnum(const VP s) {
	int i; VP ss=0;
	tag_t res=0;
	ASSERT(IS_c(s),"_tagnum(): non-string argument");
	memcpy(&res,BUF(s),MIN(s->n,sizeof(res)));
	return res;
}
/* static inline  */
inline tag_t _tagnums(const char* name) {
	tag_t res=0;
	memcpy(&res,name,MIN(strlen(name),sizeof(res)));
	return res;
}

// JOINS (so to speak)
// possibly useless

VP bracketjoin(VP x,VP y) { 
	// returns x[n] when 'on'
	//  turned on by y[0][n]=1
	//  turned off by y[1][n]=1
	// otherwise 0
	// useful for matching patterns involving more than one entity
	int i,ctr=0,maxx=0,on=0,typerr=-1; VP y0,y1,res,emptyset;
	// XRAY_log("bracketjoin\n");XRAY_emit(x);XRAY_emit(y);
	if(!LIST(y) || y->n!=2) return EXC(Tt(type),"bracketjoin y must be 2-arg list",x,y);
	y0=ELl(y,0); y1=ELl(y,1);
	if(y0->t != y1->t) return EXC(Tt(type),"bracketjoin y items must be same type",x,y);
	// find the most tightly nested group by counting nesting depth
	for(i=0;i<y0->n;i++) {
		if(EL(y0,CTYPE_b,i)==1)
			ctr++;
		if(ctr>maxx) maxx=ctr;
		if(EL(y1,CTYPE_b,i)==1) ctr=MAX(0,ctr-1);
	}
	emptyset=take(XI0,xi(y0->n));
	if(maxx==0) { return emptyset; } // no nestable pairs
	ctr=0; res=xbsz(x->n); CTYPE_b b0=0,b1=1;
	for(i=0;i<y0->n;i++) {
		if(EL(y0,CTYPE_b,i)==1) ctr++;
		if(EL(y1,CTYPE_b,i)==1) ctr=MAX(0,ctr-1);
		// XRAY_log("%d %d %d\n", i, ctr, maxx);
		if(ctr==maxx) {
			appendbuf(res,(buf_t)&EL(x,CTYPE_b,i%x->n),1);
		} else
			appendbuf(res,(buf_t)&b0,1);
	}
	// XRAY_log("after scanning for maxx\n"); XRAY_emit(res);
	VP c=ELl(partgroups(condense(res)),0);
	// XRAY_log("partgroups result\n"); XRAY_emit(c);
	if(c->n) {
		c=append(c,plus(max(c),xi(1)));
		// XRAY_log("bracket append next\n"); XRAY_emit(c);
	}
	res=assign(take(XI0,xi(y0->n)),c,xi(1));
	// XRAY_log("bracketjoin returning\n");XRAY_emit(res);
	return res;
}
VP consecutivejoin(VP x, VP y) {
	// returns x[n] if y[0][n]==1 && y[1][n+1]==1 && .. else 0
	int j,n=y->n, typerr=-1, on=0; VP acc,tmp;
	// XRAY_log("consecutivejoin\n"); XRAY_emit(x); XRAY_emit(y);
	if(!LIST(y)) return and(x,y);
	IF_EXC(!LIST(y)||y->n<1,Tt(type),"consecutivejoin y must be list of simple types",x,y);
	VP y0=ELl(y,0);
	for(j=0; tmp=ELl(y,j), j<n; j++) 
		IF_EXC(tmp->t!=y0->t,Tt(type),"consecutivejoin y must all be same type or similar numeric",x,y);
	acc = ALLOC_LIKE_SZ(x,y0->n);
	VARY_EACHRIGHT(x,y0,({
		if(UNLIKELY(_y)==1) {
			on=1;
			for(j=0; tmp=ELl(y,j), j<n; j++) {
				if(_j + j > tmp->n || EL(tmp,typeof(_y),_j+j) == 0){on = 0; break;}
			} 
			if(on) EL(acc,typeof(_x),_j)=EL(x,typeof(_x),_j % x->n);
			else EL(acc,typeof(_x),_j)=0; 
		}
	}),typerr);
	IF_EXC(typerr>-1,Tt(type),"consecutivejoin couldnt work with those types",x,y);
	acc->n=y0->n;
	XRAY_log("consecutivejoin return\n"); XRAY_emit(acc);
	return acc;
}
VP signaljoin(VP x,VP y) {
	// could be implemented as +over and more selection but this should be faster
	int typerr=-1, on=0; VP acc;
	// XRAY_log("signaljoin\n");XRAY_emit(x);XRAY_emit(y);
	acc = ALLOC_LIKE_SZ(x,y->n);
	if(SCALAR(x)) { // TODO signaljoin() should use take to duplicate scalar args.. but take be broke
		VARY_EACHRIGHT(x,y, ({
			if(_y == 1) on=!on;
			if(on) EL(acc,typeof(_x),_j)=(typeof(_x))_x;
			else EL(acc,typeof(_x),_j)=0;
		}), typerr);
	} else {
		VARY_EACHBOTHLIST(x,y,({
			if(_y == 1) on=!on;
			if(on) EL(acc,typeof(_x),_i)=(typeof(_x))_x;
			else EL(acc,typeof(_x),_i)=0;
		}),typerr);
	}
	acc->n=y->n;
	IF_EXC(typerr>-1,Tt(type),"signaljoin couldnt work with those types",x,y);
	XRAY_log("signaljoin return\n");XRAY_emit(acc);
	return acc;
}

// MATCHING

VP nest(VP x,VP y) {
	VP p1,p2,open,close,opens,closes,where,rep,out,base;

	renest:

	XRAY_log("NEST\n");XRAY_emit(x);XRAY_emit(y);
	//if(!LIST(x) || x->n < 2) return x;
	if(x->n<2)return x;
	p1=proj(2,&matcheasy,x,0);
	p2=proj(2,&matcheasy,x,0);
	open=apply_simple_(y,0); close=apply_simple_(y,1);
	if(!LIST(x) && x->t != open->t) return x;
	// if(LIST(open) && x->t != AS_l(open,0)->t) return x;
	if(_equal(open,close)) {
		opens=each(open,p1);
		XRAY_log("+ matching opens\n");XRAY_emit(opens);
		if(_any(opens)) {
			VP esc = 0;
			if(y->n >= 3 && LEN(LIST_item(y,2))!=0) {
				esc = matcheasy(x,LIST_item(y,2));
				XRAY_log("escapes\n");XRAY_emit(esc);
				EL(opens,VP,0)=and(AS_l(opens,0),shift_(not(esc),-1));
				XRAY_log("new escaped opens\n");
			}	
			if(open->tag) base=matchtag(x,xt(open->tag));
			else base=xb(1);
			opens=signaljoin(base,AS_l(opens,0));
			xfree(base);
			XRAY_log("after signaljoin\n");XRAY_emit(opens);
			out=partgroups(condense(opens));
			if(out->n) {
				out=AS_l(out,0);
				XRAY_log("matching pre-append"); XRAY_emit(out);
				out=append(out,plus(max(out),xi(1)));
				XRAY_log("matching post-append"); XRAY_emit(out);
			}
			XRAY_emit(out);
			where=out;
		} else 
			where=xl0();
		// exit(1);
	} else {
		opens=each(open,p1);
		XRAY_log("+ opens\n");XRAY_emit(opens);
		closes=each(close,p2);
		XRAY_log("- closes\n");XRAY_emit(closes);
		if(LIST(opens)) {
			if (!AS_b(any(AS_l(opens,0)),0)) return x;
		} else if(!AS_b(any(opens),0)) return x;
		if(LIST(closes)) {
			if (!AS_b(any(AS_l(closes,0)),0)) return x;
		} else if(!AS_b(any(closes),0)) return x;
		// remember that bracket join needs the farthest-right
		// index of the matching closing bracket, if it's more than
		// one item
		closes=consecutivejoin(xb(1),closes);
		if(close->n > 1)
			closes=shift_(closes, (close->n-1)*-1);
		base=xb(1);
		out=bracketjoin(base,xln(2,consecutivejoin(base,opens),closes)); 
		xfree(base);
		where=condense(out);
	}
	XRAY_log("nest where\n");XRAY_emit(where);
	if(where->n) {
		rep=apply(x,where);
		if(y->n >= 5 && LEN(LIST_item(y,4))!=0)
			rep=apply(LIST_item(y,4),rep);
		if(y->n >= 4 && LEN(LIST_item(y,3))!=0)
			rep->tag=AS_t(LIST_item(y,3),0);
		rep=list2vec(rep);
		XRAY_log("nest rep\n");XRAY_emit(rep);
		// splice is smart enough to merge like-type replace args into one
		// like-typed vector. but that's not what we want here, because the
		// thing we're inserting is a "child" of this position, so we want to
		// ensure we always splice in a list
		XRAY_log("nest x\n");
		out=splice(split(x,xi0()),where,rep);
		if(x->tag) out->tag=x->tag;
		XRAY_log("nest out\n");XRAY_emit(out);
		if(!LIST(out)) out=xl(out);
		x=out;
		goto renest;
	} else { out = x; }
	XRAY_log("nest returning\n"); XRAY_emit(out);
	return out;
}
VP matchany(VP obj,VP pat) {
	IF_EXC(!SIMPLE(obj) && !LIST(obj) && !TABLE(obj),Tt(type),"matchany only works with simple or list types in x",obj,pat);
	// IF_EXC(!SIMPLE(pat),Tt(type),"matchany only works with simple types in y",obj,pat);
	IF_EXC(SIMPLE(obj) && obj->t != pat->t, Tt(type),"matchany only works with matching simple types",obj,pat);
	int j,n=obj->n,typerr=-1;VP item, acc;
	XRAY_log("matchany\n"); XRAY_emit(obj); XRAY_emit(pat);
	if(CALLABLE(pat)) return each(obj, pat);
	acc=xbsz(n); 
	acc->n=n;
	if(LIST(obj)) {
		VP this;
		FOR(0,n,({ 
			this=ELl(obj,_i);
			if((pat->tag==0 || pat->tag==this->tag) && _find1(pat,this) != -1) {
				// XRAY_log("matchany found list at %d\n", _i);
				EL(acc,CTYPE_b,_i)=1; }}));
	} else {
		VARY_EACHLEFT(obj, pat, ({
			// TODO matchany(): buggy subscripting:
			if((pat->tag==0 || pat->tag==obj->tag) && _findbuf(pat, (buf_t)&_x) != -1) {
				// XRAY_log("matchany found simple at %d\n", _i);
				EL(acc,CTYPE_b,_i) = 1;
			}
		}), typerr);
		IF_EXC(typerr>-1, Tt(type), "matchany could not match those types",obj,pat);
	}
	XRAY_log("matchany result\n"); XRAY_emit(acc);
	return acc;
}
VP matcheasy(VP obj,VP pat) {
	IF_EXC(!SIMPLE(obj) && !LIST(obj) && !TABLE(obj),Tt(type),
		"matcheasy only works with simple types, lists, and tables in x",obj,pat);
	int j,n=obj->n,typerr=-1;VP item, acc;
	XRAY_log("matcheasy\n"); XRAY_emit(obj); XRAY_emit(pat);
	if(CALLABLE(pat)) return each(obj, pat);
	if(TABLE(obj)) {
		if(!CALLABLE(pat)) return EXC(Tt(type),"tables can only be matched with functions",obj,pat);
		return eachtable(obj,pat);
	}
	acc=xbsz(n); // TODO matcheasy() should be smarter about initial buffer size
	acc->n=n;
	if(LIST(obj)) {
		FOR(0,n,({ 
			VP item = ELl(obj,_i);
			if((pat->tag == 0 || pat->tag==item->tag) && _equal(item,pat)) 
				EL(acc,CTYPE_b,_i)=1; }));
	} else {
		VARY_EACHLEFT(obj, pat, ({
			if(_x == _y) {
				j=0;
				do { EL(acc,CTYPE_b,_i+j) = 1; j++;
				} while (_i+j<n && j<_yn && _equalm(obj,_i+j,pat,j));
			}
		}), typerr);
		IF_EXC(typerr>-1, Tt(type), "matcheasy could not match those types",obj,pat);
	}
	XRAY_log("matcheasy result\n"); 
	XRAY_emit(info(acc));
	XRAY_emit(acc);
	return acc;
}
VP matchexec(VP obj,VP pats) {
	int i,j,diff;VP rule,res,res2,sel;
	ASSERT(LIST(pats)&&pats->n%2==0,"pats should be a list of [pat1,fn1,pat2,fn2..]");
	XRAY_log("matchexec start\n");
	XRAY_emit(obj);
	XRAY_emit(pats);
	for(i=0;i<pats->n;i+=2) {
		XRAY_log("matchexec %d\n", i);
		rule=apply_simple_(pats,i);
		if(IS_t(rule)) res=matchtag(obj,rule);
		else res=matchany(obj,rule);
		XRAY_log("matchexec match, rule and res:\n");
		XRAY_emit(rule);
		XRAY_emit(res);
		// rules start with an unimportant first item: empty tag for tagmatch
		if(_any(res)) {
			VP cond=condense(res);
			VP indices=partgroups(cond);
			xfree(cond);
			diff = 0;
			for (j=0; j<indices->n; j++) {
				VP idx = LIST_item(indices, j);
				XRAY_log("matchexec idx, len=%d, diff=%d\n", idx->n, diff); XRAY_emit(idx);
				VP diffi=xi(diff);
				VP newidx=plus(idx,diffi);
				xfree(diffi);
				VP objelem=apply(obj,newidx);
				VP handler=LIST_item(pats,i+1);
				res2=apply(handler,objelem);
				xfree(objelem);
				if(LIST(res2) && res2->n == 0) continue;
				XRAY_log("matchexec after apply, len=%d\n", res2->n); XRAY_emit(res2);
				obj=splice(obj,newidx,res2);
				diff += 1 - idx->n;
				XRAY_log("matchexec new obj, diff=%d", diff); XRAY_emit(obj);
				// xfree(res2);
				xfree(newidx);
				// idx isnt a reference, dont free it.
			}
			xfree(indices);
		}
	}	
	XRAY_log("matchexec done\n"); XRAY_emit(obj);
	return obj;
}
VP matchtag(const VP obj,const VP pat) {
	IF_EXC(!SIMPLE(obj) && !LISTDICT(obj),Tt(type),"matchtag only works with simple types or lists",obj,pat);
	IF_EXC(!IS_t(pat), Tt(type), "matchtag y must be tag",obj,pat);
	int j,n,typerr=-1;VP searchobj, item, acc;
	XRAY_log("matchtag\n"); XRAY_emit(obj); XRAY_emit(pat);

	if(DICT(obj)) searchobj=VALS(obj);
	else searchobj=obj;

	n=searchobj->n;

	acc=xbsz(n); // TODO matcheasy() should be smarter about initial buffer size
	acc->n=n;
	if(LIST(searchobj)) {
		FOR(0,n,({ 
			if(AS_t(pat,0) == ELl(searchobj,_i)->tag) 
				EL(acc,CTYPE_b,_i)=1; }));
	} else {
		VARY_EACHLEFT(searchobj, pat, ({
			if(AS_t(pat,0) == searchobj->tag) EL(acc,CTYPE_b,_i) = 1;
		}), typerr);
		IF_EXC(typerr>-1, Tt(type), "matchtag could not match those types",searchobj,pat);
	}
	XRAY_log("matchtag result\n"); XRAY_emit(acc);
	return acc;
}

// CONTEXTS:

VP rootctx() {
	VP res;
	res=xd0();
	#include"rootctx.h"
	return res;
}
VP mkbarectx() {
	return xx0();
}
VP mkworkspace() {
	char name[8];
	VP root,res,locals;
	snprintf(name,sizeof(name),"wk%-6d", rand());
	res=xxsz(2); res->n=2;
	ELl(res,0)=rootctx(); ELl(res,1)=xl0();
	return res;
}
VP eval(VP code) {
	ASSERT(1, "eval nyi");
	return NULL;
}
VP list2vec(VP obj) {
	// Collapses lists that contain all the same kind of vector items into a
	// single vector [1,2,3i] = (1,2,3i) Will NOT collapse [1,(2,3),4] - use
	// flatten for this. (See note below for non-flat first items) The original
	// list will be returned when rejected for massaging.
	int i, t=0;
	VP acc,this;
	// XRAY_log("list2vec\n"); XRAY_emit(obj);
	if(!LIST(obj)) return obj;
	if(!obj->n) return obj;
	acc=ALLOC_LIKE(ELl(obj,0));
	if(obj->tag!=0) { t=obj->tag; acc->tag=obj->tag; }
	FOR(0,obj->n,({ this=ELl(obj,_i);
		// bomb out on non-scalar items or items of a different type than the first
		// note: we allow the first item to be nonscalar to handle the list2vec
		// [(0,1)] case - this is technically not a scalar first item, but clearly
		// it should return (0,1)
		if((t != 0 && this->tag != t)
			 || (_i > 0 && !SCALAR(this)) || this->t != acc->t){xfree(acc); return obj; } 
		else append(acc,this); 
		if(this->tag) t=this->tag;
	}));
	XRAY_log("list2vec result\n"); XRAY_emit(acc); 
	return acc;
}
VP labelitems(VP label,VP items) {
	VP res;char* labelbuf=sfromxA(label);
	//XRAY_log("labelitems\n");XRAY_emit(label);XRAY_emit(items);
	res=flatten(items);res->tag=_tagnums(labelbuf);
	free(labelbuf);
	//XRAY_emit(res);
	return res;
}
VP mklexer(const char* chars, const char* label) {
	return xln(2,
		entags(xfroms(chars),"raw"),
		proj(2,&labelitems,xfroms(label),0)
	);
}
VP parseexpr(VP x) {
	int i;
	XRAY_log("parseexpr\n");XRAY_emit(x);
	if(!LIST(x)) return x;               // confuzzling..
	if(IS_c(ELl(x,0)) && AS_c(ELl(x,0),0)=='[') {
		if(LEN(x)==2) return xl0();        // empty
		if(LEN(x)==3 && IS_c(ELl(x,1)) && AS_c(ELl(x,1),0)==':')
			return dict(xl0(),xl0()); // empty
		VP res=xlsz(LEN(x)+1);
		res=append(res,xl0());
		if(LEN(x)>2) {
			res=append(res,entags(xc(','),"raw"));
			for(i=1;i<LEN(x)-1;i++) 
				res=append(res,ELl(x,i));
		}
		return res;
	}
	if(LIST(x) && IS_c(ELl(x,0)) && 
			((AS_c(ELl(x,0),0)=='(') ||
			  AS_c(ELl(x,0),0)=='['))
		return drop_(drop_(x,-1),1);
	else
		return x;
}
VP parsename(VP x) {
	XRAY_log("parsename\n");XRAY_emit(x);
	VP res=flatten(x);
	if(IS_c(res)) {
		if(AS_c(res,0)=='\'') {
			XRAY_log("parsename tag\n");
			res=behead(res);
			res=split(res,xc('.')); // very fast if not found
			XRAY_emit(res);
			return str2tag(res);
		} else {
			XRAY_log("parsename non-tag\n");
			if(res->n==1 && AS_c(res,0)=='.')
				return entags(Tt(.),"name");
			else {
				res=split(res,xc('.')); // very fast if not found
				res->tag=Ti(name);
			}
		}
	}
	return res;
}
VP parsenum(VP x) {
	XRAY_log("parsenum\n");XRAY_emit(x);
	VP res=flatten(x);
	if(IS_c(res)) return str2num(res);
	else return res;
}
VP parselambda(VP x) {
	int i,arity=0,typerr=-1,tname=Ti(name),traw=Ti(raw); VP this;
	XRAY_log("parselambda\n");XRAY_emit(x);
	x=list(x);
	for(i=0;i<x->n;i++) {
		this=ELl(x,i); // not alloced, no need to free
		if(IS_c(this) && this->tag==tname && AS_c(this,0)=='x') 
			arity=1;
		if(IS_c(this) && this->tag==tname && AS_c(this,0)=='y') { 
			arity=2;break;
		}
	};
	if(LIST(x) && IS_c(ELl(x,0)) && AS_c(ELl(x,0),0)=='{')
		return entags(xln(2,drop_(drop_(x,-1),1),xi(arity)),"lambda");
	else return x;
}
VP parsecomment(VP x) {
	// strip tags from x
	int i; VP item;
	ARG_MUTATING(x);
	for(i=0; i<LEN(x); i++) {
		item=ELl(x,i);
		if(item->tag==Ti(raw)) item->tag=0;
	}	
	VP res=list2vec(x);
	res->tag=Ti(comment);
	return res;
}
VP parsestrlit(VP x) {
	int i,arity=1,typerr=-1,traw=Ti(raw);
	// XRAY_log("PARSESTRLIT!!!\n");XRAY_emit(x);
	if(LIST(x) && IS_c(AS_l(x,0)) && AS_c(AS_l(x,0),0)=='"') {
		VP res=xlsz(x->n), el, next; int ch,nextch,last;
		last=x->n-1;
		for(i=0;i<x->n;i++) {
			// XRAY_log("parsestrlit #%d/%d\n",i,last);
			el=AS_l(x,i);
			if(!el) continue;
			XRAY_emit(el);
			if(IS_c(el)) {
				ch=AS_c(el,0);
				// XRAY_log("parselit ch=%c\n",ch);
				if ((i==0 || i==last) && ch=='"')
					continue; // skip start/end quotes
				if (i<last &&
				    (ch=AS_c(el,0))=='\\') {
					// XRAY_log("investigating %d\n",i+1);
					next=AS_l(x,i+1);
					if(IS_c(next) && next->n) {
						nextch=AS_c(next,0);
						if(nextch=='n') {
							res=append(res,xc(10)); i++;
						} else if(nextch=='r') {
							res=append(res,xc(13)); i++;
						} else if(nextch=='t') {
							res=append(res,xc(9)); i++;
						}
					}
				} else  
					res=append(res,el);
			} else 
				res=append(res,el);
		}
		// due to the looping logic, we would wind up with an empty list - we want an empty list with an empty string! :)
		if(res->n==0) res=append(res,xc0()); 
		// XRAY_log("flattenin\n");XRAY_emit(res);
		res=flatten(res);
		// XRAY_emit(res);
		return res;
	} else {
		XRAY_log("parsestrlit not interested in\n");
		XRAY_emit(x);
		return x;
	}
}
VP parseloopoper(VP x) {
	XRAY_log("parseloopoper\n");XRAY_emit(x);
	VP op=xfroms("~!@#$%^&*|:\\/<>~',."); op->tag=Ti(raw);
	VP tmp2=matchany(x,op);
	if (!_any(tmp2)) { xfree(op); xfree(tmp2); return x; }
	VP cat=xfroms(":|>"); cat->tag=Ti(raw); 
	VP tmp1=matchany(x,cat); 
	if (!_any(tmp1)) { xfree(cat); xfree(op); xfree(tmp2); xfree(tmp1); return x; }
	VP tmp3=xln(2,tmp2,tmp1);
	VP join=consecutivejoin(XB1,tmp3);
	if(_any(join)) {
		int diff=0, j=0; VP idx,rep,indices=partgroups(condense(join));
		for (; j<indices->n; j++) {  // probably needs an abstraction
			idx = plus(ELl(indices, j), xi(diff));
			idx = append(idx, plus(idx,XI1));
			rep = list2vec(apply(x, idx));
			rep->tag = Ti(oper);
			x=splice(x, idx, rep);
			diff += 1 - idx->n;
			xfree(idx); xfree(rep);
		}
		xfree(indices); 
	}
	xfree(cat); xfree(op); xfree(tmp1); xfree(tmp2); xfree(tmp3); xfree(join);
	return x;
}
VP parseallexprs(VP tree) {
	if(IS_EXC(tree) || !LIST(tree)) return tree;
	XRAY_log("parseallexprs\n");XRAY_emit(tree);
	VP brace=xfroms("{"), paren=xfroms("("), bracket=xfroms("[");
	if(_find1(tree,brace)!=-1)
		tree=nest(tree,xln(5, entags(brace,"raw"), entags(xfroms("}"),"raw"), xfroms(""), Tt(lambda), x1(&parselambda)));
	if(_find1(tree,paren)!=-1)
		tree=nest(tree,xln(5, entags(paren,"raw"), entags(xfroms(")"),"raw"), xfroms(""), Tt(expr), x1(&parseexpr)));
	if(_find1(tree,bracket)!=-1)
		tree=nest(tree,xln(5, entags(bracket,"raw"), entags(xfroms("]"),"raw"), xfroms(""), Tt(listexpr), x1(&parseexpr)));
	return tree;
}
VP resolve(VP ctx,VP ptree) {
	XRAY_log("resolve\n");XRAY_emit(ctx);XRAY_emit(ptree);
	if(!IS_x(ctx) && !LIST(ptree)) return EXC(Tt(type),"resolve",ctx,ptree);
	VP name; int i, tname=Ti(name), traw=Ti(raw);
	for(i=0;i <LEN(ptree); i++) {
		name=LIST_item(ptree,i);
		if(IS_c(name) && (name->tag==tname || name->tag==traw)) {
			VP tag=xt(_tagnum(name));
			VP val=DICT_find(KEYS(ctx),tag);
			if(val!=NULL) {
				// printf("resolve replacing %s with %s\n",reprA(name),reprA(val));
				if(!SIMPLE(val)) val=entag(val,tag);  //hmm
				EL(ptree,VP,i)=xref(val);
				// printf("resolved\n");XRAY_emit(val); 
			} else {
				// printf("couldnt resolve %s\n",reprA(tag));XRAY_emit(tag);
			}
			xfree(tag);
		}
	}
	XRAY_log("returning\n");XRAY_emit(ptree);
	return ptree;
}
VP parseresolvestr(const char* str,VP ctx) {
	VP lex,pats,acc,t1,t2;size_t l=strlen(str);int i;
	XRAY_log("parsestr '%s'\n",str);
	if(l==0) return NULL;
	acc=xlsz(l);
	for(i=0;i<l;i++)
		append(acc,entags(xc(str[i]),"raw"));
	if(AS_c(ELl(acc,acc->n - 1),0)!='\n')
		append(acc,entags(xc('\n'),"raw"));	
	acc=nest(acc,xln(5, entags(xfroms("\""),"raw"), xfroms("\""), xfroms("\\"), Tt(string), x1(&parsestrlit)));
	acc=nest(acc,xln(5, entags(xfroms("//"),"raw"), xfroms("\n"), xfroms(""), Tt(comment), x1(&parsecomment)));
	acc=nest(acc,xln(5, entags(xfroms("/*"),"raw"), xfroms("*/"), xfroms(""), Tt(comment), x1(&parsecomment)));
	acc=parseloopoper(acc);
	pats=xl0();
	lex=mklexer("0123456789.","num");
	append(pats,ELl(lex,0));
	append(pats,x1(&parsenum));
	xfree(lex);
	lex=mklexer("'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_.?","name");
	append(pats,ELl(lex,0));
	append(pats,x1(&parsename));
	xfree(lex);
	lex=mklexer(" \n\t","ws");
	append(pats,ELl(lex,0));
	append(pats,ELl(lex,1));
	xfree(lex);
	t1=matchexec(acc,pats);
	xfree(pats);
	XRAY_log("matchexec results\n");XRAY_emit(t1);
	if(ctx!=NULL) t2=resolve(ctx,t1);
	else t2=t1;
	t2=wide(t2,x1(&parseallexprs));
	return t2;
}
VP parsestr(const char* str) {
	return parseresolvestr(str,NULL);
}

VP parse(VP x) {
	char* buf=sfromxA(x);
	VP res=parsestr(buf);
	free(buf);
	return res;
}

// THREADING

void thr_start() {
	// TODO threading on Windows
	#ifndef THREAD
	#else
	NTHR=0;
	#endif
	return;
}
void* thr_run0(void* VPctxargs) {
	#ifndef THREAD
	#else
	init_thread_locals();
	VP val=(VP)VPctxargs;
	VP ctx=LIST_item(val,0);
	VP arg=LIST_item(val,1);
	ctx=apply(ctx,arg);
	// printf("thr_run0 done\n"); XRAY_emit(ctx);
	xfree(val);
	pthread_exit(NULL);
	#endif
	return 0;
}
void thr_run1(VP ctx,VP arg) {
	#ifndef THREAD
	apply(ctx,arg);
	#else
	VP ctx_with_args=xln(2, ctx, arg);
	pthread_attr_t a; pthread_attr_init(&a); pthread_attr_setdetachstate(&a, PTHREAD_CREATE_JOINABLE);
	// nthr=sysconf(_SC_NPROCESSORS_ONLN);if(nthr<2)nthr=2;
	WITHLOCK(thr,if(pthread_create(&THR[NTHR++], &a, &thr_run0, ctx_with_args)!=0)PERR("pthread_create"));
	#endif
}
void thr_run(VP ctx) {
	thr_run1(ctx,xl0());
	return;
}
void thr_wait() {
	#ifndef THREAD
	#else
	void* _; int i; for(i=0;i<NTHR;i++) pthread_join(THR[i],&_);
	return;
	#endif
}

// TESTS

void test_basics() {
	printf("TEST_BASICS\n");
	#include "test-basics.h"
}
void test_ctx() {
	VP ctx,tmp1,tmp2;

	printf("TEST_CTX\n");
	#include "test-ctx.h"	
}
void test_deal_speed() {
	int i;
	VP a,b,c;
	// xprofile_start();
	
	a=xi(1024 * 1024);b=xi(100);
	TIME(100, ({ c=deal(a,b); xfree(c); }));
}
void test_eval() {
	#include"test-eval.h"
}
void test_json() {
	VP mask, jsrc, res; char str[256]={0};
	strncpy(str,"[[\"abc\",5,[\"def\"],6,[7,[8,9]]]]",256);
	jsrc=split(xfroms(str),xc0());
	XRAY_emit(jsrc);
	res=nest(jsrc,xln(2,xfroms("["),xfroms("]")));
	XRAY_emit(res);
	XRAY_emit(each(res, x1(&repr)));
	exit(1);
}
void test_logic() {
	VP a,b,c;
	printf("TEST_LOGIC\n");
	#include"test-logic.h"
}
void test_nest() {
	VP a,b,c;
	printf("TEST_NEST\n");
	#include"test-nest.h"
	xfree(a);xfree(b);xfree(c);
}
void test_semantics() {
	printf("TEST_SEMANTICS\n");
	#include"test-semantics.h"
}
VP evalinwith(VP tree,VP ctx,VP xarg) {
	if(!tree || !ctx) return NULL;
	if(IS_c(tree)) {
		char* buf=sfromxA(tree);
		VP res=evalstrinwith(buf,ctx,xarg);
		free(buf);
		return res;
	}
	ARG_MUTATING(ctx);
	if(!IS_x(ctx)) ctx=xxn(2,ctx,tree);  // try to make context ouf of dict (hopefully) and parse tree
	else {
		if(ctx->n==1) { ctx=xrealloc(ctx,2); }
		VALS(ctx)=tree;                 // parse tree is last item of context (a list, basically)
		ctx->n=2;
	}
	VP res=applyctx(ctx,xarg,NULL);
	return res;
}
VP evalin(VP tree,VP ctx) {
	if(IS_EXC(tree) || LEN(tree)==0) return tree;
	return evalinwith(tree,ctx,NULL);
}
VP evalstrin(const char* str, VP ctx) {
	return evalstrinwith(str,ctx,NULL);
}
VP evalstrinwith(const char* str, VP ctx, VP xarg) {
	VP p=parsestr(str);
	VP r=evalinwith(p,ctx,xarg);
	return r;
}
VP evalfile(VP ctx,const char* fn) {
	return loadin(xfroms(fn), ctx);
}
VP load0(VP fn,VP ctx) {
	if(!IS_c(fn) || !IS_x(ctx)) return EXC(Tt(type),"load0 x is filename and y is ctx",fn,ctx);
	VP contents = fileget(fn);
	RETURN_IF_EXC(contents);
	char* str=sfromxA(contents);
	set(ctx,Tt(_dir),filedirname(fn)); // set() returns y value, not x context - dont preserve
	VP parsetree=parseresolvestr(str,ctx);
	if(IS_EXC(parsetree)) return parsetree;
	free(str);
	append(ctx,parsetree);
	xfree(parsetree);
	VP res=applyctx(ctx,NULL,NULL);
	return res;
}
VP import(VP fn,VP ctx) {              // get, parse, eval in isolated ctx; return last result
	return ctx;
}
VP loadin(VP fn,VP ctx) {              // get, parse, eval in current ctx; return last result
	VP bkupdir=xref(lookup(ctx,Tt(_dir)));  // load0 clobbers _dir
	VP res=load0(fn,ctx);
	set(ctx,Tt(_dir),bkupdir);
	return res;
}
VP selftest(VP dummy) {
	int i;
	VP a,b,c;
	if (DEBUG) {
		printf("TESTS START\n");
		test_basics();
		test_nest();
		test_ctx();
		test_eval();
		test_logic();
		test_semantics();
		printf("TESTS PASSED\n");
		if(MEM_WATCH) {
			XRAY_log("alloced = %llu, freed = %llu\n", MEM_ALLOC_SZ, MEM_FREED_SZ);
		}
	}
	return 0;
}
void init_thread_locals() {
	XXL_SYS=xd0();
	XXL_SYS=assign(XXL_SYS,Tt(ver),xfroms(XXL_VER));
	XXL_SYS=assign(XXL_SYS,Tt(srcpath),xfroms(XXL_SRCPATH));
	XXL_SYS=assign(XXL_SYS,Tt(compobj),xfroms(XXL_COMPILEOBJ));
	XXL_SYS=assign(XXL_SYS,Tt(compshared),xfroms(XXL_COMPILESHARED));
	XXL_SYS=assign(XXL_SYS,Tt(buildobj),xfroms(XXL_BUILDOBJ));
	XXL_SYS=assign(XXL_SYS,Tt(buildshared),xfroms(XXL_BUILDSHARED));
	XXL_CUR_CTX=NULL;
}
void init() {
	srand(time(NULL)); // TODO need verb to srand
	XB0=xb(0); XB1=xb(1);
	XI0=xi(0); XI1=xi(1);
	TIEXCEPTION=Ti(exception); TINULL=_tagnums(""); 
	TTPARENT=Tt(parent);
	init_thread_locals();
	thr_start();
}
void deinit(int failed) {
	exit(failed);
}
void args(VP ctx, int argc, char* argv[]) {
	if(argc > 1) {
		int i; VP a=xlsz(argc); 
		for(i=1; i<argc; i++) {
			VP item=xfroms(argv[i]);
			if(AS_c(item,0)=='-') 
				show(evalin(show(catenate(xfroms("1 "),behead(item))),ctx));
			else if (access(argv[i],R_OK) != -1) { 
				show(evalfile(ctx,argv[i]));
				if(!isatty(fileno(stdout))) 
					deinit(0);
			} else {
				VP xarg=NULL;
				#ifdef STDLIB_FILE
				xarg=fileget(xfroms("-"));
				#endif
				show(evalstrinwith(argv[i],ctx,xarg));
				deinit(0);
			}
			a=append(a,item);
		}
		set(ctx,Tt(argv),a);
	} else selftest(NULL);
}
int main(int argc, char* argv[]) {
	init();
	VP ctx=mkworkspace();
	args(ctx,argc,argv);
	// net();
	repl(ctx);
	deinit(0);
	return 0;
}
/*	
	TODO diff: (1,2,3,4)diff(1,2,55) = [['set,2,55],['del,3]] (plus an easy way to map that diff to funcs to perform)
	TODO mailboxes
	TODO some kind of backing store for contexts cant stand losing my work
	TODO can we auto-parallelize some loops?
*/ 
