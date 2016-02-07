/* macros and helpers, function prototypes */
#include "def.h"
#include "proto.h"

// globals
VP XI0=0; VP XI1=0; // set in init() 
I8 PF_ON=0;
I8 PF_LVL=0;
VP TAGS=NULL;

#define GOBBLERSZ 50
static VP MEM_RECENT[GOBBLERSZ] = {0};
static I8 MEM_W=0; //watching memory?
#define N_MEM_PTRS 1024
static VP MEM_PTRS[N_MEM_PTRS]={0};
static I32 MEM_ALLOC_SZ=0,MEM_FREED_SZ=0;
static I32 MEM_ALLOCS=0, MEM_REALLOCS=0, MEM_FREES=0, MEM_GOBBLES=0;

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
*/
char* repr0(VP x,char* s,size_t sz) {
	type_info_t t;
	if(x==NULL) { APF(sz,"/*null*/",0); return s; }
	t=typeinfo(x->t);
	if(0 && DEBUG) {
		APF(sz," /*%p %s tag=%d#%s itemsz=%d n=%d rc=%d*/ ",x,t.name,
			x->tag,(x->tag!=0 ? sfromx(tagname(x->tag)) : ""),
			x->itemsz,x->n,x->rc);
	}
	if(x->tag!=0) 
		APF(sz, "'%s(", sfromx(tagname(x->tag)));
	if(t.repr) (*(t.repr)(x,s,sz));
	if(x->tag!=0)
		APF(sz, ")", 0);
	return s;
}
char* reprA(VP x) {
	#define BS 1024*10
	char* s = calloc(1,BS);
	s = repr0(x,s,BS);
	//APF(BS,"\n",0);
	return s;
}
VP repr(VP x) {
	char* s = reprA(x);
	return xfroms(s);
}
char* repr_l(VP x,char* s,size_t sz) {
	int i=0, n=x->n;VP a;
	APF(sz,"[",0);
	for(i=0;i<n;i++){
		a = ELl(x,i);
		repr0(a,s,sz);
		if(i!=n-1)
			// APF(sz,",\n",0);
			APF(sz,", ",0);
			// APF(sz,", ",0);
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
		if(ch=='\n') APF(sz,"\\n", 0);
		if(ch=='\r') APF(sz,"\\r", 0);
		else APF(sz,"%c",ch);
		// repr0(*(EL(x,VP*,i)),s,sz);
	}
	APF(sz,"\"",0);
	return s;
}
char* repr_t(VP x,char* s,size_t sz) {
	int i=0,n=x->n,tag;
	if(n>1) APF(sz,"(",0);
	for(;i<n;i++){
		tag = AS_t(x,i);
		APF(sz,"'%s",sfromx(tagname(tag)));
		if(i!=n-1)
			APF(sz,",",0);
		// repr0(*(EL(x,VP*,i)),s,sz);
	}
	if(n>1) APF(sz,")",0);
	return s;
}
char* repr_x(VP x,char* s,size_t sz) {
	int i;VP a;
	APF(sz,"'ctx[",0);
	for(i=0;i<x->n;i++){
		a = ELl(x,i);
		repr0(a,s,sz);
		if(i!=x->n-1)
			APF(sz,", ",0);
		// repr0(*(EL(x,VP*,i)),s,sz);
	}
	APF(sz,"]",0);
	return s;
}
char* repr_d(VP x,char* s,size_t sz) {
	int i, n;
	VP k=KEYS(x),v=VALS(x);
	if (!k || !v) { APF(sz,"[null]",0); return s; }
	APF(sz,"[",0);
	n=k->n;
	for(i=0;i<n;i++) {
		repr0(apply(k,xi(i)), s, sz);
		APF(sz,":",0);
		repr0(apply(v,xi(i)), s, sz);
		if(i!=n-1)
			APF(sz,", ",0);
	}
	APF(sz,"]",0);
	return s;
}
char* repr_p(VP x,char* s,size_t sz) {
	Proj p = EL(x,Proj,0);
	ASSERT(1,"repr_p");
	APF(sz,"'projection(%p,%d,%p,",x,p.type,p.type==1?p.f1:p.f2);
	if(p.left!=NULL) 
		repr0(p.left, s, sz);
	else
		APF(sz,"()",0);
	APF(sz,",",0);
	if(p.right!=NULL) 
		repr0(p.right, s, sz);
	else
		APF(sz,"()",0);
	APF(sz,")",0);
	return s;
}
#include "repr.h"
#include "types.h"

static inline type_info_t typeinfo(type_t n) { 
	ITER(TYPES,sizeof(TYPES),{ IF_RET(_x.t==n,_x); }); 
	return (type_info_t){0}; }
static inline type_info_t typechar(char c) { 
	ITER(TYPES,sizeof(TYPES),{ IF_RET(_x.c==c,_x); }); 
	return (type_info_t){0}; }

VP xalloc(type_t t,I32 initn) {
	VP a; int g,i,itemsz,sz; 
	initn = initn < 4 ? 4 : initn;
	itemsz = typeinfo(t).sz; sz=itemsz*initn;
	//PF("%d\n",sz);
	a=NULL;g=0;
	if (GOBBLERSZ > 0) {
		WITHLOCK(mem, {
			FOR(0,GOBBLERSZ,({
				if(MEM_RECENT[_i]!=0 && 
					 ((VP)MEM_RECENT[_i])->sz > sz &&  // TODO xalloc gobbler should bracket sizes
					 ((VP)MEM_RECENT[_i])->sz < (sz * 20)) {
					a=MEM_RECENT[_i];
					MEM_RECENT[_i]=0;
					MEM_GOBBLES++;
					g=_i;
					memset(BUF(a),0,a->sz);
					break;
				}
			}));
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
	return xl0();
}
VP xprofile_end() {
	int i;
	VP ctx;
	VP res; 
	MEM_W=0;
	printf("allocs: %d (%d), gobbles: %d, reallocs: %d, frees: %d\n", MEM_ALLOC_SZ, MEM_ALLOCS, MEM_GOBBLES, MEM_REALLOCS, MEM_FREES);
	for(i=0;i<N_MEM_PTRS;i++)
		if(MEM_PTRS[i]!=0) {
			printf("freeing mem ptr\n");
			xfree(MEM_PTRS[i]);
			MEM_PTRS[i]=0;
		}
	return xl0();
}
VP xrealloc(VP x,I32 newn) {
	// PF("xrealloc %p %d\n",x,newn);
	if(newn>x->cap) {
		buf_t newp; I32 newsz;
		newn = (newn < 10*1024) ? newn * 4 : newn * 1.25; // TODO there must be research about realloc bins no?
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
	//PF("xfree(%p)\n",x);DUMP(x);//DUMP(info(x));
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
		//PF("xfree(%p) really dropping type=%d n=%d alloc=%d\n",x,x->t,x->n,x->alloc);
	} return x; }
VP xref(VP x) { if(MEM_W){MEMPF("ref %p\n",x);} x->rc++; return x; }
VP xfroms(const char* str) {  // character value from string - strlen helper
	size_t len = strlen(str); type_info_t t = typechar('c');
	VP a = xalloc(t.t,len); memcpy(BUF(a),str,len); a->n=len; return a; }
const char* sfromx(VP x) { 
	if(x==NULL)return "null";
	return (char*)BUF(x); }

VP clone(VP obj) { 
	// TODO keep a counter of clone events for performance reasons - these represent a concrete
	// loss over mutable systems
	// PF("clone\n");DUMP(obj);
	if(CONTAINER(obj)) return deep(obj,x1(&clone));
	int i;VP res=ALLOC_LIKE(obj);
	for(i=0;i<obj->n;i++) {
		// PF("cloning %d\n", i);
		res=appendbuf(res,ELi(obj,i),1);
	}
	// PF("clone returning\n");DUMP(res);
	return res;
}

// RUNTIME 

VP appendbuf(VP x,buf_t buf,size_t nelem) {
	int newn;buf_t dest;
	//PF("appendbuf %d\n", nelem);DUMP(x);
	newn = x->n+nelem;
	x=xrealloc(x,newn);
	// PF("after realloc"); DUMP(x);
	dest = ELi(x,x->n);
	memmove(dest,buf,x->itemsz * nelem);
	x->n=newn;
	// PF("appendbuf newn %d\n", newn); DUMPRAW(dest, x->itemsz * newn);
	return x;
}
VP append(VP x,VP y) { 
	// append all items of y to x. if x is a general list, append pointer to y, and increase refcount.
	// PF("append %p %p\n",x,y); DUMP(x); DUMP(y);
	IF_EXC(!CONTAINER(x) && !(x->t==y->t), Tt(Type), "append x must be container or types must match", x, y);
	if(IS_d(x)) {
		ASSERT(y->n % 2 == 0, "append to a dict with ['key;value]");
		VP k=KEYS(x),v=VALS(x),y1,y2; int i;
		y1=ELl(y,0);
		y2=ELl(y,1);
		// tough decisions
		// PF("append dict\n");DUMP(x);DUMP(k);DUMP(v);
		if(k==NULL) { // create dict
			if(0 && SCALAR(y1)) {
				k=ALLOC_LIKE_SZ(y1, 4);
			} else {
				k=xl0();
			}
			v=xl0(); xref(k);xref(v); EL(x,VP,0)=k; EL(x,VP,1)=v;
			x->n=2;
			// PF("dict kv %p %p\n", k, v); DUMP(k); DUMP(v);
			i=-1;
		} else 
			i=_find1(k,y1);
		if(i==-1) {
			xref(y1); xref(y2); EL(x,VP,0)=append(k,y1); EL(x,VP,1)=append(v,y2);
		} else {
			xref(y2);
			ELl(v,i)=y2;
		}
		return x;
	}
	if(CONTAINER(x)) { 
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
	if(_find1(x,y)==-1) append(x,y); return x;
}
int _upsertidx(VP x,VP y) {
	int idx = _find1(x,y);
	if(idx>-1) return idx;
	append(x,y); return x->n-1;
}
VP amend(VP x,VP y) {
	PF("amend\n");DUMP(x);DUMP(y);
	// TODO amend should create a new structure rather than modifying one in-place
	if(!SIMPLE(x) && !CONTAINER(x))return EXC(Tt(type),"amend x must be a simple or container type",x,y);
	if(!CONTAINER(y) || y->n!=2)return EXC(Tt(type),"amend y should be [indices,replacements]",x,y);
	VP idx=ELl(y,0), val=ELl(y,1), acc=ALLOC_LIKE_SZ(x, idx->n), idxi, idxv, tmp=0; int i,fail=-1;
	if(CALLABLE(idx)) idx=condense(matcheasy(x,idx));
	for(i=0;i<idx->n;i++) { 
		idxi=xi(i); idxv=apply(idx,idxi); // TODO really need a fast 0 alloc version of apply(simple,int)!
		if(UNLIKELY(CALLABLE(val))) tmp=apply(val,apply(x,idxv));
		else {
			if (SCALAR(val)) tmp=xref(val); 
			else tmp=apply(val,idxv);
		}
		if(UNLIKELY(!CONTAINER(x) && tmp->t!=x->t))return EXC(Tt(value),"amend value type does not match x",x,tmp);
		assign(x,idxv,tmp);
		xfree(idxi); xfree(idxv); xfree(tmp);
	}
	PF("amend returning\n");DUMP(x);
	return x;
}
static inline VP assign(VP x,VP k,VP val) {
	// PF("assign\n");DUMP(x);DUMP(k);DUMP(val);
	if (LIST(k) && k->n) {
		int i=0;VP res=x;
		for(;i<k->n-1;i++) {
			PF("assign-at-depth %d\n",i);
			res=apply(res,ELl(k,i));
			DUMP(res);
			if(UNLIKELY(IS_EXC(res)) || res->n==0)
				return res;
		}
		PF("assign-at-depth setting\n");
		assign(res,ELl(k,k->n-1),val);
		return res;
	}
	if(DICT(x)) {
		xref(k); xref(val);
		return append(x,xln(2,k,val));
	} else if(SIMPLE(x) && NUM(k)) {
		// PF("assign");DUMP(x);DUMP(k);DUMP(val);
		if(x->t != val->t) return EXC(Tt(type),"assign value and target types don't match",x,val);
		int typerr=-1;
		VARY_EACHRIGHT_NOFLOAT(x,k,({
			EL(x,typeof(_x),_y) = EL(val,typeof(_x),_y%val->n); // TODO assign should create new return value
		}),typerr);
		// PF("assign num returning");DUMP(x);
		return x;
	}
	return EXC(Tt(type),"assign: bad types",x,0);
}
static inline VP assigns(VP x,const char* key,VP val) {
	return assign(x,xfroms(key),val);
}
VP behead(VP x) {
	PF("behead\n");DUMP(x);
	return drop_(x,1);
}
int _flat(VP x) { // returns 1 if vector, or a list composed of vectors (and not other lists)
	// PF("flat\n");DUMP(x);
	if(!CONTAINER(x)) return 1;
	else return 0; // lists are never flat
}
VP flatten(VP x) {
	int i,t=-1;VP res=0;
	if(!LIST(x))return x;
	if(x->n) {
		t=ELl(x,0)->t; res=ALLOC_LIKE(ELl(x,0));
		for(i=0;i<x->n;i++) {
			if(ELl(x,i)->t!=t) {
				xfree(res); return x;
			} else
				append(res,ELl(x,i));
		}
		return res;
	} else return xl0();
}
VP curtail(VP x) {
	PF("curtail\n");DUMP(x);
	return drop_(x,-1);
}
VP dict(VP x,VP y) {
	PF("dict\n");DUMP(x);DUMP(y);
	if(DICT(x)) {
		if(DICT(y)) {
			ASSERT(LIST(KEYS(x)) && LIST(VALS(x)),"dict() x keys or vals not list");
			FOR(0,x->n,({ assign(y,ELl(KEYS(x),_i),ELl(VALS(x),_i)); }));
			return y;
		} 
		if(KEYS(x)->n > VALS(x)->n) { // dangling dictionary detection
			append(VALS(x),y);
		} else {
			if(LIST(y) && y->n==2) {
				append(KEYS(x),first(y));
				append(VALS(x),last(y));
			} else {
				append(KEYS(x),y);
			}
		}
		PF("dict already dict returning\n");DUMP(x);
		return x;
	} else {
		if(x->n > 1 && x->n != y->n) return EXC(Tt(value),"can't create dict from unlike vectors",x,y);
		VP d=xd0();
		if(LIKELY(SCALAR(x))) {
			if(LIST(x))  // handle ['a:1] which becomes [['a]:1]
				d=assign(d,ELl(x,0),y);
			else
				d=assign(d,x,y);
		} else {
			int i;VP ii;
			for(i=0;i<x->n;i++) {
				ii=xi(i); d=assign(d,apply(x,ii),apply(y,ii));
			}
		}
		d->n=2;
		PF("dict new returning\n");DUMP(d);
		return d;
	}
}
VP drop_(VP x,int i) {
	VP res;
	int st, end;
	if(i<0) { st = 0; end=x->n+i; }
	else { st = i; end=x->n; }
	PF("drop_(,%d) %d %d %d\n", i, x->n, st, end);
	DUMP(x);
	res=ALLOC_LIKE_SZ(x,end-st);
	// DUMP(info(res));
	if(end-st > 0) {
		appendbuf(res, ELi(x,st), end-st);
	}
	PF("drop_ result\n"); DUMP(res);
	return res;
}
VP drop(VP x,VP y) {
	VP res=0;
	int typerr=-1;
	PF("drop args\n"); DUMP(x); DUMP(y);
	IF_RET(!NUM(y) || !SCALAR(y), EXC(Tt(type),"drop y arg must be single numeric",x,y));	
	VARY_EL(y, 0, ({ return drop_(x,_x); }), typerr);
	return res;
}
VP first(VP x) {
	VP i,r;
	if(CONTAINER(x)) return xref(ELl(x,0));
	else { i=xi(0); r=apply(x,i); xfree(i); return r; }
}
VP identity(VP x) {
	return x;
}
VP join(VP x,VP y) {
	VP res=0;
	PF("join\n");DUMP(x);DUMP(y);
	int n = x->n + y->n;
	if(!CONTAINER(x) && x->tag==0 && x->t==y->t) {
		PF("join2\n");
		res=ALLOC_LIKE_SZ(x, n);
		appendbuf(res, BUF(x), x->n);
		appendbuf(res, BUF(y), y->n);
	} else {
		PF("join3\n");
		if(DICT(x))
			return dict(x,y);
		else if(LIST(x) && (!LIST(y) || y->n==1)) {
			res=append(x,y);
		} else {
			PF("join4\n");
			res=xlsz(2);
			res=append(res,x);
			res=append(res,y);
		}
	}
	//res=list2vec(res);
	PF("join result");DUMP(res);
	return res;
}
VP last(VP x) {
	VP res=ALLOC_LIKE_SZ(x,1);
	if(x->n==0) return res;
	res=appendbuf(res,ELi(x,x->n-1),1);
	return res;
}
static inline VP list(VP x) { // convert x to general list
	if(LIST(x))return x;
	return split(x,xi0());
}
VP list2(VP x,VP y) { // helper for [ - convert y to general list, drop x
	return xl(y);
}
VP nullfun(VP x) {
	return xl0();
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
VP reverse(VP x) {
	if(!SIMPLE(x)||CONTAINER(x)) return EXC(Tt(type),"reverse arg must be simple or container",x,0);
	int i,typerr=-1; VP acc=ALLOC_LIKE(x);
	for(i=x->n-1;i>=0;i--) appendbuf(acc,ELi(x,i),1);
	return acc;
}
VP shift_(VP x,int i) {
	PF("shift_ %d\n",i);DUMP(x);
	int n=x->n;
	if(i<0) 
		return join(take_(x,i%n),drop_(x,i%n));
	else
		return join(drop_(x,i%n),take_(x,i%n));
}
VP shift(VP x,VP y) {
	PF("shift\n");DUMP(x);DUMP(y);
	if(!SIMPLE(x)) return EXC(Tt(type),"shr x must be a simple type",x,y);
	if(!NUM(y)) return EXC(Tt(type),"shr y must be numeric",x,y);
	int typerr=-1;
	VARY_EL(y,0,({return shift_(x,_x);}),typerr);
	return (VP)0;
}
VP show(VP x) {
	char* p = reprA(x);
	printf("%s\n",p);
	free(p);
	return x;
}
VP splice(VP x,VP idx,VP replace) {
	int i, first = AS_i(idx,0),last=first+idx->n;
	VP acc;
	PF("splice (%d len) %d..%d",x->n, first,last);DUMP(x);DUMP(idx);DUMP(replace);
	if(first==0 && last==x->n) return replace;
	acc=xl0();
	if(LIST(x)) {
		for(i=0;i<first;i++)
			acc=append(acc,ELl(x,i));
		append(acc,replace);
		for(i=last;i<x->n;i++)
			acc=append(acc,ELl(x,i));
	} else {
		if(first > 0) 
			acc=append(acc, take_(x, first));
		acc=append(acc, replace);
		if (last < x->n)
			acc=append(acc, drop_(x, last));
		PF("splice calling over\n");DUMP(acc);
		return over(acc, x2(&join));
	}
	PF("splice returning\n"); DUMP(acc);
	return acc;
}
VP split(VP x,VP tok) {
	PF("split");DUMP(x);DUMP(tok);
	VP tmp=0,tmp2=0; int locs[1024],typerr=-1;

	// special case for empty or null tok.. split vector into list
	if(tok->n==0) {
		tmp=xl0();
		if(LIST(x))return x;
		VARY_EACHLIST(x,({
			// PF("in split vary_each %c\n",_x);
			tmp2=ALLOC_LIKE_SZ(x, 1);
			tmp2=appendbuf(tmp2,(buf_t)&_x,1);
			tmp=append(tmp,tmp2);
		}),typerr);
		IF_RET(typerr>-1, EXC(Tt(type),"can't split that type", x, tok));
		PF("split returning\n");DUMP(tmp);
		return tmp;
	}

	if(x->t == tok->t && tok->n==1) {
		int i=0,j=0,last=0;
		VP rest=x,acc=0;
		for(;i<x->n;i++) {
			if (_equalm(x,i,tok,0)) {
				if(!acc)acc=xlsz(x->n/1);
				acc=append(acc,take_(rest,i-last));
				rest=drop_(rest,i+1-last);
				last=i+1;
			}
		}
		if(acc){ acc=append(acc,rest); return acc; }
		else return x;
	}

	return EXC(Tt(nyi),"split with that type of data not yet implemented",x,tok);
}
VP take_(const VP x,const int i) {
	VP res;
	int st, end, xn=x->n;
	if (i<0) { st=ABS((xn+i)%xn); end=ABS(i)+st; } else { st=0; end=i; }
	res=ALLOC_LIKE_SZ(x,end-st);
	FOR(st,end,({ res=appendbuf(res,ELi(x,_i % xn),1); }));
	return res;
}
VP take(const VP x,const VP y) {
	int typerr=-1;
	size_t st,end; //TODO slice() support more than 32bit indices
	PF("take args\n"); DUMP(x); DUMP(y);
	IF_RET(!NUM(y) ||!SCALAR(y), EXC(Tt(type),"take x arg must be single numeric",x,y));	
	VARY_EL(y, 0, ({ return take_(x,_x); }), typerr);
	return (VP)0;
}
static inline int _equalm(const VP x,const int xi,const VP y,const int yi) {
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
int _equal(const VP x,const VP y) {
	// TODO _equal() needs to handle comparison tolerance and type conversion
	// TODO _equal should use the new VARY_*() macros, except for general lists
	//PF("_equal\n"); DUMP(x); DUMP(y);
	// if the list is a container for one item, we probably want to match the inner one
	VP a=x,b=y;
	if(LIST(a) && SCALAR(a)) a=ELl(a,0);
	if(LIST(b) && SCALAR(b)) b=ELl(b,0);
	IF_RET(a->n != b->n, 0);
	if(CONTAINER(a) && CONTAINER(b)) { ITERV(a,{ IF_RET(_equal(ELl(a,_i),ELl(b,_i))==0, 0); }); return 1; }
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
int _findbuf(const VP x,const buf_t y) {   // returns index or -1 on not found
	// PF("findbuf\n");DUMP(x);
	if(LISTDICT(x)) { ITERV(x,{ 
		// PF("findbuf trying list\n"); DUMP(ELl(x,_i));
		IF_RET(_findbuf(ELl(x,_i),y)!=-1,_i);
		// PF("findbuf no list match\n");
	}); } else {
		// PF("findbuf trying vector\n");
		ITERV(x,{ IF_RET(memcmp(ELi(x,_i),y,x->itemsz)==0,_i); });
		// PF("findbuf no vector match\n");
	}
	return -1;
}
int _find1(VP x,VP y) {        // returns index or -1 on not found
	// probably the most common, core call in the code. worth trying to optimize.
	// PF("_find1\n",x,y); DUMP(x); DUMP(y);
	ASSERT(LIST(x) || (x->t==y->t && y->n==1), "_find1(): x must be list, or types must match with right scalar");
	if(LISTDICT(x)) { ITERV(x,{ 
		VP xx; xx=ELl(x,_i);
		// PF("_find1 %d\n",_i); DUMP(xx);
		if(xx!=NULL) 
			IF_RET(_equal(xx,y)==1,_i);
	}); }
	else {
		ITERV(x,{ IF_RET(memcmp(ELi(x,_i),ELi(y,0),x->itemsz)==0,_i); });
	}
	return -1;
}
VP find1(VP x,VP y) {
	return xi(_find1(x,y));
}
int _contains(VP x,VP y) {
	return _find1(x,y)==-1 ? 0 : 1;
}
VP contains(VP x,VP y) {
	return xi(_contains(x,y));
}
VP condense(VP x) {
	// equivalent to k's & / where - condenses non-zeroes into their positions (ugh english)
	// always returns an int vector for now; we generally rely on ints too much for this
	int typerr=-1; int j; VP acc=xi0();
	// PF("condense\n");DUMP(x);
	VARY_EACH(x,({ if(_x) { j=_i; FOR(0,_x,appendbuf(acc,(buf_t)&j,1)); } }),typerr);
	// PF("condense returning\n");DUMP(acc);
	return acc;
}
VP cast(VP x,VP y) { 
	// TODO cast() should short cut matching kind casts 
	#define BUFSZ 128
	VP res=0; I8 buf[BUFSZ]={0}; int typetag=-1;type_t typenum=-1; 
	// right arg is tag naming a type, use that.. otherwise use y's type
	if(IS_t(y)) typetag=AS_t(y,0); else typenum=y->t;
	#include"cast.h"
	// DUMPRAW(buf,BUFSZ);
	return res;
}
VP len(VP x) {
	return xin(1,x->n);
}
VP capacity(VP x) {
	return xin(1,x->cap);
}
VP itemsz(VP x) {
	return xi(x->itemsz);
}
VP info(VP x) {
	VP res;
	type_info_t t;
	t=typeinfo(x->t);
	res=xd0();
	res=assign(res,Tt(typenum),xi(x->t));
	res=assign(res,Tt(type),xfroms(t.name));
	res=assign(res,Tt(len),len(x));
	res=assign(res,Tt(capacity),capacity(x));
	res=assign(res,Tt(itemsz),itemsz(x));
	res=assign(res,Tt(alloced),xi(x->alloc));
	res=assign(res,Tt(baseptr),xi((int)x));
	res=assign(res,Tt(memptr),xi((int)BUF(x)));
	return res;
}
VP deal(VP range,VP amt) {
	PF("deal\n");DUMP(range);DUMP(amt);
	IF_EXC(!NUM(range),Tt(type),"deal: left arg must be numeric", range, amt);
	IF_EXC(!NUM(range),Tt(type),"deal: single right arg must be numeric", range, amt);
	IF_EXC(!SCALAR(range) || !SCALAR(amt), Tt(nyi), "deal: both args must be scalar", range, amt);

	int typerr=-1;
	VP acc=0;
	VARY_EL(amt,0,({ typeof(_x)amt=_x; acc=ALLOC_LIKE_SZ(range,_x); // TODO rethink deal in terms of more types
		VARY_EL_NOFLOAT(range,0,({
			FOR(0,amt,({ EL(acc,typeof(_x),_i)=rand()%_x; }));
			acc->n=amt;
		}),typerr);}),typerr);
	return acc;
}

// APPLICATION, ITERATION AND ADVERBS

static inline VP applyexpr(VP parent,VP code,VP xarg,VP yarg) {
	PF("applyexpr (code, xarg, yarg):\n");DUMP(code);DUMP(xarg);DUMP(yarg);
	// if(!LIST(code))return EXC(Tt(code),"expr code not list",code,xarg);
	if(!LIST(code))return code;
	char ch; int i, tag, tcom, texc, tlam, traw, tname, tstr, tws, xused=0, yused=0; 
	VP left,item;
	clock_t st=0;
	left=xarg; if(!yarg) yused=1;
	tcom=Ti(comment); texc=Ti(exception); tlam=Ti(lambda); 
	traw=Ti(raw); tname=Ti(name); tstr=Ti(string); tws=Ti(ws);

	if(SIMPLE(code)) return code;
	if(!LIST(code)) code=list(code);
	PFIN();
	for(i=0;i<code->n;i++) {
		PF("applyexpr #%d/%d, consumed=%d/%d\n",i,code->n-1,xused,yused);
		DUMP(left);
		item = ELl(code,i);
		tag=item->tag;
		DUMP(item);
		// consider storing these skip conditions in an array
		if(tag==tws) continue;
		if(tag==tcom) continue;

		if(tag==tlam) { // create a context for lambdas
			VP newctx,this; int j;
			newctx=xx0();
			item=list(item);
			for(j=0;j<parent->n;j++) {
				this=ELl(parent,j);
				if(j!=parent->n-1 || !LIST(this))
					append(newctx,clone(ELl(parent,j)));
			}
			append(newctx,entags(ELl(item,0),"")); // second item of lambda is arity; ignore for now
			item=newctx;
			PF("created new lambda context item=\n");DUMP(item);
		} else if (tag==Ti(listexpr) || tag==Ti(expr)) {
			PF("applying subexpression\n");
			PFIN();
			item=applyexpr(parent,item,0,0);
			PFOUT();
			RETURN_IF_EXC(item);
			if(tag==Ti(listexpr)&&!CONTAINER(item)) item=list(item);
			PF("subexpression came back with");DUMP(item);
		} else 
		// if(LIKELY(IS_c(item)) && tag != tstr) {
		if(LIKELY(IS_c(item)) && tag != tstr) {
			ch = AS_c(item,0);
			if(ch==';') { // end of expr; remove left/x
				left=0; xused=1;
				continue;
			} 
			PF("much ado about\n");DUMP(item);
			if(item->n==1 && ch=='x')
				item=xarg;
			else if(item->n==1 && ch=='y' && yarg!=0) {
				PF("picking up y arg\n");DUMP(yarg);
				item=yarg;
			}
			else if(item->n==2 && ch=='a' && AS_c(item,1)=='s') {
				left=proj(2,&set,xln(2,parent,left),0);
				left->tag=Ti(setproj);
				xfree(left);
				PF("created set projection\n");
				DUMP(left);
				continue;
			} else if(item->n==2 && ch=='.' && AS_c(item,1)=='t') {
				printf("timer on\n");
				st=clock();
				continue;
			} else
				item=get(parent,item);
			PF("decoded string identifier\n");DUMP(item);
			if(IS_EXC(item)) return left!=0 && CALLABLE(left)?left:item;
		} else if(tag==tname) {
			PF("non-string name encountered");
			item=get(parent,item);
			RETURN_IF_EXC(item);
		}
		
		PF("before grand switch, xused=%d:\n", xused);
		DUMP(left);
		DUMP(item);

		if(left!=0 && CALLABLE(left)) {
			// they seem to be trying to call a unary function, though it's on the
			// left - NB. possibly shady
			//
			// if you pass a projection as the xargument, we should NOT immediately pass
			// the next value to it, even though it appears as "left". xused acts
			// as a gate for that "are we still possibly needing the passed-in (left) value?"
			// logic to not allow this behavior in first position inside expression
			Proj p;
			left=apply(left,item);
			RETURN_IF_EXC(left);
			if(IS_p(left)) {
				p=AS_p(left,0);
				if(!yused && p.type==2 && (!p.left || !p.right)) {
					PF("applyexpr consuming y:\n");DUMP(yarg);
					left=apply(left,yarg);
					yused=1;
				}
			}
		} else if(!CALLABLE(item) && (left!=0 && !CALLABLE(left))) {
			PF("applyexpr adopting left =\n");DUMP(item);
			xused=1;
			left=item;
		} else {
			if(left) {
				PF("applyexpr calling apply\n");DUMP(item);DUMP(left);
				xused=1;
				PFIN();
				left=apply(item,left);
				RETURN_IF_EXC(left);
				PFOUT();
				if(left->tag==texc) return left;
				PF("applyexpr apply returned\n");DUMP(left);
			} else {
				PF("no left, so continuing with item..\n");
				left=item;
			}
		}
	}
	PFOUT();
	PF("applyexpr returning\n");
	DUMP(left);
	if(st!=0) {
		clock_t en;
		en = clock();
		printf("time=%0.4f\n", ((double)(en-st)) / CLOCKS_PER_SEC); 
	}
	return left;
}
VP applyctx(VP x,VP y) {
	// structure of a context. like a general list conforming to:
	// first item: code. list of projections or values. evaluated left to right.
	// second item: parent context. used to resolve unidentifiable symbols
	// third item: parent's parent
	// .. and so on
	// checked in order from top to bottom with apply()
	if(!IS_x(x)) return EXC(Tt(type),"context not a context",x,y);
	int i;VP this,res=NULL;
	PF("applyctx\n");DUMP(x);DUMP(y);
	for(i=x->n-1;i>=0;i--) {
		this=ELl(x,i);
		PF("applyctx #%d\n", i);
		DUMP(this);
		if(LIST(this)) { // code bodies are lists - maybe use 'code tag instead? 
			PF("CTX CODE BODY\n");DUMP(this);
			res=applyexpr(x,this,y,0);
			if(!res) return res;
		}
		// NB. if the function body returns an empty list, we try the scopes (dictionaries).
		// this may not be what we want in the long run.
		if(res==NULL || (LIST(res) && res->n == 0))
			res=apply(this,y);
		PF("applyctx apply result was\n");DUMP(res);
		if(!LIST(res) || res->n > 0) {
			PF("applyctx returning\n"); DUMP(res); 
			return res;
		}
	}
	return EXC(Tt(undef),"undefined in applyctx",x,y);
}
VP apply(VP x,VP y) {
	// this function is everything.
	VP res=NULL;int i=0,typerr=-1;
	// PF("apply\n");DUMP(x);DUMP(y);
	if(x->tag==_tagnums("exception"))return x;
	if(IS_p(x)) { 
		// if its dyadic
		   // if we have one arg, add y, and call - return result
			 // if we have no args, set y as left, and return x
		// if its monadic
			// if we have one arg already, call x[left], and then apply result with y
			// i think this is right..
		// PF("apply proj\n");DUMP(x);
		Proj* p; p=(Proj*)ELi(x,0);
		// if(!p->left) p->left=y; else if (!p->right) p->right=y;
		// DUMP(p->left); DUMP(p->right);
		if(p->type==1) {
			if (p->left) // not sure if this will actually happen - seems illogical
				return (*p->f1)(p->left);
			else
				return (*p->f1)(y);
		}
		if(p->type==2) {
			PF("proj2\n"); 
			PFIN();
			if(p->left && p->right)
				y=(*p->f2)(p->left,p->right);
			else if(y && p->left)
				y=(*p->f2)(p->left, y);
			else if (y && p->right)
				y=(*p->f2)(y,p->right);
			else if (y) 
				y=proj(2,p->f2,y,0);
			else
				y=x;
			PFOUT(); PF("proj2 done\n"); DUMP(y);
			return y;
		}
		return xp(*p);
	}
	if(IS_1(x)) {
		PF("apply 1\n");DUMP(x);DUMP(y);
		unaryFunc* f; f=AS_1(x,0); return (*f)(y);
	}
	if(IS_2(x)) {
		res=proj(2,AS_2(x,0),y,0);
		PF("apply f2 returning\n");DUMP(res);
		return res;
	}
	if(!CALLABLE(x) && UNLIKELY(LIST(y))) { // indexing at depth - never done for callable types 1, 2, and p (but we do use it for x)
		PF("indexing at depth\n");
		DUMP(x);
		DUMP(info(x));
		DUMP(y);
		DUMP(info(y));
		res=x;
		for(;i<y->n;i++) {
			res=apply(res,ELl(y,i));
			PF("at-depth result %d\n",i);DUMP(res);
			if(UNLIKELY(IS_EXC(res)) || res->n==0) 
				return res;
		}
		return res;
	}
	if(IS_x(x)) {
		// PF("apply ctx\n");DUMP(x);DUMP(y);
		return applyctx(x,y);
	}
	// TODO apply() is called most often in XXL; maybe worth transforming this if cascade into a case?	
	if(DICT(x)) {
		VP k=KEYS(x),v=VALS(x);I8 found;
		if(k==NULL || v==NULL) return NULL;
		res=xi0();
		if(LIST(k) && IS_c(y)) { // special case for strings as dictionary keys - common
			int idx;
			PF("searching for string\n");
			if ((idx=_find1(k,y))>-1)
				append(res,xi(idx));
		} else {
			ITERV(y,{ 
				int idx;
				PF("searching %d\n",_i);
				DUMP(y); DUMP(k);
				if(LIST(y)) idx = _find1(k,ELl(y,_i));
				else idx = _findbuf(k,ELi(y,_i));
				if(idx>-1) {
					found=1;
					PF("found at idx %d\n", idx); 
					append(res,xi(idx));
					break;
				}
			});
		}
		if(res->n==0) {
			if(x->next!=0) res=apply((VP)x->next, res);
		}
		if(res->n==0) return xl0(); else return apply(v,res);
	}
	if(NUM(y)) {
		// index a value with an integer 
		if(y->n==1 && LIST(x)) {
			// special case for generic lists:
			// if you index with one item, return just that item
			// generally you would receive a list back
			// this may potentially become painful later on 
			i = AS_i(y,0); 
			IF_RET(i>=x->n, ({
				EXC(Tt(index),"index out of range",x,y);	
			}));

			VP tmp = ELl(x,i); xref(tmp); return tmp;
		} else {
			res=xalloc(x->t,y->n);
			VARY_EACH_NOFLOAT(y,appendbuf(res,ELi(x,_x),1),typerr);
			//PF("apply VARY_EACH after\n"); DUMP(res);
			if(typerr>-1) return EXC(Tt(type),"cant use y as index into x",x,y);
			return res;
		}
	}
	return EXC(Tt(apply),"apply failure",x,y);
}
VP deep(VP obj,VP f) {
	// TODO perhaps deep() should throw an error with non-list args - calls each() now
	int i;
	PF("deep\n");DUMP(info(obj));DUMP(obj);DUMP(f);
	VP acc,subobj;
	if(!CONTAINER(obj)) return each(obj,f);
	if(_flat(obj)) {
		PF("deep flat\n");
		acc=apply(f,obj);
		if(obj->tag) acc->tag=obj->tag;
		return acc;
	}
	acc=ALLOC_LIKE(obj);
	PFIN();
	FOR(0,obj->n,({
		// PF("deep %d\n", _i);
		subobj=ELl(obj,_i);
		if(LIST(subobj))
			subobj=deep(subobj,f);
		else
			subobj=apply(f,subobj);
		// append(acc,subobj);
		EL(acc,VP,_i) = subobj; // this may not be safe, but append() is overriden for dicts, so we cant simply append the list
	}));
	acc->n=obj->n;
	PFOUT();
	PF("deep returning\n");DUMP(acc);
	return acc;
}
static inline VP each(VP obj,VP fun) { 
	// each returns a list if the first returned value is the same as obj's type
	// and has one item
	VP tmp, res, acc=NULL; int n=obj->n;
	// PF("each\n");DUMP(obj);DUMP(fun);
	FOR(0,n,({ 
		// PF("each #%d\n",n);
		tmp=apply(obj, xi(_i)); res=apply(fun,tmp); 
		// delay creating return type until we know what this func produces
		if (!acc) acc=xalloc(SCALAR(res) ? res->t : 0,obj->n); 
		else if (!LIST(acc) && res->t != acc->t) 
			acc = xl(acc);
		xfree(tmp);
		append(acc,res); }));
	// PF("each returning\n");DUMP(acc);
	return acc;
}
static inline VP eachprior(VP obj,VP fun) {
	ASSERT(1,"eachprior nyi");
	return (VP)0;
}
VP exhaust(VP x,VP y) {
	int i;
	PF("+++EXHAUST\n");DUMP(x);DUMP(y);
	IF_RET(CALLABLE(x), EXC(Tt(type),"exhaust y must be func or projection",x,y));
	IF_RET(x->n==0, ALLOC_LIKE_SZ(x,0));
	VP last=x,this=0;
	for(i=0;i<MAXSTACK;i++) {
		PF("exhaust calling #%d\n",i);DUMP(y);DUMP(this);DUMP(last);
		PFIN();
		this=apply(y,last);
		PFOUT();
		PF("exhaust result #%d\n",i);DUMP(last);DUMP(this);
		if(UNLIKELY(_equal(this,last))) {
			PF("exhaust = returning\n");DUMP(this);
			return this;
		} else 
			last=this;
	}
	return EXC(Tt(exhausted),"exhaust hit stack limit",x,last);
}
VP over(VP x,VP y) {
	PF("over\n");DUMP(x);DUMP(y);
	IF_RET(!CALLABLE(y), EXC(Tt(type),"over y must be func or projection",x,y));
	IF_RET(x->n==0, xalloc(x->t, 0));
	VP last,next;
	last=apply(x,xi(0));
	FOR(1,x->n,({
		next=apply(x, xi(_i));
		last=apply(apply(y,last),next);
	}));
	return last;
}
VP scan(VP x,VP y) { // always returns a list
	PF("scan\n");DUMP(x);DUMP(y);
	IF_RET(!CALLABLE(y), EXC(Tt(type),"scan y must be func or projection",x,y));
	IF_RET(x->n==0, xalloc(x->t, 0));
	IF_RET(x->n==1, x);
	VP last,next,acc=0;
	last=apply(x,xi(0));
	acc=ALLOC_LIKE(x);
	append(acc,last);
	FOR(1,x->n,({
		next=apply(x, xi(_i));
		last=apply(apply(y,last),next);
		PF("scan step\n");DUMP(last);
		append(acc,last);
	}));
	PF("scan result\n");DUMP(acc);
	return acc;
}
VP wide(VP obj,VP f) {
	int i; VP acc;
	PF("wide\n");DUMP(info(obj));DUMP(obj);DUMP(f);

	if(!CONTAINER(obj)) return apply(f, obj);

	PF("wide top level\n");DUMP(obj);
	acc=apply(f,obj);
	if(CONTAINER(acc)) {
		for(i=0;i<acc->n;i++) {
			PF("wide #%d\n",i);
			PFIN();
			EL(acc,VP,i)=wide(AS_l(acc,i),f);
			PFOUT();
		}
	}
	return acc;
}

// MATHY STUFF:

VP and(VP x,VP y) {
	int typerr=-1;
	VP acc;
	// PF("and\n"); DUMP(x); DUMP(y); // TODO and() and friends should handle type conversion better
	IF_EXC(x->n > 1 && y->n > 1 && x->n != y->n, Tt(len), "and arguments should be same length", x, y);	
	if(x->t == y->t) acc=xalloc(x->t, x->n);
	else acc=xlsz(x->n);
	VARY_EACHBOTH(x,y,({ 
		if (_x < _y) appendbuf(acc, (buf_t)&_x, 1); 
		else appendbuf(acc, (buf_t)&_y, 1); }), typerr);
	IF_EXC(typerr != -1, Tt(type), "and arg type not valid", x, y);
	// PF("and result\n"); DUMP(acc);
	return acc;
}
int _any(VP x) {
	int typerr=-1;
	VP acc;
	// PF("_any\n"); DUMP(x);
	if(LIST(x)) x=list2vec(deep(x,x1(&any)));
	VARY_EACH(x,({ 
		if(_x==1) return 1;
	}),typerr);
	// since this routine returns an int we can't return an exception!
	ASSERT(typerr==-1, "_any noniterable arg");
	// PF("_any returning 0\n");
	return 0;
}
VP any(VP x) {
	//PF("any\n");DUMP(x);
	IF_EXC(!SIMPLE(x) && !LIST(x), Tt(type), "any arg must be list or simple type", x, 0);
	if(LIST(x)) return deep(x,x1(&any));
	return xb(_any(x));
}
static inline VP divv(VP x,VP y) {
	int typerr=-1; VP acc=ALLOC_BEST(x,y);
	PF("div");DUMP(x);DUMP(y);DUMP(info(acc));
	if(UNLIKELY(!SIMPLE(x))) return EXC(Tt(type),"div argument should be simple types",x,0);
	VARY_EACHBOTH(x,y,({
		if(LIKELY(x->t > y->t)) { _x=_x/_y; appendbuf(acc,(buf_t)&_x,1); }
		else { _y=_y/_x; appendbuf(acc,(buf_t)&_y,1); }
		if(!SCALAR(x) && SCALAR(y)) _j=-1; // NB. AWFUL!
	}),typerr);
	IF_EXC(typerr > -1, Tt(type), "div arg wrong type", x, y);
	PF("div result\n"); DUMP(acc);
	return acc;
}
VP ifelse(VP x,VP y) {
	PF("ifelse\n");DUMP(x);DUMP(y);
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
	PF("greater\n"); DUMP(x); DUMP(y); 
	if(!SIMPLE(x) || !SIMPLE(y)) return EXC(Tt(type), "> args should be simple types", x, y);
	IF_EXC(x->n > 1 && y->n > 1 && x->n != y->n, Tt(len), "> arguments should be same length", x, y);	
	acc=xbsz(MAX(x->n,y->n));
	VARY_EACHBOTH(x,y,({ 
		if (_x <= _y) append(acc, v0); 
		else append(acc, v1); 
		if(!SCALAR(x) && SCALAR(y)) _j=-1; // NB. AWFUL!
	}), typerr);
	IF_EXC(typerr != -1, Tt(type), "> arg type not valid", x, y);
	PF("> result\n"); DUMP(acc);
	xfree(v0);xfree(v1);
	return acc;
}
VP lesser(VP x,VP y) {
	int typerr=-1;
	VP acc,v0=xb(0),v1=xb(1);
	PF("lesser\n"); DUMP(x); DUMP(y); 
	if(!SIMPLE(x) || !SIMPLE(y)) return EXC(Tt(type), "< args should be simple types", x, y);
	IF_EXC(x->n > 1 && y->n > 1 && x->n != y->n, Tt(len), "< arguments should be same length", x, y);	
	acc=xbsz(MAX(x->n,y->n));
	VARY_EACHBOTH(x,y,({ 
		if (_x >= _y) append(acc, v0); 
		else append(acc, v1); 
		if(!SCALAR(x) && SCALAR(y)) _j=-1; // NB. AWFUL!
	}), typerr);
	IF_EXC(typerr != -1, Tt(type), "< arg type not valid", x, y);
	PF("< result\n"); DUMP(acc);
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
VP mod(VP x,VP y) {
	// TODO mod probably *doesnt* need type promotion
	int typerr=-1;
	PF("mod\n");DUMP(x);DUMP(y);
	IF_EXC(!SIMPLE(x) || !SIMPLE(y), Tt(type), "mod args should be simple types", x, y); 
	VP acc=ALLOC_BEST(x,y);
	if(LIKELY(x->t > y->t)) {
		VARY_EACHBOTH_NOFLOAT(x,y,({ 
			if(_x==0)_x=0; else _x=_x%_y;
			appendbuf(acc,(buf_t)&_x,1); if(!SCALAR(x) && SCALAR(y)) _j=-1; // NB. AWFUL!
		}),typerr);
	} else {
		VARY_EACHBOTH_NOFLOAT(x,y,({ 
			if(_x==0)_y=0; else _y=_x%_y;
			appendbuf(acc,(buf_t)&_y,1); if(!SCALAR(x) && SCALAR(y)) _j=-1; // NB. AWFUL!
		}),typerr);
	}
	IF_EXC(typerr > -1, Tt(type), "mod arg wrong type", x, y);
	PF("mod result\n"); DUMP(acc);
	return acc;
}
VP not(VP x) {
	int typerr=-1;
	VP acc;
	// PF("and\n"); DUMP(x); DUMP(y); // TODO and() and friends should handle type conversion better
	acc=ALLOC_LIKE(x);
	VARY_EACH(x,({ 
		_x=!_x; appendbuf(acc,(buf_t)&_x,1);
	}),typerr);
	IF_EXC(typerr != -1, Tt(type), "not arg type not valid", x, 0);
	// PF("and result\n"); DUMP(acc);
	return acc;
}
VP or(VP x,VP y) { // TODO most of these primitive functions have the same pattern - abstract?
	int typerr=-1;
	VP acc;
	// PF("or\n"); DUMP(x); DUMP(y); // TODO or() and friends should handle type conversion better
	if(x->n==0) return y;
	if(y->n==0) return x;
	IF_EXC(x->n > 1 && y->n > 1 && x->n != y->n, Tt(len), "or arguments should be same length", x, y);	
	if(x->t == y->t) acc=xalloc(x->t, x->n);
	else acc=xlsz(x->n);
	VARY_EACHBOTH(x,y,({ if (_x > _y) appendbuf(acc, (buf_t)&_x, 1); 
		else appendbuf(acc, (buf_t)&_y, 1); }), typerr);
	IF_EXC(typerr != -1, Tt(type), "or arg type not valid", x, y);
	// PF("or result\n"); DUMP(acc);
	return acc;
}
VP plus(VP x,VP y) {
	int typerr=-1;
	// PF("plus\n");DUMP(x);DUMP(y);
	IF_EXC(!SIMPLE(x) || !SIMPLE(y), Tt(type), "plus args should be simple types", x, y); 
	VP acc=ALLOC_BEST(x,y);
	VARY_EACHBOTH(x,y,({
		if(LIKELY(x->t > y->t)) { _x=_x+_y; appendbuf(acc,(buf_t)&_x,1); }
		else { _y=_y+_x; appendbuf(acc,(buf_t)&_y,1); }
		if(!SCALAR(x) && SCALAR(y)) _j=-1; // NB. AWFUL!
	}),typerr);
	IF_EXC(typerr > -1, Tt(type), "plus arg wrong type", x, y);
	// PF("plus result\n"); DUMP(acc);
	return acc;
}
static inline VP str2num(VP x) {
	// TODO optimize str2int
	double d; I128 buf=0;char* s=sfromx(flatten(x));
	PF("str2num %s\n",s);DUMP(x);
	IF_EXC(!IS_c(x),Tt(type),"str2int arg should be char vector",x,0);
	if(strchr(s,'.')!=0 && (d=strtod(s,NULL))!=0) {
		return xf(d);
	} else if (sscanf(s,"%lld",&buf)==1) { // should probably use atoi or strtol
		/* assume int by default 
		if(buf<MAX_b)
			return xb((CTYPE_b)buf);
		*/
		if(buf<MAX_i)
			return xi((CTYPE_i)buf);
		if(buf<MAX_j)
			return xj((CTYPE_j)buf);
		return xo((CTYPE_o)buf);
	} 
	else if(strncmp(s,"0.0",3)==0)
		return xf(0.0);
	else 
		return x;
	// return EXC(Tt(value),"str2int value could not be converted",x,0);
}
VP sum(VP x) {
	PF("sum");DUMP(x);
	I64 val=0;int i,typerr=-1;
	if(UNLIKELY(!SIMPLE(x))) return EXC(Tt(type),"sum argument should be simple types",x,0);
	VARY_EACH(x,({ val += _x; }),typerr);
	IF_EXC(typerr > -1, Tt(type), "sum arg wrong type", x, 0);
	return xj(val);
}
VP sums(VP x) {
	PF("sums\n");DUMP(x);
	if(UNLIKELY(!SIMPLE(x))) return EXC(Tt(type),"sums argument should be simple types",x,0);
	VP acc=ALLOC_LIKE(x); int typerr=-1;
	VARY_EACH(x,({ _xtmp += _x; appendbuf(acc,(buf_t)&_xtmp,1); }),typerr);
	IF_EXC(typerr > -1, Tt(type), "sums arg wrong type", x, 0);
	PF("sums result\n"); DUMP(acc);
	return acc;
}
VP count(VP x) {
	VP acc=0;int i;int typerr=-1;
	PF("count\n"); DUMP(x);
	VARY_EL_NOFLOAT(x, 0, 
		{ __typeof__(_x) i; acc=xalloc(x->t,MAX(_x,1)); acc->n=_x; for(i=0;i<_x;i++) { EL(acc,__typeof__(i),i)=i; } }, 
		typerr);
	IF_RET(typerr>-1, EXC(Tt(type), "count arg must be numeric", x, 0));
	DUMP(acc);
	return acc;
}
static inline VP times(VP x,VP y) {
	int typerr=-1; VP acc=ALLOC_BEST(x,y);
	PF("times");DUMP(x);DUMP(y);DUMP(info(acc));
	if(UNLIKELY(!SIMPLE(x))) return EXC(Tt(type),"times argument should be simple types",x,0);
	VARY_EACHBOTH(x,y,({
		if(LIKELY(x->t > y->t)) { _x=_x*_y; appendbuf(acc,(buf_t)&_x,1); }
		else { _y=_y*_x; appendbuf(acc,(buf_t)&_y,1); }
		if(!SCALAR(x) && SCALAR(y)) _j=-1; // NB. AWFUL!
	}),typerr);
	IF_EXC(typerr > -1, Tt(type), "times arg wrong type", x, y);
	PF("times result\n"); DUMP(acc);
	return acc;
}
VP xor(VP x,VP y) { 
	int typerr=-1;
	VP acc;
	PF("xor\n"); DUMP(x); DUMP(y); // TODO or() and friends should handle type conversion better
	if(UNLIKELY(!SIMPLE(x) && !SIMPLE(y))) return EXC(Tt(type),"xor argument should be simple types",x,0);
	IF_EXC(x->n > 1 && y->n > 1 && x->n != y->n, Tt(len), "xor arguments should be same length", x, y);	
	if(x->t == y->t) acc=xalloc(x->t, x->n);
	else acc=xlsz(x->n);
	VARY_EACHBOTH_NOFLOAT(x,y,({ _x = _x ^ _y; appendbuf(acc, (buf_t)&_x, 1); }), typerr);
	IF_EXC(typerr != -1, Tt(type), "xor arg type not valid", x, y);
	// PF("or result\n"); DUMP(acc);
	return acc;
}

// MANIPULATING LISTS AND VECTORS (misc):

VP key(VP x) {
	if(DICT(x)) return ELl(x,0);
	if(IS_x(x)){ // locals for context
		int i;VP item;
		for(i=x->n-1;i>=0;i--) 
			if(DICT(ELl(x,i)))
				return ELl(ELl(x,i),0);
		return xd0();
	}
	if(SIMPLE(x)) return count(xi(x->n));
	return EXC(Tt(type),"key can't operate on that type",x,0);
}

VP val(VP x) {
	if(DICT(x)) return ELl(x,1);
	return EXC(Tt(type),"val can't operate on that type",x,0);
}

VP get(VP x,VP y) {
	// TODO get support nesting
	int i,j; VP res;
	PF("get\n");DUMP(y);
	if(IS_x(x)) {
		if(LIKELY(IS_c(y) || (LIST(y) && IS_c(ELl(y,0))))) 
			y=str2tag(y);
		DUMP(y);
		if(IS_t(y) && y->n > 1 && AS_t(y,0)==0) { // empty first element = start from root
			PF("get starting from root\n");
			i=0;y=list(behead(y));
			for(;i<x->n;i++) {
				PF("get #%d\n", i);
				if(LIST(ELl(x,i))) continue; // skip code bodies - maybe should use tags for this?
				res=apply(ELl(x,i),y);
				if(!LIST(res) || res->n > 0) return res;
			}
		} else {
			i=x->n-1;
			for(;i>=0;i--) {
				PF("get #%d\n", i);
				if(LIST(ELl(x,i))) continue; // skip code bodies - maybe should use tags for this?
				res=apply(ELl(x,i),y);
				if(!LIST(res) || res->n > 0) return res;
			}
		}
		return EXC(Tt(undef),"undefined",y,x);
	}
	return apply(x,y);
}

VP set(VP x,VP y) {
	// TODO set needs to support nesting
	int i; VP res,ctx,val;
	PF("set\n");DUMP(x);DUMP(y);
	if(LIST(x)) {
		if(!IS_x(AS_l(x,0))) return EXC(Tt(type),"set x must be (context,value)",x,y);
		if(x->n!=2) return EXC(Tt(type),"set x must be (context,value)",x,y);
		if(!IS_t(y)) return EXC(Tt(type),"set y must be symbol",x,y);
		ctx=AS_l(x,0);val=AS_l(x,1); i=ctx->n-1;

		// TODO rewrite these to share logic in the body of loop .. must be an easy
		// way without using a helper func or macro
		if(IS_t(y) && y->n > 1 && AS_t(y,0)==0) { // empty first element = start from root
			PF("set starting from root\n");
			i=0;y=list(behead(y));
			for(;i<ctx->n;i++) {
				VP dest = AS_x(ctx,i);
				if(LIST(dest) || CALLABLE(dest)) // skip code bodies
					continue;
				if(DICT(dest)) {
					PF("set assigning in %d..\n", i);
					dest=assign(dest,y,val);
					PF("set in %p\n",dest);
					DUMP(dest);
					return val;
				}
			}
		} else {
			for(;i>=0;i--) {
				VP dest = AS_x(ctx,i);
				if(LIST(dest) || CALLABLE(dest)) // skip code bodies
					continue;
				if(DICT(dest)) {
					PF("set assigning..\n");
					dest=assign(dest,y,val);
					PF("set in %p\n",dest);
					DUMP(dest);
					return val;
				}
			}
		}
		return EXC(Tt(set),"could not set value in parent scope",x,y);
	}
	return xl0();
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
			// PF("pg inner %d %d\n", _xtmp, _x);
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
	// PF("partgroups returning\n");DUMP(acc);
	xfree(tmp);
	return acc;
}

VP pick(VP x,VP y) { // select items of x[0..n] where y[n]=1
	VP acc;int n=0,typerr=-1;
	IF_EXC(!SIMPLE(x) || !SIMPLE(y), Tt(type), "pick args should be simple types", x, y); // TODO pick: gen lists
	PF("pick\n");DUMP(x);DUMP(y);
	acc=ALLOC_LIKE_SZ(x,x->n/2);
	VARY_EACHBOTHLIST(x,y,({
		if(_y) {
			// PF("p %d %d/%d %d %d/%d s%d\n", _x,_i,_xn,_y,_j,_yn,SCALAR(x));
			acc=appendbuf(acc,(buf_t)&_x,1);
			n++;
		}
	}),typerr);
	IF_EXC(typerr > -1, Tt(type), "pick arg wrong type", x, y);
	PF("pick result\n"); DUMP(acc);
	return acc;
}
VP pickapart(VP x,VP y) { // select items of x[0..n] where y[n]=1, and divide non-consecutive regions
	VP acc, sub=NULL;int n=0,typerr=-1;
	IF_EXC(!SIMPLE(x) || !SIMPLE(y), Tt(type), "pickapart args should be simple types", x, y); // TODO pick: gen lists
	PF("pickapart\n");DUMP(x);DUMP(y);
	acc=xlsz(4);
	VARY_EACHBOTHLIST(x,y,({
		PF("%d ",_y);
		if(_y) {
			if (!sub) sub=ALLOC_LIKE_SZ(x,x->n/2);
			sub=appendbuf(sub,(buf_t)&_x,1);
		} else {
			if (sub) { acc=append(acc,sub); xfree(sub); sub=NULL; }
		}
	}),typerr);
	IF_EXC(typerr > -1, Tt(type), "pickapart arg wrong type", x, y);
	if (sub) { acc=append(acc,sub); xfree(sub); sub=NULL; }
	PF("pickapart result\n"); DUMP(acc);
	if(acc->n==0) return ELl(acc,0);
	else return acc;
}
VP proj(int type, void* func, VP left, VP right) {
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


// TAG STUFF:

static inline VP str2tag(VP str) { // turns string, or list of strings, into tag vector
	int i=0; VP acc=xtsz(str->n);
	if(IS_c(str)) return xt(_tagnum(str));
	if(LIST(str)) {
		VP tmp;
		for(;i<str->n;i++) {
			tmp=ELl(str,i);
			if(!IS_c(tmp)) return EXC(Tt(type),"str2tag arg not string or list of strings",str,0);
			acc=append(acc,xt(_tagnum(tmp)));
		}
		return acc;
	}
	return EXC(Tt(type),"str2tag arg not string or list of strings",str,0);
}
static inline VP tagwrap(VP tag,VP x) {
	return entag(xln(1, x),tag);
}
/*static inline 
*/
VP tagv(const char* name, VP x) {
	return entags(xln(1,x),name);
}
static inline VP entag(VP x,VP t) {
	if(IS_c(t))
		x->tag=_tagnum(t);
	else if (IS_i(t))
		x->tag=AS_i(t,0);
	return x;
}
static inline VP entags(VP x,const char* name) {
	x->tag=_tagnums(name);
	return x;
}
static inline VP tagname(const I32 tag) {
	VP res;
	if(TAGS==NULL) { TAGS=xl0();TAGS->rc=INT_MAX; }
	if(tag>=TAGS->n) return xfroms("unknown");
	res = ELl(TAGS,tag);
	return res;
}
static inline const char* tagnames(const I32 tag) {
	return sfromx(tagname(tag));
}
static inline int _tagnum(const VP s) {
	int i; VP ss=0;
	WITHLOCK(tag, {
		ss=s;ss->tag=0;
		if(TAGS==NULL) { TAGS=xl0();TAGS->rc=INT_MAX;upsert(TAGS,xfroms("")); PF("new tags\n"); DUMP(TAGS); }
		i=_upsertidx(TAGS,s);
	});
	return i;
}
/* static inline  */
int _tagnums(const char* name) {
	int t;VP s;
	//printf("_tagnums %s\n",name);
	//printf("tagnums free\n");
	// DUMP(TAGS);
	s=xfroms(name); t=_tagnum(s); xfree(s); return t;
}

// JOINS (so to speak)
// possibly useless

VP bracketjoin(VP x,VP y) { 
	// returns x[n] when 'on'
	//  turned on by y[0][n]=1
	//  turned off by y[1][n]=1
	// otherwise 0
	// useful for matching patterns involving more than one entity
	int i,on=0,typerr=-1; VP c,ret,acc,y0,y1,mx;
	PF("bracketjoin\n");DUMP(x);DUMP(y);
	IF_EXC(!LIST(y)||y->n!=2,Tt(type),"bracketjoin y must be 2-arg list",x,y);
	y0=ELl(y,0); y1=ELl(y,1);
	IF_EXC(y0->t != y1->t,Tt(type),"bracketjoin y items must be same type",x,y);
	acc=plus(y0, times(xi(-1),y1));
	DUMP(acc);
	acc=sums(acc);
	PF("bracket sums\n");DUMP(acc);
	mx=max(acc);
	PF("bracket max\n");DUMP(mx);
	PF("bracket x\n");DUMP(x);
	ret=take(xi(0),xi(y0->n));
	if(EL(mx,CTYPE_b,0)==0) { PF("bracketjoin no coverage\n"); DUMP(acc); return ret; }
	c=ELl(partgroups(condense(and(x,matcheasy(acc,mx)))),0);
	DUMP(c);
	if(c->n) {
		c=append(c,plus(max(c),xi(1)));
		PF("bracket append next\n");
		DUMP(c);
	}
	ret=assign(ret,c,xi(1));
	// acc=pick(x,matcheasy(acc,mx));
	PF("bracket acc after pick");DUMP(ret);
	return ret;
	acc = ALLOC_LIKE_SZ(x,y0->n);
	VARY_EACHRIGHT(x,y0,({
		if(_y == 1) on++;
		if(on) EL(acc,typeof(_x),_j)=EL(x,typeof(_x),_j % x->n);
		else EL(acc,typeof(_x),_j)=0;
		// NB. the off channel affects the *next element*, not this one - maybe not right logic
		if(_j < y1->n && EL(y1,typeof(_y),_j)==1) on--;
	}),typerr);
	IF_EXC(typerr>-1,Tt(type),"bracketjoin couldnt work with those types",x,y);
	acc->n=y0->n;
	mx=max(acc);
	PF("bracketjoin max\n");DUMP(mx);
	acc=matcheasy(acc,mx);
	PF("bracketjoin return\n");DUMP(acc);
	return acc;
}
VP consecutivejoin(VP x, VP y) {
	// returns x[n] if y[0][n]==1 && y[1][n+1]==1 && .. else 0
	int j,n=y->n, typerr=-1, on=0; VP acc,tmp;
	PF("consecutivejoin\n"); DUMP(x); DUMP(y);
	
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
	PF("consecutivejoin return\n"); DUMP(acc);
	return acc;
}
VP signaljoin(VP x,VP y) {
	// could be implemented as +over and more selection but this should be faster
	int typerr=-1, on=0; VP acc;
	PF("signaljoin\n");DUMP(x);DUMP(y);
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
	PF("signaljoin return\n");DUMP(acc);
	return acc;
}

// MATCHING

VP nest(VP x,VP y) {
	VP p1,p2,open,close,opens,closes,where,rep,out;
	PF("NEST\n");DUMP(x);DUMP(y);
	//if(!LIST(x) || x->n < 2) return x;
	if(x->n<2)return x;
	p1=proj(2,&matcheasy,x,0);
	p2=proj(2,&matcheasy,x,0);
	open=apply(y,xi(0)); close=apply(y,xi(1));
	if(!LIST(x) && x->t != open->t) return x;
	// if(LIST(open) && x->t != AS_l(open,0)->t) return x;
	if(_equal(open,close)) {
		opens=each(open,p1);
		PF("+ matching opens\n");DUMP(opens);
		if(_any(opens)) {
			VP esc = 0;
			if(y->n >= 3) {
				esc = matcheasy(x,ELl(y,2));
				PF("escapes\n");DUMP(esc);
				EL(opens,VP,0)=and(AS_l(opens,0),shift_(not(esc),-1));
				PF("new escaped opens\n");
			}	
			opens=signaljoin(xb(1),AS_l(opens,0));
			PF("after signaljoin\n");DUMP(opens);
			out=partgroups(condense(opens));
			if(out->n) {
				out=AS_l(out,0);
				PF("matching pre-append"); DUMP(out);
				out=append(out,plus(max(out),xi(1)));
				PF("matching post-append"); DUMP(out);
			}
			DUMP(out);
			where=out;
		} else 
			where=xl0();
		// exit(1);
	} else {
		opens=each(open,p1);
		PF("+ opens\n");DUMP(opens);
		closes=each(close,p2);
		PF("- closes\n");DUMP(closes);
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
		out=bracketjoin(xb(1), xln(2,consecutivejoin(xb(1),opens),closes)); 
		DUMP(out);
		where=condense(out);
	}
	PF("nest where\n");DUMP(where);
	if(where->n) {
		rep=apply(x,where);
		if(y->n >= 5) {
			rep=apply(ELl(y,4),rep);
		}
		if(y->n >= 4)
			rep->tag=AS_t(ELl(y,3),0);
		rep=list2vec(rep);
		PF("nest rep\n");DUMP(rep);
		// splice is smart enough to merge like-type replace args into one
		// like-typed vector. but that's not what we want here, because the
		// thing we're inserting is a "child" of this position, so we want to
		// ensure we always splice in a list
		PF("nest x");
		out=splice(split(x,xi0()),where,rep);
		PF("nest out\n");DUMP(out);
		if(!LIST(out)) out=xl(out);
	} else { out = x; }
	PF("nest returning\n"); DUMP(out);
	return out;
}
VP matchany(VP obj,VP pat) {
	IF_EXC(!SIMPLE(obj) && !LIST(obj),Tt(type),"matchany only works with simple or list types in x",obj,pat);
	// IF_EXC(!SIMPLE(pat),Tt(type),"matchany only works with simple types in y",obj,pat);
	IF_EXC(SIMPLE(obj) && obj->t != pat->t, Tt(type),"matchany only works with matching simple types",obj,pat);
	int j,n=obj->n,typerr=-1;VP item, acc;
	PF("matchany\n"); DUMP(obj); DUMP(pat);
	acc=xbsz(n); 
	acc->n=n;
	if(LIST(obj)) {
		VP this;
		FOR(0,n,({ 
			this=ELl(obj,_i);
			if((pat->tag==0 || pat->tag==this->tag) && _find1(pat,this) != -1) {
				PF("matchany found list at %d\n", _i);
				EL(acc,CTYPE_b,_i)=1; }}));
	} else {
		VARY_EACHLEFT(obj, pat, ({
			// TODO matchany(): buggy subscripting:
			if((pat->tag==0 || pat->tag==obj->tag) && _findbuf(pat, (buf_t)&_x) != -1) {
				PF("matchany found simple at %d\n", _i);
				EL(acc,CTYPE_b,_i) = 1;
			}
		}), typerr);
		IF_EXC(typerr>-1, Tt(type), "matchany could not match those types",obj,pat);
	}
	PF("matchany result\n"); DUMP(acc);
	return acc;
}
VP matcheasy(VP obj,VP pat) {
	IF_EXC(!SIMPLE(obj) && !LIST(obj),Tt(type),"matcheasy only works with numeric or string types in x",obj,pat);
	int j,n=obj->n,typerr=-1;VP item, acc;
	PF("matcheasy\n"); DUMP(obj); DUMP(pat);
	acc=xbsz(n); // TODO matcheasy() should be smarter about initial buffer size
	acc->n=n;

	if(CALLABLE(pat)) {
		PF("matcheasy callable\n");
		return each(obj, pat);
	}

	if(LIST(obj)) {
		FOR(0,n,({ 
			if((pat->tag == 0 || pat->tag==obj->tag) && _equal(ELl(obj,_i),pat)) 
				EL(acc,CTYPE_b,_i)=1; }));
	} else {
		VARY_EACHLEFT(obj, pat, ({
			if(_x == _y) EL(acc,CTYPE_b,_i) = 1;
		}), typerr);
		IF_EXC(typerr>-1, Tt(type), "matcheasy could not match those types",obj,pat);
	}
	PF("matcheasy result\n"); DUMP(acc);
	return acc;
}
VP matchexec(VP obj,VP pats) {
	int i,j,diff;VP rule,res,res2,sel;
	ASSERT(LIST(pats)&&pats->n%2==0,"pats should be a list of [pat1,fn1,pat2,fn2..]");
	PF("matchexec start\n");
	DUMP(obj);
	DUMP(pats);
	for(i=0;i<pats->n;i+=2) {
		PF("matchexec %d\n", i);
		rule=apply(pats,xi(i));
		if(IS_t(rule)) 
			res=matchtag(obj,rule);
		else
			res=matchany(obj,rule);
		PF("matchexec match, rule and res:\n");
		DUMP(rule);
		DUMP(res);
		// rules start with an unimportant first item: empty tag for tagmatch
		if(_any(res)) {
			VP indices = partgroups(condense(res));
			diff = 0;
			for (j=0; j<indices->n; j++) {
				VP idx = ELl(indices, j);
				PF("matchexec idx, len=%d, diff=%d\n", idx->n, diff); DUMP(idx);
				res2=apply(ELl(pats,i+1),apply(obj,plus(idx, xi(diff))));
				if(LIST(res2) && res2->n == 0) continue;
				PF("matchexec after apply, len=%d\n", res2->n);
				DUMP(res2);
				obj=splice(obj,plus(idx, xi(diff)),res2);
				diff += 1 - idx->n;
				PF("matchexec new obj, diff=%d", diff);
				DUMP(obj);
			}
		}
	}	
	PF("matchexec done");
	DUMP(obj);
	return obj;
}
VP matchtag(VP obj,VP pat) {
	IF_EXC(!SIMPLE(obj) && !LIST(obj),Tt(type),"matchtag only works with numeric or string types in x",obj,pat);
	int j,n=obj->n,typerr=-1;VP item, acc;
	PF("matchtag\n"); DUMP(obj); DUMP(pat);
	acc=xbsz(n); // TODO matcheasy() should be smarter about initial buffer size
	acc->n=n;
	if(LIST(obj)) {
		FOR(0,n,({ 
			if(AS_t(pat,0) == ELl(obj,_i)->tag) 
				EL(acc,CTYPE_b,_i)=1; }));
	} else {
		VARY_EACHLEFT(obj, pat, ({
			if(AS_t(pat,0) == obj->tag) EL(acc,CTYPE_b,_i) = 1;
		}), typerr);
		IF_EXC(typerr>-1, Tt(type), "matchtag could not match those types",obj,pat);
	}
	PF("matchtag result\n"); DUMP(acc);
	return acc;
}

// CONTEXTS:

VP rootctx() {
	VP res;
	res=xd0();
	#include"rootctx.h"
	PF("rootctx returning\n");
	DUMP(res);
	return res;
}
VP mkbarectx() {
	char name[8];
	return xx0();
}
VP mkworkspace() {
	char name[8];
	VP root,res,locals;
	snprintf(name,sizeof(name),"wk%-6d", rand());
	res=xx0(); root=rootctx(); locals=xd0();
	assign(root,Tt(name),locals);
	assign(locals,Tt(wkspc),xfroms(name));
	res=append(res,root);
	res=append(res,locals);
	return res;
}
VP eval(VP code) {
	ASSERT(1, "eval nyi");
	return (VP)0;
}
VP list2vec(VP obj) {
	// Collapses lists that contain all the same kind of vector items into a
	// single vector [1,2,3i] = (1,2,3i) Will NOT collapse [1,(2,3),4] - use
	// flatten for this. (See note below for non-flat first items) The original
	// list will be returned when rejected for massaging.
	int i, t=0;
	VP acc,this;
	PF("list2vec\n"); DUMP(obj);
	if(!LIST(obj)) return obj;
	if(!obj->n) return obj;
	acc=ALLOC_LIKE(ELl(obj,0));
	if(obj->tag!=0) acc->tag=obj->tag;
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
	PF("list2vec result\n"); DUMP(acc); DUMP(info(acc));
	return acc;
}
VP labelitems(VP label,VP items) {
	VP res;
	//PF("labelitems\n");DUMP(label);DUMP(items);
	res=flatten(items);res->tag=_tagnums(sfromx(label));
	//DUMP(res);
	return res;
}
VP mklexer(const char* chars, const char* label) {
	return xln(2,
		entags(xfroms(chars),"raw"),
		proj(2,&labelitems,xfroms(label),0)
	);
}
VP parseexpr(VP x) {
	PF("parseexpr\n");DUMP(x);
	if(LIST(x) && IS_c(ELl(x,0)) && 
			((AS_c(ELl(x,0),0)=='(') ||
			  AS_c(ELl(x,0),0)=='['))
		return drop_(drop_(x,-1),1);
	else
		return x;
}
VP parsename(VP x) {
	PF("parsename\n");DUMP(x);
	VP res=flatten(x);
	if(IS_c(res)) {
		if(AS_c(res,0)=='\'') {
			PF("parsename tag\n");
			res=behead(res);
			res=split(res,xc('.')); // very fast if not found
			DUMP(res);
			return str2tag(res);
		} else {
			PF("parsename non-tag\n");
			res=split(res,xc('.')); // very fast if not found
			DUMP(res);
			res->tag=Ti(name);
		}
	}
	return res;
}
VP parsenum(VP x) {
	PF("parsenum\n");DUMP(x);
	VP res=flatten(x);
	if(IS_c(res)) {
		return str2num(res);
	} else return res;
}
VP parselambda(VP x) {
	int i,arity=1,typerr=-1,traw=Ti(raw); VP this;
	PF("parselambda\n");DUMP(x);
	x=list(x);
	for(i=0;i<x->n;i++) {
		this=ELl(x,i);
		if(IS_c(this) && this->tag==traw && AS_c(this,0)=='y') { 
			arity=2;break;
		}
	};
	if(LIST(x) && IS_c(ELl(x,0)) && AS_c(ELl(x,0),0)=='{')
		return entags(xln(2,drop_(drop_(x,-1),1),xi(arity)),"lambda");
	else return x;
}
VP parsestrlit(VP x) {
	int i,arity=1,typerr=-1,traw=Ti(raw);
	PF("PARSESTRLIT!!!\n");DUMP(x);
	if(LIST(x) && IS_c(AS_l(x,0)) && AS_c(AS_l(x,0),0)=='"') {
		VP res=xlsz(x->n), el, next; int ch,nextch,last;
		last=x->n-1;
		for(i=0;i<x->n;i++) {
			PF("parsestrlit #%d/%d\n",i,last);
			el=AS_l(x,i);
			DUMP(el);
			if(IS_c(el)) {
				ch=AS_c(el,0);
				PF("parselit ch=%c\n",ch);
				if ((i==0 || i==last) && ch=='"')
					continue; // skip start/end quotes
				if (i<last &&
				    (ch=AS_c(el,0))=='\\') {
					PF("investigating %d\n",i+1);
					next=AS_l(x,i+1);
					if(IS_c(next) && next->n) {
						nextch=AS_c(next,0);
						if(nextch=='n') {
							res=append(res,xc(10)); i++;
						} else if(nextch=='r') {
							res=append(res,xc(13)); i++;
						}
					}
				} else  
					res=append(res,el);
			} else {
				res=append(res,el);
			}
		}
		// due to the looping logic, we would wind up with an empty list - we want an empty list with an empty string! :)
		if(res->n==0) res=append(res,xc0()); 
		PF("flattenin\n");DUMP(res);
		res=flatten(res);
		DUMP(res);
		return res;
	} else {
		PF("parsestrlit not interested in\n");
		DUMP(x);
		return x;
	}
}
VP parsestr(const char* str) {
	VP ctx,lex,pats,acc,t1,t2;size_t l=strlen(str);int i;
	PF("parsestr '%s'\n",str);
	acc=xlsz(l);
	for(i=0;i<l;i++)
		append(acc,entags(xc(str[i]),"raw"));
	if(AS_c(ELl(acc,acc->n - 1),0)!='\n')
		append(acc,entags(xc('\n'),"raw"));
	ctx=mkbarectx();
	pats=xln(3,
		proj(2,&nest,0,xln(4, xfroms("//"), xfroms("\n"), xfroms(""), Tt(comment))),
		proj(2,&nest,0,xln(4, xfroms("/*"), xfroms("*/"), xfroms(""), Tt(comment))),
		proj(2,&nest,0,xln(5, xfroms("\""), xfroms("\""), xfroms("\\"), Tt(string), x1(&parsestrlit)))
	);
	ctx=append(ctx,pats);
	acc=exhaust(acc,ctx);
	PF("parsestr after nest\n");
	xfree(pats);

	pats=xl0();
	lex=mklexer("0123456789.","num");
	append(pats,ELl(lex,0));
	append(pats,x1(&parsenum));
	xfree(lex);
	lex=mklexer("'abcdefghijklmnopqrstuvwxyz.?","name");
	append(pats,ELl(lex,0));
	append(pats,x1(&parsename));
	xfree(lex);
	lex=mklexer(" \n\t","ws");
	append(pats,ELl(lex,0));
	append(pats,ELl(lex,1));
	xfree(lex);
	t1=matchexec(acc,pats);

	xfree(pats);
	xfree(acc);
	//xfree(ctx);
	PF("matchexec results\n");DUMP(t1);

	// we only form expression trees after basic parsing - this saves us from
	// having to do a bunch of deep manipulations earlier in this routine (i.e.,
	// its faster to scan a flat list)
	ctx=mkbarectx();
	pats=xln(3,
		proj(2,&nest,0,xln(5, xfroms("{"), xfroms("}"), xfroms(""), Tt(lambda), x1(&parselambda))),
		proj(2,&nest,0,xln(5, xfroms("("), xfroms(")"), xfroms(""), Tt(expr), x1(&parseexpr))),
		proj(2,&nest,0,xln(5, xfroms("["), xfroms("]"), xfroms(""), Tt(listexpr), x1(&parseexpr)))
	);
	t2=t1;
	for(i=0;i<pats->n;i++) {
		PF("parsestr exhausting %d\n", i);
		t2=exhaust(t2,proj(2,&wide,0,ELl(pats,i)));
	}
	return t2;
}

VP parse(VP x) {
	return parsestr(sfromx(x));
}

// STANDARD LIBRARY

VP fileget(VP fn) {
	#define READFILEBLK 1024 * 64
	char buf[READFILEBLK]; int r=0,fd=0;
	if(!IS_c(fn)) return EXC(Tt(type),"readfile filename must be string",fn,0);
	fd=open(sfromx(fn),O_RDONLY);
	if(fd<0) return EXC(Tt(open),"could not open file for reading",fn,0);
	VP acc=xcsz(2048);
	do {
		r=read(fd,buf,READFILEBLK);
		if(r<0) { xfree(acc); close(fd); return EXC(Tt(read),"could not read from file",fn,0); }
		else appendbuf(acc,(buf_t)buf,r);
	} while (r==READFILEBLK);
	close(fd);
	return acc;
}
VP fileset(VP str,VP fn) {
	if(!IS_c(str)) return EXC(Tt(type),"writefile only deals writes strings right now",str,fn);
	if(!IS_c(fn)) return EXC(Tt(type),"writefile filename must be string",str,fn);
	int fd=open(sfromx(fn),O_WRONLY);
	if(fd<0) return EXC(Tt(open),"could not open file for writing",str,fn);
	if(write(fd,ELb(str,0),str->n)<str->n) return EXC(Tt(write),"could not write file contents",str,fn);
	close(fd);
	return str;
}

// Threading
void thr_start() {
	// TODO threading on Windows
	#ifndef THREAD
	#else
	NTHR=0;
	#endif
	return;
}
void* thr_run0(void* VPctx) {
	#ifndef THREAD
	#else
	VP ctx=VPctx;
	PFW(({
	printf("thr_run %s\n", reprA(ctx));
	ctx=apply(ctx,xl0());
	DUMP(ctx);
	}));
	pthread_exit(NULL);
	#endif
	return 0;
}
void thr_run(VP ctx) {
	#ifndef THREAD
	apply(ctx,xl0());
	#else
	pthread_attr_t a; pthread_attr_init(&a); pthread_attr_setdetachstate(&a, PTHREAD_CREATE_JOINABLE);
	// nthr=sysconf(_SC_NPROCESSORS_ONLN);if(nthr<2)nthr=2;
	WITHLOCK(thr,pthread_create(&THR[NTHR++], &a, &thr_run0, ctx));
	#endif
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
	DUMP(jsrc);
	res=nest(jsrc,xln(2,xfroms("["),xfroms("]")));
	DUMP(res);
	DUMP(each(res, x1(&repr)));
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
void test_proj() {
	VP a,b,c,n;
	printf("TEST_PROJ\n");
	n=xi(1024*1024);
	//a=proj(1,&count,n,0);
	a=x1(&count);
	b=apply(a,n);
	PF("b\n");DUMP(b);
	c=apply(proj(1,&sum,b,0),0);
	PF("result\n");DUMP(c);
	//printf("%lld\n", AS_o(c,0));
	xfree(a);xfree(b);xfree(c);xfree(n);
	//DUMP(c);
}
void test_proj_thr0(void* _) {
	/*
	VP a,b,c,n; int i;
	for (i=0;i<1024;i++) {
		printf("TEST_PROJ %d\n", pthread_self());
		n=xi(1024*1024);
		//a=proj(1,&count,n,0);
		a=x1(&count);
		b=apply(a,n);
		PF("b\n");DUMP(b);
		c=apply(proj(1,&sum,b,0),0);
		PF("result\n");DUMP(c);
		printf("%lld\n", AS_o(c,0));
		xfree(a);xfree(b);xfree(c);xfree(n);
	}
	return;
	*/
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
	thr_start();
	for(i=0;i<n;i++) thr_run(test_proj_thr0);
	thr_wait();
	*/
}
VP evalin(VP tree,VP ctx) {
	if(IS_c(tree)) return evalstrin(sfromx(tree),ctx);
	if(!IS_x(ctx))
		ctx=xxn(2,ctx,tree);
	else
		append(ctx,tree);
	return applyctx(ctx,0); 
}
VP evalstrin(const char* str, VP ctx) {
	VP r;
	PF("evalstrin\n\"%s\"\n",str);
	VP p=parsestr(str);
	r=evalin(p,ctx);
	PF("evalstrin returning\n");DUMP(r);
	return r;
}
void evalfile(VP ctx,const char* fn) {
	#define EFBLK 65535
	int fd,r;char buf[EFBLK];VP acc=xcsz(1024),res;
	fd=open(fn,O_RDONLY);
	if(fd<0)return perror("evalfile open");
	do {
		r=read(fd,buf,EFBLK);
		if(r<0)perror("evalfile read");
		else appendbuf(acc,(buf_t)buf,r);
	} while (r==EFBLK);
	PFW({
	PF("evalfile executing\n"); DUMP(acc);
	append(ctx,parsestr(sfromx(acc)));
	res=apply(ctx,xl0()); // TODO define global constant XNULL=xl0(), XI1=xi(1), XI0=xi(0), etc..
	PF("evalfile done"); DUMP(res);
	});
	printf("%s\n",repr(res));
	// exit(1); fall through to repl
}
void tests() {
	int i;
	VP a,b,c;
	// xprofile_start();
	
	if (DEBUG) {
		// xprofile_start();
		printf("TESTS START\n");
		test_basics();
		test_nest();
		// test_json();
		test_ctx();
		test_eval();
		test_logic();
		printf("TESTS PASSED\n");
		// test_proj_thr();
		// xprofile_end();
		if(MEM_W) {
			PF("alloced = %llu, freed = %llu\n", MEM_ALLOC_SZ, MEM_FREED_SZ);
		}
	}
}
void init(){
	XI0=xi(0); XI1=xi(1);
	thr_start();
}
int main(int argc, char* argv[]) {
	init();
	VP ctx=mkworkspace();
	// net();
	if(argc == 2) evalfile(ctx,argv[1]);
	else tests();
	repl();
	exit(1);
}
/*
	
	TODO diff: (1,2,3,4)diff(1,2,55) = [['set,2,55],['del,3]] (plus an easy way to map that diff to funcs to perform)
	TODO set PF_LVL from code via 'xray' or 'trace' vals
	TODO mailboxes
	TODO some kind of backing store for contexts cant stand losing my work
	TODO decide operator for typeof
	TODO decide operator for tagof
	TODO decide operator for applytag
	TODO can we auto-parallelize some loops?

*/ 
