// XXL - a minimalistic programming language
// (C) Copyright 2016 Thomas Lackner; 3 clause BSD-L

#include "compile.h"                   // make xxl aware of its own compilation settings
#include "def.h"                       // helper macros and main type defs
#include "proto.h"                     // prototypes
#include "accessors.h"
#include "vary.h"

// GLOBALS (dangggerous)

// commonly used "static" values, set in init()
VP XB0=NULL,XB1=NULL,XI0=NULL,XI1=NULL;
THREADLOCAL VP XXL_SYS=NULL; 

I8 PF_ON=0; I8 PF_LVL=0;               // controls debugging output on/off/nesting depth
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

#define REPR_SEEN_MAX 1024
THREADLOCAL VP REPR_SEEN[REPR_SEEN_MAX]={0};
char* repr0(VP x,char* s,size_t sz) {
	type_info_t t;
	int i;
	if(x==NULL) { APF(sz,"/*null*/",0); return s; }
	if(x->t < 0 || x->t > MAX_TYPE) { APF(sz,"/*unknown*/",0); return s; }
	if(!SIMPLE(x)) {
		for(i=0; i<REPR_SEEN_MAX; i++) {
			if(REPR_SEEN[i] == x) {
				APF(sz,"/*cycle*/",0); return s;
			}
		}
	}
	t=typeinfo(x->t);
	if(0 && DEBUG) {
		APF(sz," /*%p %s tag=%d#%s itemsz=%d n=%d rc=%d*/ ",x,t.name,
			x->tag,(x->tag!=0 ? sfromx(tagname(x->tag)) : ""),
			x->itemsz,x->n,x->rc);
	}

	IN_OUTPUT_HANDLER++;

	if(x->tag!=0) 
		APF(sz, "'%s(", tagnames(x->tag));
	if(t.repr) (*(t.repr)(x,s,sz));
	if(x->tag!=0)
		APF(sz, ")", 0);
	if(!SIMPLE(x)) {
		for(i=0; i<REPR_SEEN_MAX; i++) {
			if(REPR_SEEN[i] == NULL) {
				REPR_SEEN[i] = x; break;
			}
		}
	}

	IN_OUTPUT_HANDLER--;

	return s;
}
char* reprA(VP x) {
	memset(REPR_SEEN,0,REPR_SEEN_MAX*sizeof(VP));
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
char* repr_c(VP x,char* s,size_t sz) {
	int i=0,n=x->n,ch;
	APF(sz,"\"",0);
	for(;i<n;i++){
		ch = AS_c(x,i);
		if(ch=='"') APF(sz,"\\\"", 0);
		else if(ch=='\n') APF(sz,"\\n", 0);
		else if(ch=='\r') APF(sz,"\\r", 0);
		else APF(sz,"%c",ch);
		// repr0(*(EL(x,VP*,i)),s,sz);
	}
	APF(sz,"\"",0);
	return s;
}
char* repr_d(VP x,char* s,size_t sz) {
	int i, n;
	VP k=KEYS(x),v=VALS(x);
	// if(!sz) return s;
	if (!k || !v) { APF(sz,"[null]",0); return s; }
	APF(sz,"[",0);
	n=k->n;
	for(i=0;i<n;i++) {
		repr0(DICT_key_n(x,i), s, sz-1);
		APF(sz,":",0);
		repr0(DICT_val_n(x,i), s, sz-2);
		if(i!=n-1)
			APF(sz,", ",0);
	}
	APF(sz,"]",0);
	return s;
}
char* repr_a(VP x,char* s,size_t sz) { // table
	int i, j, kn, vn;
	VP k=KEYS(x), v=VALS(x), tmp, tmp2;
	// if(!sz) return s;
	if (!k || !v) { APF(sz,"[null]",0); return s; }
	APF(sz,"[",0);
	kn=k->n; vn=LEN(v) ? ELl(v,0)->n : 0;
	repr0(k, s, sz-1);
	APF(sz,"\n",0);
	for(i=0; i<vn; i++) {
		for(j=0; j<kn; j++) {
			// TODO kill me. need shortcut apply or value-at type!
			tmp=apply_simple_(ELl(v,j),i);
			repr0(tmp, s, sz-2);
			if(j!=kn-1) APF(sz,", ",0);
			xfree(tmp);
		}
		if(i!=vn-1) APF(sz,"\n",0);
	}
	APF(sz,"]",0);
	return s;
}
char* repr_l(VP x,char* s,size_t sz) {
	int i=0, n=x->n;VP a;
	APF(sz,"[",0);
	for(i=0;i<n;i++){
		a = ELl(x,i);
		if (a==NULL) APF(sz,"/*null*/",0); 
		else repr0(a,s,sz);
		if(i!=n-1)
			APF(sz,", ",0);
	}
	APF(sz,"]",0);
	return s;
}
char* repr_o(VP x,char* s,size_t sz) {
	int i=0,n=x->n;tag_t tag;
	if(n>1) APF(sz,"(",0);
	for(;i<n;i++){
		APF(sz,"%s",sfromx(numelem2base(x,i,10)));
		if(i!=n-1)
			APF(sz,",",0);
		// repr0(*(EL(x,VP*,i)),s,sz);
	}
	if(n>1) APF(sz,")",0);
	return s;
}
char* repr_p(VP x,char* s,size_t sz) {
	Proj p = EL(x,Proj,0);
	APF(sz,"'projection(%p,%d,",x,p.type);
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
char* repr_t(VP x,char* s,size_t sz) {
	int i=0,n=x->n;tag_t tag;
	if(n>1) APF(sz,"(",0);
	for(;i<n;i++){
		tag = AS_t(x,i);
		APF(sz,"'%s",tagnames(tag));
		if(i!=n-1)
			APF(sz,",",0);
		// repr0(*(EL(x,VP*,i)),s,sz);
	}
	if(n>1) APF(sz,")",0);
	return s;
}
char* repr_x(VP x,char* s,size_t sz) {
	int i;VP a;
	APF(sz,"'ctx#%p[\n",x);
	for(i=0;i<x->n;i++){
		a = ELl(x,i);
		if(!a) APF(sz,"/*null*/",0);
		else if(IS_d(a)) {
			APF(sz,"'scope",0);
			repr0(KEYS(a),s,sz);
		} else
			repr0(a,s,sz);
		if(i!=x->n-1)
			APF(sz,"\n",0);
		// repr0(*(EL(x,VP*,i)),s,sz);
	}
	APF(sz,"]",0);
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
	//PF("%d\n",sz);
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
					memset(a,0,sizeof(struct V)+a->sz);
					break;
				}
			}));
		});
	} 
	if(a==NULL)
		a = calloc(sizeof(struct V)+sz,1);
	if (MEM_WATCH) {
		WITHLOCK(mem, {
			MEMPF("%salloc %d %p %d (%d * %d) (total=%d, freed=%d, bal=%d)\n",(g==1?"GOBBLED! ":""),t,a,sizeof(struct V)+sz,finaln,itemsz,MEM_ALLOC_SZ,MEM_FREED_SZ,MEM_ALLOC_SZ-MEM_FREED_SZ);
			MEM_ALLOC_SZ += sizeof(struct V)+sz;
			MEM_ALLOCS++;
			for(i=0;i<N_MEM_PTRS;i++) {
				if (MEM_PTRS[i]==0)
					MEM_PTRS[i]=a;
			}
		});
	}
	a->t=t;a->tag=0;a->n=0;a->rc=1;a->cap=finaln;a->sz=sz;a->itemsz=itemsz;
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
		if(MEM_WATCH) {
			// MEMPF("realloc %d %p -> %d\n", x->t, x, newsz);
			MEM_ALLOC_SZ += newsz;
			MEM_REALLOCS++;
		}
		// PF("realloc new ptr = %p\n", newp);
		if(newp==NULL) PERR("realloc");
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
	if(UNLIKELY(x==NULL)) return x;
	//PF("xfree(%p)\n",x);DUMP(x);//DUMP(info(x));
	x->rc--; 
	if(LIKELY(x->rc==0)) {
		if(CONTAINER(x))
			ITERV(x,xfree(ELl(x,_i)));
		if(MEM_WATCH) {
			MEM_FREED_SZ+=sizeof(struct V) + x->sz;
			MEM_FREES+=1;
			MEMPF("free %d %p %d (%d * %d) (total=%d, freed=%d, bal=%d)\n",x->t,x,x->sz,x->itemsz,x->cap,MEM_ALLOC_SZ,MEM_FREED_SZ,MEM_ALLOC_SZ-MEM_FREED_SZ);
		}
		if (x->alloc == 0 && x->sz < RETAIN_MAX && N_RETAINS > 0) {
			for(i=0;i<N_RETAINS;i++)
				if(MEM_RETAIN[i]==x || MEM_RETAIN[i]==0) {
					MEM_RETAIN[i]=x;
					return x;
				}
		}
		//PF("xfree(%p) really dropping type=%d n=%d alloc=%d\n",x,x->t,x->n,x->alloc);
		//free(x);
		//if(x->alloc && x->dyn) free(x->dyn);
	} return x; }
VP xref(VP x) { if(MEM_WATCH){MEMPF("ref %p\n",x);} x->rc++; return x; }
VP xfroms(const char* str) {  
	// character vector from C string. allocates new VP. free it when done.
	// NB. returned VP does not contain the final \0 
	// (i.e., its result->n = strlen(str)-1)
	size_t len = strlen(str); type_info_t t = typechar('c');
	VP a = xalloc(t.t,len); memcpy(BUF(a),str,len); a->n=len; return a; }
const char* sfromx(VP x) {
	// NB. does NOT allocate - do not free returned value.
	if(x==NULL)return "null";
	return (char*)BUF(x); }
static inline int _equalm(const VP x,const int xi,const VP y,const int yi) {
	if(x==NULL||y==NULL) return 0;
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
	// this is the most common call in the code
	// TODO _equal() needs to handle comparison tolerance and type conversion
	// TODO _equal should use the new VARY_*() macros, except for general lists
	// PF("_equal\n"); DUMP(x); DUMP(y);
	// if the list is a container for one item, we probably want to match the inner one
	if(x==NULL||y==NULL) return 0;
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
VP equal(const VP x,const VP y) {
	return xb(_equal(x,y));
}
VP clone(const VP obj) { 
	// TODO keep a counter of clone events for performance reasons - these represent a concrete
	// loss over mutable systems
	// PF("clone\n");DUMP(obj);
	int i, objn=obj->n; VP res=ALLOC_LIKE(obj);
	if(CONTAINER(obj)) {
		res->n=objn;
		for(i=0;i<objn;i++) 
			EL(res,VP,i)=clone(ELl(obj,i));
	} else 
		res=appendbuf(res,BUF(obj),objn);
	// PF("clone returning\n");DUMP(res);
	return res;
}

// RUNTIME 

inline VP appendbuf(VP x,const buf_t buf,const size_t nelem) {
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
		if(k==NULL) {                      // create dict
			if(0 && SCALAR(y1)) 
				k=ALLOC_LIKE_SZ(y1, 4);
			else 
				k=xl0();
			v=xl0(); xref(k);xref(v); EL(x,VP,0)=k; EL(x,VP,1)=v;
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
		// PF("append %p to list %p\n", y, x); DUMP(x);
		x=xrealloc(x,x->n+1); xref(y);
		EL(x,VP,x->n)=y; x->n++;
		// PF("afterward:\n"); DUMP(x);
	} else {
		buf_t dest;
		// PF("append disaster\n");DUMP(info(x));DUMP(x);DUMP(info(y));DUMP(y);
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
VP amend_dict_dict(VP x,VP y) {
	// NB. amend supports callbacks as values but we
	// arent going to do that here - possibly needs rethink
	VP k=KEYS(y), v=VALS(y);
	int i=0, kn=k->n, vn=v->n;
	for(; i<kn; i++) x=assign(x,ELl(k,i),ELl(v,i));
	return x;
}
VP amend(VP x,VP y) {
	PF("amend\n");DUMP(x);DUMP(y);
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
			PF("amend calling val cb\n");DUMP(val);
			PFIN();
			tmp=apply(val,apply(x,idxv));
			PFOUT();
		} else {
			// handle the case of assigning one vector (ie a string) to a given index
			// or the case of assigning many indices to one value
			if (SCALAR(idx) || SCALAR(val)) tmp=xref(val); 
			else { PFIN(); tmp=apply(val,idxv); PFOUT(); }
		}
		RETURN_IF_EXC(tmp);
		if(IS_EXC(tmp)) { xfree(idxv); return tmp; }
		if(UNLIKELY(!CONTAINER(x) && tmp->t!=x->t)) return EXC(Tt(value),"amend value type does not match x",x,tmp);
		assign(x,idxv,tmp);
		xfree(idxv); xfree(tmp);
	}
	PF("amend returning\n");DUMP(x);
	return x;
}
inline VP assign(VP x,VP k,VP val) {
	// PF("assign\n");DUMP(x);DUMP(k);DUMP(val);
	if (LIST(k) && k->n) {
		int i=0;VP res=x;
		for(;i<k->n-1;i++) {
			PF("assign-at-depth %d\n",i);
			ARG_MUTATING(x);
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
		xref(k); xref(val); return append(x,xln(2,k,val));
	} else if(SIMPLE(x) && NUM(k)) {
		// PF("assign");DUMP(x);DUMP(k);DUMP(val);
		if(val->n == 0) return EXC(Tt(value),"assignment with empty values not supported on simple types",x,val);
		if(x->t != val->t) return EXC(Tt(type),"assign value and target types don't match",x,val);
		int typerr=-1, i=NUM_val(k);
		ARG_MUTATING(x);
		if (i>=x->n) { xrealloc(x,i+1); x->n=i+1; } 
		VARY_EACHRIGHT_NOFLOAT(x,k,({
			EL(x,typeof(_x),_y) = EL(val,typeof(_x),_y%val->n); // TODO assign should create new return value
		}),typerr);
		// PF("assign num returning");DUMP(x);
		return x;
	} else if(LIST(x) && NUM(k)) {
		int i=NUM_val(k);
		PF("%d\n",i);
		ARG_MUTATING(x);
		if (i>=x->n) { xrealloc(x,i+1); x->n=i+1; }
		if(i < x->n && ELl(x,i)) xfree(ELl(x,i));
		EL(x,VP,i)=val;
		DUMP(x);
		return x;
	}
	return EXC(Tt(type),"assign: bad types",x,0);
}
static inline VP assigns(VP x,const char* key,VP val) {
	return assign(x,xfroms(key),val);
}
VP behead(VP x) {
	// PF("behead\n");DUMP(x);
	return drop_(x,1);
}
VP call(VP x,VP y) {
	if(y==0) return EXC(Tt(value),"can't call null",x,y);
	return apply(y,x);
}
VP catenate_table(VP table, VP row) {
	PF("catenate_table\n"); DUMP(table); DUMP(row);
	int trows=TABLE_nrows(table), tcols=TABLE_ncols(table);
	if(DICT(row)) {
		VP rk=KEYS(row), rv=VALS(row), rktmp, rvtmp, vec; int i=0;
		ASSERT(LIST(rk)&&LIST(rv),"catenate_table: row keys or vals not list");
		for(; i<rk->n; i++) {
			rktmp=ELl(rk,i); rvtmp=ELl(rv,i);
			vec=DICT_find(KEYS(table),rktmp);  // does this key already exist in table?
			ARG_MUTATING(table);
			if(vec==NULL) {
				// NB. unknown columns are added, but old rows
				// will get the same value as this one!
				EL(table,VP,0)=append(ELl(table,0),rktmp);
				EL(table,VP,1)=append(ELl(table,1),take_(rvtmp,trows));
			} else vec=append(vec,rvtmp);
		}
		return table;
	} else if (LIST(row)) {
		int i=0;
		// this could either be a single row, i.e., a list comprised of simple types,
		// or a list of lists, meaning a list of individual lists of simple types. we'll
		// check if the first member is a list to determine which we think it is.
		if(LIST_of_lists(row) && LEN(LIST_first(row)) == tcols) {
			PF("LOL\n");DUMP(row);
			// TODO probably important not to recurse here
			for(; i<row->n; i++) table=catenate_table(table,ELl(row,i));
			return table;
		} else {
			VP colval;
			if(row->n != tcols) return EXC(Tt(value),"table definition doesn't match list",table,row);
			if(LEN(VALS(table))==0) {
				for(i=0; i<LEN(row); i++) {
					colval=ELl(row,i);
					// if any of the values of this dictionary are not scalar, we'll need
					// to make that column into a general list, or indexing row values will
					// become very confusing indeed
					if(!CONTAINER(colval) && !SCALAR(colval)) EL(row,VP,i)=xl(colval);
				}
				VALS(table) = row;
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
	PF("catenate\n");DUMP(x);DUMP(y);
	int n = x->n + y->n;
	if(!CONTAINER(x) && x->tag==0 && x->t==y->t) {
		PF("catenate2 - like simple objects\n");
		res=ALLOC_LIKE_SZ(x, n);
		appendbuf(res, BUF(x), x->n); appendbuf(res, BUF(y), y->n);
	} else {
		PF("catenate3 - container as x, or unlike objects\n");
		if(TABLE(x)) { return catenate_table(x,y); }
		if(DICT(x)) { return dict(x,y); }
		else if(LIST(x) && !LIST(y)) { ARG_MUTATING(x); res=append(x,y); }
		else {
			PF("catenate4 - create 2-item list with both items in it\n");
			res=xlsz(2);
			res=append(res,x);
			res=append(res,y);
		}
	}
	//res=list2vec(res);
	PF("catenate result");DUMP(res);
	return res;
}
VP curtail(VP x) {
	// PF("curtail\n");DUMP(x);
	return drop_(x,-1);
}
VP dict(VP x,VP y) {
	PF("dict\n");DUMP(x);DUMP(y);
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
		PF("dict already dict returning\n");DUMP(x);
		return x;
	} else {
		if(x->n > 1 && LIST_of_lists(y)) return make_table(x,y);
		if(x->n > 1 && x->n != y->n) return EXC(Tt(value),"can't create dict from unlike vectors",x,y);
		VP d=xd0();
		if(LIKELY(SCALAR(x))) {
			if(LIST(x))  // handle ['a:1] which becomes [['a]:1]
				d=assign(d,ELl(x,0),y);
			else
				d=assign(d,x,y);
		} else {
			int i;VP tmp1,tmp2;
			for(i=0;i<x->n;i++) {
				tmp1=apply_simple_(x, i);
				tmp2=apply_simple_(y, i);
				d=assign(d,tmp1,tmp2);
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
	if(!x || x->n==0) return x;
	if(i<0) { st = 0; end=x->n+i; }
	else { st = i; end=x->n; }
	// PF("drop_(,%d) %d %d %d\n", i, x->n, st, end); DUMP(x);
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
	// PF("drop args\n"); DUMP(x); DUMP(y);
	IF_RET(!NUM(y) || !SCALAR(y), EXC(Tt(type),"drop y arg must be single numeric",x,y));	
	VARY_EL(y, 0, ({ return drop_(x,_x); }), typerr);
	return res;
}
VP first(VP x) {
	VP i,r;
	if(CONTAINER(x)) return xref(ELl(x,0));
	else { i=xi(0); r=apply(x,i); xfree(i); return r; }
}
int _flat(VP x) { // returns 1 if vector, or a list composed of vectors (and not other lists)
	// PF("flat\n");DUMP(x);
	if(!CONTAINER(x)) return 1;
	else return 0; // lists are never flat
}
VP flatten(VP x) {
	int i,t=-1;VP item,res=NULL;
	PF("flatten\n");DUMP(x);
	if(!LIST(x))return x;
	if(x->n) {
		for(i=0;i<x->n;i++) {
			item=ELl(x,i);
			if(item==NULL) continue;
			if(LIST(item)) item=flatten(item);
			if(!res) {
				t=item->t; res=ALLOC_LIKE(item);
			} else if(item->t!=t) {
				PF("gave up on");DUMP(item);
				xfree(res); return x;
			}
			res=append(res,item);
		}
		PF("flatten returning\n");DUMP(res);
		return res;
	} else return xl0();
}
VP identity(VP x) {
	return x;
}
VP join(VP list,VP sep) {
	VP acc=NULL,x,t1; int i;
	if(list->n==0 || SCALAR(list)) return list;
	for(i=0;i<list->n-1;i++) {
		t1=xi(i); x=apply(list,t1);
		if(!acc) acc=ALLOC_LIKE(x);
		acc=catenate(acc,x);
		acc=catenate(acc,sep);
		xfree(t1);xfree(x);
	}
	t1=xi(i); x=apply(list,t1);
	acc=catenate(acc,x);
	xfree(t1); xfree(x);
	return acc;
}
VP last(VP x) {
	VP res=ALLOC_LIKE_SZ(x,1);
	if(x->n==0) return res;
	if(CONTAINER(x)) return xref(ELl(x,x->n-1));
	else res=appendbuf(res,ELi(x,x->n-1),1);
	return res;
}
static inline VP list(VP x) { // convert x to general list
	if(x==0) return xl0();
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
	ARG_MUTATING(x);
	for(i=0;i<n;i++) xfree(ELl(x,i));
	if(n>1) { memmove(ELi(x,1),ELi(x,n),x->itemsz*(x->n-n)); }
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
	// PF("shift_ %d\n",i);DUMP(x);
	int n=x->n;
	if(i<0) return catenate(take_(x,i%n),drop_(x,i%n));
	else return catenate(drop_(x,i%n),take_(x,i%n));
}
VP shift(VP x,VP y) {
	PF("shift\n");DUMP(x);DUMP(y);
	if(!SIMPLE(x)) return EXC(Tt(type),"shr x must be a simple type",x,y);
	if(!NUM(y)) return EXC(Tt(type),"shr y must be numeric",x,y);
	int typerr=-1;
	VARY_EL(y,0,({return shift_(x,_x);}),typerr);
	return NULL;
}
VP show(VP x) {
	if(x==NULL) { printf("null\n"); return x; }
	if(IS_c(x)) {
		const char* p=sfromx(x);
		printf("%s\n",p);
	} else {
		char* p = reprA(x);
		printf("%s\n",p);
		free(p);
	}
	return x;
}
VP splice(VP x,VP idx,VP replace) {
	int i, first=AS_i(idx,0), last=first+idx->n;
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
		return over(acc, x2(&catenate));
	}
	PF("splice returning\n"); DUMP(acc);
	return acc;
}
VP split(VP x,VP tok) {
	PF("split\n");DUMP(x);DUMP(tok);
	int typerr=-1;

	// special case for empty or null tok.. split vector into list
	if(tok->n==0) {
		VP tmp, tmp2;
		tmp=xl0();
		if(LIST(x)) return x;
		VARY_EACHLIST(x,({
			// PF("in split vary_each %c\n",_x);
			tmp2=ALLOC_LIKE_SZ(x, 1);
			tmp2=appendbuf(tmp2,(buf_t)&_x,1);
			tmp=append(tmp,tmp2);
		}),typerr);
		IF_RET(typerr>-1, EXC(Tt(type),"can't split that type", x, tok));
		PF("split returning\n");DUMP(tmp);
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
	}
	return EXC(Tt(nyi),"split with that type of data not yet implemented",x,tok);
}
VP take_(const VP x, const int i) {
	VP res;
	int st, end, xn=x->n;
	if(!x || x->n==0) return x;
	if(i<0) { st=ABS((xn+i)%xn); end=ABS(i)+st; } else { st=0; end=i; }
	res=ALLOC_LIKE_SZ(x,end-st);
	FOR(st,end,({ res=appendbuf(res,ELi(x,_i % xn),1); }));
	return res;
}
VP take(const VP x, const VP y) {
	int typerr=-1;
	size_t st, end; //TODO slice() support more than 32bit indices
	PF("take args\n"); DUMP(x); DUMP(y);
	IF_RET(!NUM(y) || !SCALAR(y), EXC(Tt(type),"take x arg must be single numeric",x,y));	
	VARY_EL(y, 0, ({ return take_(x,_x); }), typerr);
	return NULL;
}
int _findbuf(const VP x, const buf_t y) {   // returns index or -1 on not found
	// PF("findbuf\n");DUMP(x);
	if(LISTDICT(x)) { ITERV(x,{ 
		IF_RET(_findbuf(ELl(x,_i),y)!=-1,_i);
	}); } else {
		ITERV(x,{ IF_RET(memcmp(ELi(x,_i),y,x->itemsz)==0,_i); });
	}
	return -1;
}
inline int _find1(const VP x, const VP y) {        // returns index or -1 on not found
	// probably the most common, core call in the code. worth trying to optimize.
	// PF("_find1\n",x,y); DUMP(x); DUMP(y);
	ASSERT(x && (LISTDICT(x) || (x->t==y->t && y->n==1)), "_find1(): x must be list, or types must match with right scalar");
	VP scan;
	if(UNLIKELY(DICT(x))) scan=KEYS(x);
	else scan=x;
	if(LIST(scan)) {
		int i, sn=scan->n, yn=y->n, yt=y->t; VP item;
		for(i=0; i<sn; i++) {
			item=ELl(scan,i);
			if(item && ( (item->t == yt && item->n == yn) ||
									 LIST(item) && ELl(item,0)->t == yt ))
				if (_equal(item,y)==1) return i;
		}
		return -1;
	} else 
		ITERV(x,{ IF_RET(memcmp(ELi(x,_i),ELi(y,0),x->itemsz)==0,_i); });
	return -1;    // The code of this function reminds me of Armenia, or some war torn place
}
VP find1(VP x, VP y) {
	if(!x || !(LISTDICT(x) || (x->t==y->t && SCALAR(y))))
		return EXC(Tt(type),"find x must be list, or types must match with right scalar",x,y);
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
	// PF("condense\n");DUMP(x);
	VARY_EACH(x,({ if(_x) { j=_i; FOR(0,_x,appendbuf(acc,(buf_t)&j,1)); } }),typerr);
	// PF("condense returning\n");DUMP(acc);
	return acc;
}
VP table_row_list_(VP tbl, int row) { 
	int tc=TABLE_ncols(tbl), i;
	VP lst=xlsz(tc), res; 
	for(i=0; i<tc; i++) { 
		res=apply_simple_(TABLE_col(tbl,i), row); 
		lst=append(lst,DISCLOSE(res)); 
	}
	return lst;
}
VP table_row_dict_(VP tbl, int row) {
	return dict(clone(KEYS(tbl)), table_row_list_(tbl, row));
}
VP make_table(VP keys,VP vals) {
  VP res, newvals; int i;
	PF("make_table\n");DUMP(keys);DUMP(vals);
	// approach: create empty table, then apply all these row values to it.
	// catenate_table already handles lists-of-lists correctly.
  res=xasz(2);
	EL(res,VP,0)=clone(keys);
	EL(res,VP,1)=xlsz(LEN(ELl(vals,0)));
	res->n=2;
	return catenate_table(res, vals);
}
VP make(VP x, VP y) { 
	// TODO cast() should short cut matching kind casts 
	PF("make\n");DUMP(x);DUMP(y);
	VP res=0; type_t typenum=-1; tag_t typetag=-1;
	// right arg is tag naming a type, use that.. otherwise use y's type
	if(IS_t(y)) typetag=AS_t(y,0); else typenum=y->t;

	if(typetag==Ti(table)) {
		if(!DICT(x)) return EXC(Tt(type), "can only create table from dict", x, y);
		return make_table(KEYS(x),VALS(x));
	}

	#include"cast.h"
	// DUMPRAW(buf,BUFSZ);
	return res;
}
VP len(VP x) {
	int n = x->n;
	if(TABLE(x)) n=TABLE_nrows(x);
	if(DICT(x)) n=1;
	return xi(n);
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
	res=assign(res,Tt(len),len(x));
	res=assign(res,Tt(capacity),capacity(x));
	res=assign(res,Tt(itemsz),itemsz(x));
	res=assign(res,Tt(alloced),xi(x->alloc));
	res=assign(res,Tt(baseptr),xi((int)x));
	res=assign(res,Tt(memptr),xi((int)BUF(x)));
	return res;
}
VP type(VP x) {
	if(x==0 || x->t<0 || x->t>MAX_TYPE) return Tt(null);
	type_info_t t=typeinfo(x->t);
	return xt(_tagnums(t.name));
}
VP deal(VP range, VP amt) {
	PF("deal\n");DUMP(range);DUMP(amt);
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
				FOR(0, amt, ({ EL(acc, typeof(_x), _i)=rand()%_x; }));
				acc->n=amt;
			}), typerr);}), typerr);
		return acc;
	}
}

// APPLICATION, ITERATION AND ADVERBS

static inline VP applyexpr(VP parent, VP code, VP xarg, VP yarg) {
	// PF_LVL++;
	PF("applyexpr (code, xarg, yarg):\n");DUMP(code);DUMP(xarg);DUMP(yarg);
	// PF_LVL--;
	// if(!LIST(code))return EXC(Tt(code),"expr code not list",code,xarg);
	if(SIMPLE(code) || !LIST(code)) return code;
	char ch; int i; 
	VP left=xarg,item=0;
	tag_t tag, tcom=Ti(comment), texc=Ti(exception), texpr=Ti(expr), tlam=Ti(lambda), 
				tlistexpr=Ti(listexpr), tname=Ti(name), toper=Ti(oper),
				traw=Ti(raw), tstr=Ti(string), tws=Ti(ws);

	VP curframe,restore_left=0,stack=xl0();
	int stack_i=-1, start_i=0, use_existing_item=0, use_existing_left=0, return_expr_type=0, return_to=0;
	stack=append(stack,xln(9,parent,code,xarg,yarg,left,XI0,xi(-1),xi(0),left));

	#define MAYBE_RETURN(value) \
		if (return_to!=-1) { \
			PF("applyexpr returning to frame %d\n",return_to); \
			DUMP(value); \
			if(return_expr_type==1) { \
				PF("setting left in caller frame..\n"); \
				use_existing_left=1; \
				left=value; \
			} else if (return_expr_type==2) { \
				PF("setting item and left in caller frame..\n"); \
				DUMP(value); DUMP(restore_left); \
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
			PF("applyexpr actually returning\n"); \
			if(value==NULL) { PF("null\n"); } \
			else DUMP(value); \
			return value; \
		} 

	applyexprtop:

	if (stack_i==-1) {
		stack_i=stack->n-1;
		PF("applyexpr picking fresh stack_i=%d of %d\n", stack_i, stack->n-1);
	} else {
		PF("applyexpr returning to stack #%d, i%d.. left/item:\n", stack_i, start_i);
		DUMP(left);
		DUMP(item);
	}
	curframe=ELl(stack,stack_i);
	DUMP(curframe);

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
		PF("using existing left\n");DUMP(left);
		// if(left!=0 && UNLIKELY(IS_EXC(left))) { MAYBE_RETURN(left); }
		use_existing_left=0;
	}

	if(SIMPLE(code)) { MAYBE_RETURN(code); }

	if(LIST(code) && code->tag==tlam && code->n == 2) {
		PF("unpacking lambda\n");
		int arity=LAMBDAARITY(code);
		if(arity==1 && xarg==0) {
			item=proj(-1,parent,xarg,yarg); MAYBE_RETURN(item);
		} 
		if(arity==2 && (xarg==0||yarg==0)) {
			item=proj(-2,parent,xarg,yarg); MAYBE_RETURN(item);
		}
		code=ELl(code,0);
	}
	
	PF("applyexpr code body:\n");
	DUMP(code);

	for(i=start_i;i<code->n;i++) {
		PF("applyexpr #%d/%d\n",i,code->n-1);
		if (use_existing_item) {
			PF("using existing item\n"); DUMP(item);
			use_existing_item=0;
			if(item==0) continue;
			// if(UNLIKELY(IS_EXC(item))) { MAYBE_RETURN(item); }
			goto evalexpr;
		} else PF("picking up item from code %d\n", i); 
		item=ELl(code,i);
		PF("item=\n");DUMP(item);

		consideritem:

		tag=item->tag;
		// consider storing these skip conditions in an array
		if(tag==tws) continue;
		if(tag==tcom) continue;
		if(tag==tlam) { // create a context for lambdas
			VP newctx,this; int j;
			newctx=xx0();
			item=list(item);
			for(j=0;j<parent->n;j++) {
				this=ELl(parent,j);
				if(j!=parent->n-1 || !LIST(this)) append(newctx,clone(ELl(parent,j)));
			}
			append(newctx,item); // second item of lambda is arity; ignore for now
			item=newctx;
			PF("created new lambda context item=\n");DUMP(item);
		// } else if (tag==texpr || tag==tlistexpr) {
		} else if (LIST(item) && !(LEN(item) && IS_c(LIST_first(item)) && LEN(LIST_first(item))==0)) { // tag==texpr || tag==tlistexpr) {
			// we've reached a paren expr or list expr that we must resolve before considering
			// what to do with this term of the parse tree. many expressions are simple and
			// don't require complex evaluation.. but some do require recursion
			if (!SIMPLE(item)) {
				PF("applying subexpression\n");
				VP newframe = xln(9,parent,item,xarg,yarg,NULL,xi(i),xi(stack_i),xi(2),left);
				PF("trying stack hack for expression (expr/listexpr)\n");
				DUMP(newframe);
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
				if(IS_c(item))
					goto consideritem;
				else if(tag==tlistexpr)
					item=list(item);
			}
		} else if(LIKELY(IS_c(item)) && (tag==tname || tag==toper || tag==traw)) {
			ch = AS_c(item,0);
			// end of expr; cleanse expression
			if(ch==';') { left=NULL; continue; }
			PF("much ado about\n");DUMP(item);
			if(item->n==1 && ch=='x') {
				if (LIKELY(xarg!=0)) 
					item=xarg;
				else {
					MAYBE_RETURN(proj(_arity(parent)*-1, parent, xarg, yarg));
				}
			} else if(item->n==1 && ch=='y') {
				if (LIKELY(yarg!=0)) {
					PF("picking up y arg\n");DUMP(yarg);
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
				left=proj(2,&set,xln(2,parent,left),NULL);
				left->tag=Ti(setproj);
				xfree(left);
				PF("created set projection (as)\n");
				DUMP(left);
				continue;
			} else if(item->n==2 && ch=='i' && AS_c(item,1)=='s') {
				left=proj(2,&set2,xln(2,parent,left),NULL);
				left->tag=Ti(setproj);
				xfree(left);
				PF("created set projection (is)\n");
				DUMP(left);
				continue;
			} else if(item->n == 4 && ch == 'l' 
							&& AS_c(item,1)=='o' && AS_c(item,2)=='a' && AS_c(item,3)=='d') {
				PF("performing load");
				item=proj(2,&loadin,0,parent);
			} else if(item->n == 4 && ch == 's' 
							&& AS_c(item,1)=='e' && AS_c(item,2)=='l' && AS_c(item,3)=='f') {
				PF("using self");
				item=clone(parent);
			} else
				item=get(parent,item);
			PF("decoded string identifier\n");DUMP(item);
			// if(IS_EXC(item)) { MAYBE_RETURN(left!=NULL && CALLABLE(left)?left:item); }
			// NB. i dont remember why we did the callable check there :/ note to self dont be evil
			if(IS_EXC(item)) { MAYBE_RETURN(item); }
		} else if(tag==tname) {
			PF("non-string name encountered");
			VP getparent=parent;
			/*
			if(LIST(item) && LEN(item) > 1 && LIST_first(item)->tag==traw) {
				// since 'x' and 'y' arent actually assigned identifiers, we have to look through
				// the first items of this non-string name and replace 'x' and 'y'
				// probably . too, somehow.. ultimately, this should be solved in get, but get
				// would need some concept of x and y 
				VP firstpart = LIST_first(item);
				if(IS_c(firstpart) && LEN(firstpart)==1 && AS_c(firstpart,0)=='x') {
					getparent=xarg; item=behead(item);
				}
				if(IS_c(firstpart) && LEN(firstpart)==1 && AS_c(firstpart,0)=='y') {
					getparent=yarg; item=behead(item);
				}
			}
			*/
			item=get(getparent,item);
			RETURN_IF_EXC(item);
		} else if(tag==tstr) {
			// strip off the 'string tag right before we use this value. this is because the
			// 'string part is more of an indication of its role in parsing, but we dont want users'
			// code to see this tag applied - they didnt ask for it.
			item->tag=0;
		}
		
		evalexpr:

		PF("before evalexpr (left,item):\n");
		DUMP(left); DUMP(item);
		if(left!=NULL) PF("left arity=%d\n", _arity(left)); 

		if(left!=0 && (left->tag==Ti(proj) || (CALLABLE(left) && _arity(left)>0))) {
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
			PF("applying dangling left callable\n");
			DUMP(left);
			if(left->tag==Ti(proj)) left=apply2(ELl(left,1),ELl(left,0),item);
			else left=apply(left,item);
			if(left == 0 || left->tag==texc) { MAYBE_RETURN(left); }
			if(IS_p(left)) {
				p=AS_p(left,0);
				if(yarg && p.type==2 && (!p.left || !p.right)) {
					PF("applyexpr consuming y:\n");DUMP(yarg);
					left=apply(left,yarg);
				}
			}
		} else if (0 && !CALLABLE(item) && (left!=0 && IS_t(left))) {
			PF("applying tag\n");
			left=entag(item,left);
		} else if(!CALLABLE(item) && (left!=0 && !CALLABLE(left))) {
			PF("applyexpr adopting left =\n");DUMP(item);
			left=item;
		} else {
			if(left) {
				if (_arity(left)==2) {
					left=xln(2,left,item); left->tag=Ti(proj);
				} else {
					PF("applyexpr calling apply(item,left)\n");DUMP(item);DUMP(left);
					if(IS_x(item)) {
						VP newframe = xln(9,item,ELl(item,item->n-1),left,NULL,left,xi(i+1),xi(stack_i),XI1,left);
						PF("trying stack hack\n");
						DUMP(newframe);
						stack=append(stack,newframe);
						stack_i=-1;
						goto applyexprtop;
					} else {
						PFIN();
						left=apply(item,left);
						PFOUT();
					}
					// if(IS_EXC(left)) { MAYBE_RETURN(left); }
				}
				PF("applyexpr apply returned\n");DUMP(left);
			} else {
				PF("no left, so continuing with item..\n");
				left=item;
			}
		}

		// recall that [1,2,3] goes into the parse tree as 'listexpr([1, 2, 3]), which is fine,
		// but as we evaluate [1, 2, 3], we don't have a way of forcing 1 into being a list before
		// that , occurs.. so after the first item, we force it to a list if it isnt one. :)
		if(i==0 && code->tag==tlistexpr && !LIST(left)) {
			PF("forcing left as list due to listexpr\n");
			left=xl(left);
		}

		PF("bottom\n");
	}
	PF("applyexpr done\n");
	DUMP(left);
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
	PF("arity = %d\n",i);DUMP(x);
	VP res=xi(i);
	return res;
}
VP applyctx(VP ctx,const VP x,const VP y) {
	if(!IS_x(ctx)) return EXC(Tt(type),"context not a context",x,y);
	int a=_arity(ctx);
	if(a > 0) {
		if(a == 2 && (x==0 || y==0))
			return proj(a*-1, ctx, x, y);
		if(a == 1 && (x==0 && y==0))
			return proj(a*-1, ctx, x, y);
	}
	// structure of a context. like a general list conforming to:
	// first: root context
	// second: nested context
	// third: ...
	// last: list of projections or lambda
	int i,tlam=Ti(lambda);
	VP code,res=NULL;
	PF("applyctx\n");DUMP(ctx);DUMP(x);DUMP(y);
	code=ELl(ctx,ctx->n-1);
	if(LIST(code))
		res=applyexpr(ctx,code,x,y);
	PF("applyctx returning\n"); DUMP(res);
	return res;
	/*
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
		PF("callctx apply result was\n");DUMP(res);
		if(!LIST(res) || res->n > 0) {
			PF("callctx returning\n"); DUMP(res); 
			return res;
		}
	}
	*/

	return EXC(Tt(undef),"undefined in applyctx",x,y);
}
VP apply_simple_(VP x,int i) {   // a faster way to select just one item of x
	if(x==0 || x->tag==Ti(exception)) return x;
	// PF("apply_simple_ %d\n", i);
	if(!LIST(x) && !SIMPLE(x)) { VP ii=xi(i), res=apply(x,ii); xfree(ii); return res; };
	if(i > LEN(x)) return EXC(Tt(index),"index out of range",x,xi(i));
	// see "special case for generic lists" below
	if(LIST(x)) return ELl(x,i);
	else {
		VP res=xalloc(x->t,1);
		res=appendbuf(res,ELi(x,i),1);
		return res;
	}
}
VP apply_table(VP x, VP y) {
	int tc=TABLE_ncols(x);
	int i;
	PF("apply_table\n");DUMP(x);DUMP(y);
	if(x==NULL || y==NULL) return NULL;
	if(NUM(y)) {
		if(SCALAR(y)) {
			int yi=NUM_val(y);
			return table_row_dict_(x, yi);
			VP row=xlsz(tc);
			for(i=0; i<tc; i++) row=append(row,apply(TABLE_col(x,i),y));
			return dict(clone(KEYS(x)),row);
		} else {
			VP newtbl=xasz(2), newvals=xlsz(LEN(y)), vec; int j;
			EL(newtbl,VP,0)=clone(KEYS(x));
			for(i=0; i<tc; i++) {
				vec=TABLE_col(x,i);
				PF("apply_table doing\n");DUMP(vec);
				newvals=append(newvals,apply(vec,y));
			}
			EL(newtbl,VP,1)=newvals;
			newtbl->n=2;
			PF("apply_table returning\n");DUMP(newtbl);
			return newtbl;
		}
	} else {
		int yn=LEN(y);
		if(SCALAR(y)) {
			return DICT_find(x,y);
		} else {
			VP res=xlsz(yn);
			for(i=0; i<yn; i++) {
				VP key=apply_simple_(y, i);
				VP fnd=DICT_find(x,key);
				if(fnd) res=append(res,fnd);
				xfree(key);
			}
			VP row=xlsz(tc);
			for(i=0; i<tc; i++) row=append(row,apply(TABLE_col(x,i),y));
			return dict(clone(KEYS(x)),row);
		}
	}
}
VP apply(VP x,VP y) {
	// this function is everything.
	VP res=NULL;int i=0,typerr=-1;
	// PF("apply\n");DUMP(x);DUMP(y);
	if(x==0 || x->tag==Ti(exception))return x;
	if(IS_p(x)) { 
		// if its dyadic
		   // if we have one arg, add y, and call - return result
			 // if we have no args, set y as left, and return x
		// if its monadic
			// if we have one arg already, call x[left], and then apply result with y
		PF("apply proj\n");DUMP(x);DUMP(y);
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
			PF("proj2\n"); 
			PFIN();
			if(p->left && p->right) y=(*p->f2)(p->left,p->right);
			else if(y && p->left) y=(*p->f2)(p->left, y);
			else if(y && p->right) y=(*p->f2)(y,p->right);
			else if(y) y=proj(2,p->f2,y,0);
			else y=x;
			PFOUT(); PF("proj2 done\n");DUMP(y);
			return y;
		}
		return xp(*p);
	}
	if(IS_1(x)) {
		// PF("apply 1\n");DUMP(x);DUMP(y);
		unaryFunc* f; f=AS_1(x,0); return (*f)(y);
	}
	if(IS_2(x)) {
		res=proj(2,AS_2(x,0),y,0);
		// PF("apply f2 returning\n");DUMP(res);
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
		PF("indexing at depth\n");
		DUMP(x);
		DUMP(y);
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
			PF("searching for string\n");
			if ((idx=_find1(k,y))>-1)
				append(res,xi(idx));
		} else {
			ITERV(y,{ 
				int idx;
				// PF("searching %d\n",_i); DUMP(y); DUMP(k);
				if(LIST(y)) idx = _find1(k,ELl(y,_i));
				else idx = _findbuf(k,ELi(y,_i));
				if(idx>-1) {
					PF("found at idx %d\n", idx); 
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
			PF("apply with number %d, x is list\n", i);
			IF_RET(i>=x->n, ({ EXC(Tt(index),"index out of range",x,y);	}));
			VP tmp = ELl(x,i); xref(tmp); return tmp;
		*/
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
	// PF("deep\n");DUMP(info(obj));DUMP(obj);DUMP(f);
	VP acc,subobj;
	if(!CONTAINER(obj)) return each(obj,f);
	if(_flat(obj)) {
		// PF("deep flat\n");
		acc=apply(f,obj);
		if(obj->tag) acc->tag=obj->tag;
		return acc;
	}
	PFIN();
	FOR(0,obj->n,({
		// PF("deep %d\n", _i);
		subobj=ELl(obj,_i);
		if(LIST(subobj)) subobj=deepinplace(subobj,f);
		else subobj=apply(f,subobj);
		// append(acc,subobj);
		EL(obj,VP,_i) = subobj; // this may not be safe, but append() is overriden for dicts, so we cant simply append the list
	}));
	PFOUT();
	PF("deep returning\n");DUMP(obj);
	return obj;
}
VP deep(const VP obj,const VP f) {
	// TODO perhaps deep() should throw an error with non-list args - calls each() now
	int i;
	// PF("deep\n");DUMP(info(obj));DUMP(obj);DUMP(f);
	VP acc,subobj;
	if(!CONTAINER(obj)) return each(obj,f);
	if(_flat(obj)) {
		// PF("deep flat\n");
		acc=apply(f,obj);
		if(obj->tag) acc->tag=obj->tag;
		return acc;
	}
	acc=ALLOC_LIKE(obj);
	PFIN();
	FOR(0,obj->n,({
		// PF("deep %d\n", _i);
		subobj=ELl(obj,_i);
		if(LIST(subobj)) subobj=deep(subobj,f);
		else subobj=apply(f,subobj);
		// append(acc,subobj);
		EL(acc,VP,_i) = subobj; // this may not be safe, but append() is overriden for dicts, so we cant simply append the list
	}));
	acc->n=obj->n;
	PFOUT();
	// PF("deep returning\n");DUMP(acc);
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
	PF("eachtable\n"); DUMP(obj); DUMP(fun);
	int objnr=TABLE_nrows(obj);
	VP acc=xlsz(objnr), tmpdict=NULL, res, item, tmpi; int i; 
	for(i=0; i<objnr; i++) {
		tmpdict=table_row_dict_(obj, i);
		PF("calling eachtable fun\n"); DUMP(tmpdict);
		PFIN();
		res=apply(fun,tmpdict);
		PFOUT();
		PF("eachtable fun result\n"); DUMP(res);
		if(IS_EXC(res)) return res;
		acc=append(acc,res);
		xfree(res); 
		xfree(tmpdict); 
	}
	PF("eachtable returning\n"); DUMP(acc);
	return acc;
}
VP each(const VP obj,const VP fun) { 
	// each returns a list if the first returned value is the same as obj's type
	// and has one item
	VP tmp, res, acc=NULL; 
	if(DICT(obj)) return eachdict(obj,fun);	
	if(TABLE(obj)) return eachtable(obj,fun);
	int n=LEN(obj);
	if(n==0) return obj;
	// PF("each\n");DUMP(obj);DUMP(fun);
	FOR(0,n,({ 
		// PF("each #%d\n",n);
		tmp=apply_simple_(obj, _i); 
		if(IS_EXC(tmp)) { if(acc) xfree(acc); return tmp; }
		res=apply(fun,tmp); 
		if(res==0) continue; // no idea lol
		if(IS_EXC(res)) { if(acc) xfree(acc); return res; }
		// delay creating return type until we know what this func produces
		if (!acc) acc=xalloc(SCALAR(res) ? res->t : 0,obj->n); 
		else if (!LIST(acc) && res->t != acc->t) 
			acc = xl(acc);
		xfree(tmp);
		append(acc,res); }));
	// PF("each returning\n");DUMP(acc);
	return acc;
}
VP eachboth(const VP obj,const VP fun) { 
	if(!LIST(obj) || obj->n != 2) return EXC(Tt(type),"eachboth x must be [left,right]",obj,fun);
	VP tmpl, tmpr, res, acc=NULL, left=ELl(obj,0), right=ELl(obj,1); int n=left->n;
	if(right->n != n) return EXC(Tt(type),"eachboth x must be same-length items",obj,fun);
	FOR(0,n,({ 
		tmpl=apply(left, xi(_i)); tmpr=apply(right, xi(_i)); res=apply2(fun,tmpl,tmpr); 
		if(res==0) continue; // no idea lol
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
VP exhaust(const VP x,const VP y) {
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
VP over(const VP x,const VP y) {
	//PF("over\n");DUMP(x);DUMP(y);
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
VP scan(const VP x,const VP y) { // always returns a list
	// PF("scan\n");DUMP(x);DUMP(y);
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
VP wide(const VP obj,const VP f) {
	int i; VP acc;
	PF("wide\n");DUMP(obj);DUMP(f);

	if(!CONTAINER(obj)) return apply(f, obj);

	// PF("wide top level\n");DUMP(obj);
	acc=apply(f,obj);
	if(CONTAINER(acc)) {
		for(i=0;i<acc->n;i++) {
			//PF("wide #%d\n",i);PFIN();
			EL(acc,VP,i)=wide(AS_l(acc,i),f);
			//PFOUT();
		}
	}
	return acc;
}

// MATHY STUFF:

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
	// PF("and\n"); DUMP(x); DUMP(y); // TODO and() and friends should handle type conversion better
	IF_EXC(x->n > 1 && y->n > 1 && x->n != y->n, Tt(len), "and arguments should be same length", x, y);	
	if(SIMPLE(x) && SIMPLE(y)) acc=ALLOC_BEST(x,y);
	else acc=xlsz(x->n);
	// PF("and acc\n");DUMP(acc);
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
VP base(VP x,VP y) {
	// two different functions in one:
	// 1. convert string x to number, assuming it's a number formatted in base y.
	// 2. convert an XXL number x to a string in base y
	int b=NUM_val(y), xn=LEN(x), i;
	if(IS_c(x)) {
		VP xr=reverse(x); I128 acc;
		if(b==10 || b==16) return str2num(x);
		return EXC(Tt(nyi), "base not yet implemented", x, y);
	}
	if(NUM(x)) {
		VP res=xcsz(5); // ARB
		for(i=0; i<xn; i++) res=append(res,numelem2base(x, i, b));
		return res;
	}
}
static inline VP divv(VP x,VP y) { 
	int typerr=-1; VP acc=ALLOC_BEST(x,y);
	// PF("div");DUMP(x);DUMP(y);DUMP(acc);
	if(UNLIKELY(!SIMPLE(x))) return EXC(Tt(type),"div argument should be simple types",x,0);
	VARY_EACHBOTH(x,y,({
		if(_y==0) return EXC(Tt(divzero),"divide by zero in mod",x,y);
		if(LIKELY(x->t > y->t)) { _x=_y/_x; appendbuf(acc,(buf_t)&_x,1); }
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
	// PF("max\n");DUMP(x);
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
	PF("minus\n");DUMP(x);DUMP(y);
	IF_EXC(!SIMPLE(x) || !SIMPLE(y), Tt(type), "minus args should be simple types", x, y); 
	VP acc=ALLOC_BEST(x,y);
	VARY_EACHBOTH(x,y,({
		if(LIKELY(x->t > y->t)) { _x=_x-_y; appendbuf(acc,(buf_t)&_x,1); }
		else { _y=_x-_y; appendbuf(acc,(buf_t)&_y,1); }
		if(!SCALAR(x) && SCALAR(y)) _j=-1; // NB. AWFUL!
	}),typerr);
	IF_EXC(typerr > -1, Tt(type), "minus arg wrong type", x, y);
	// PF("minus result\n"); DUMP(acc);
	return acc;
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
			if(_y==0) return EXC(Tt(divzero),"divide by zero in mod",x,y);
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
VP orelse(VP x,VP y) {
	PF("orelse\n");DUMP(x);DUMP(y);
	if(!IS_EXC(x) && LEN(x)>0) return x;
	else if(NUM(x) && _any(x)) return x;
	else {
		if(CALLABLE(y)) return apply(y,x);
		else return y;
	}
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
	// NB. str2num creates int at the minimum
	double d; I128 buf=0; const char* s=sfromx(flatten(x));
	PF("str2num %s\n",s);DUMP(x);
	IF_EXC(!IS_c(x),Tt(type),"str2int arg should be char vector",x,0);
	if(strchr(s,'.')!=0 && (d=strtod(s,NULL))!=0) {
		return xf(d);
	} else if(LEN(x)>2 && s[0]=='0' && s[1]=='x') {
		if (sscanf(s,"%llx",&buf)==1) {
			if(buf<MAX_i)
				return xi((CTYPE_i)buf);
			if(buf<MAX_j)
				return xj((CTYPE_j)buf);
			return xo((CTYPE_o)buf);
		} else 
			return x;
	} else if (sscanf(s,"%lld",&buf)==1) { // should probably use atoi or strtol
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
	//PF("times\n");DUMP(x);DUMP(y);DUMP(info(acc));
	if(UNLIKELY(!SIMPLE(x))) return EXC(Tt(type),"times argument should be simple types",x,0);
	VARY_EACHBOTH(x,y,({
		PF("%d %d %d %d\n", _i, _j, _x, _y);
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
				return KEYS(ELl(x,i));
		return xd0();
	}
	if(SIMPLE(x)) return count(xi(x->n));
	return EXC(Tt(type),"key can't operate on that type",x,0);
}

VP val(VP x) {
	if(DICT(x)) return ELl(x,1);
	return EXC(Tt(type),"val can't operate on that type",x,0);
}

VP _getmodular(VP x,VP y) {
	ASSERT((DICT(x)||IS_x(x))&&LIST(y),"_getmodular");
	PF("_getmodular");DUMP(y);
	VP rootscope;
	if(DICT(x))
		rootscope=x;
	else {
	 	rootscope=KEYS(x);
		if(!DICT(rootscope)) return 0;
	}
	VP tag=ELl(y,0);
	VP res=apply(rootscope,tag); // need a fast-path dict lookup
	if(res==NULL) return 0;
	res=apply(res,Tt(get));
	if(res==NULL) return 0;
	return apply(res,ELl(y,1));
}

VP get(VP x,VP y) {
	// get is used to resolve names in applyctx(). x is context, y is thing to
	// look up. scans tree of scopes/closures to get value. 
	//
	// in k/q, get is
	// overriden to do special things with files and other handles as well.
	// 
	// in our case, if you pass in ['tag,arg] on the right, it will look up
	// 'tag in the root scope, and then try to call its "get" member. 
	// see _getmodular()
	int xn=x->n,yn=y->n,i,j;VP item,res;
	PF("get\n");DUMP(x);DUMP(y);
	if((DICT(x)||IS_x(x)) && LIST(y) && yn >= 2 && IS_t(ELl(y,0))) {
		res=_getmodular(x,y);
		if(res!=0) return res;
	}
	if(IS_x(x)) {
		if(LIKELY(IS_c(y) || (LIST(y) && IS_c(ELl(y,0))))) 
			y=str2tag(y);
		DUMP(y);
		if(IS_t(y) && AS_t(y,0)==Ti(.)) {
			//return clone(curtail(x)); // clone(KEYS(x));
			return curtail(x);
		} else if(IS_t(y) && yn > 1 && AS_t(y,0)==0) { // empty first element = start from root
			PF("get starting from root\n");
			i=0;y=list(behead(y));
			for(;i<xn;i++) {
				PF("get r#%d\n", i);
				if(LIST(ELl(x,i))) continue; // skip code bodies - maybe should use tags for this?
				res=apply(ELl(x,i),y);
				if(res!=NULL) return res;
			}
		} else {
			i=xn-1;
			if(SIMPLE(y) && yn>1) y=list(y); // support depth queries with scalars like thing.item
			for(;i>=0;i--) {
				PF("get t#%d\n", i);
				item=ELl(x,i);
				DUMP(item);
				if(SIMPLE(y) && DICT(item)) {
					if((res=DICT_find(item,y))) return res;
				} else {
					if(LIST(item)) continue; // skip code bodies - maybe should use tags for this?
					res=apply(item,y);
					if(res!=NULL) return res;
				}
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
	if(x==NULL || y==NULL) return x;
	if(LIST(x)) {
		if(!IS_x(AS_l(x,0))) return EXC(Tt(type),"set x must be (context,value)",x,y);
		if(x->n!=2) return EXC(Tt(type),"set x must be (context,value)",x,y);
		if(!IS_t(y)) return EXC(Tt(type),"set y must be symbol",x,y);
		ctx=AS_l(x,0);val=AS_l(x,1); i=ctx->n-1;
		if(val==NULL) return val;

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
					PF("set assigning in #%d\n",i);DUMP(dest);
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

VP set2(VP x,VP y) {                   // exactly like set, but arguments are [[ctx,name],[value]]
	PF("set2\n");
	DUMP(x);DUMP(y);
	VP acc=xln(2,ELl(x,0),y), res=set(acc,ELl(x,1)); xfree(acc); return res;
}

VP numelem2base(VP num,int i,int base) {
	// NB. this is called from repr_o. NO DUMP() ALLOWED!
	if(!NUM(num)) return EXC(Tt(type),"numelem2base can't operate on that type",num,0);
	if(base > 36) return EXC(Tt(value),"numelem2base can't produce that base",num,0);
	if(i > LEN(num)) return EXC(Tt(value),"numelem2base bad index",num,xi(i));
	if(IS_f(num)) {
		char buf[20];
		snprintf(buf,20,"%0.04f",num);
		return xfroms(buf);
	}
	int typerr=-1; I8 buf; I128 rem; 	
	VARY_EL_NOFLOAT(num,i,({ rem = _x; }),typerr);
	VP res=xcsz(5);
	char ch[]=CH_SET_na;
	do {
		// PF("%lld\n", rem);
		buf=ch[rem % base];
		appendbuf(res,&buf,1);
		rem = rem / base;
	} while (rem>0);
	return reverse(res);
}
VP str(VP x) {
	PF("str\n");DUMP(x);
	if(IS_c(x)) return x;
	if(IS_t(x)) return tagname(AS_t(x,0));
	if(NUM(x)) return numelem2base(x,0,10);
	return EXC(Tt(type),"str only works with simple types",x,NULL);
}
VP sys(VP x) {
	PF("sys\n");DUMP(x);
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
	if(type<0) p.ctx=func;
	else if(type==1) p.f1=func; 
	else p.f2=func;
	p.left=left; p.right=right;
	EL(pv,Proj,0)=p;
	pv->n=1;
	return pv;
}
VP xray(VP x) {
	PF("xray\n");DUMP(x);
	if(!_any(x)) {
		PF_LVL=0;
		return Tt(xrayoff);
	} else {
		PF_LVL=2;
		return Tt(xrayon);
	}
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
VP tagv(const char* name, VP x) {
	return entags(xln(1,x),name);
}
VP entag(VP x,VP t) {
	if(IS_c(t))
		x->tag=_tagnum(t);
	else if (IS_i(t))
		x->tag=AS_i(t,0);
	else if (IS_t(t))
		x->tag=AS_t(t,0);
	return x;
}
VP entags(VP x,const char* name) {
	x->tag=_tagnums(name);
	return x;
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
	//printf("_tagnums %s\n",name);
	//printf("tagnums free\n");
	// DUMP(TAGS);
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
	// PF("bracketjoin\n");DUMP(x);DUMP(y);
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
		// PF("%d %d %d\n", i, ctr, maxx);
		if(ctr==maxx) {
			appendbuf(res,(buf_t)&EL(x,CTYPE_b,i%x->n),1);
		} else
			appendbuf(res,(buf_t)&b0,1);
	}
	// PF("after scanning for maxx\n"); DUMP(res);
	VP c=ELl(partgroups(condense(res)),0);
	// PF("partgroups result\n"); DUMP(c);
	if(c->n) {
		c=append(c,plus(max(c),xi(1)));
		// PF("bracket append next\n"); DUMP(c);
	}
	res=assign(take(XI0,xi(y0->n)),c,xi(1));
	// PF("bracketjoin returning\n");DUMP(res);
	return res;
}
VP consecutivejoin(VP x, VP y) {
	// returns x[n] if y[0][n]==1 && y[1][n+1]==1 && .. else 0
	int j,n=y->n, typerr=-1, on=0; VP acc,tmp;
	// PF("consecutivejoin\n"); DUMP(x); DUMP(y);
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
	// PF("signaljoin\n");DUMP(x);DUMP(y);
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
	VP p1,p2,open,close,opens,closes,where,rep,out,base;

	renest:

	PF("NEST\n");DUMP(x);DUMP(y);
	//if(!LIST(x) || x->n < 2) return x;
	if(x->n<2)return x;
	p1=proj(2,&matcheasy,x,0);
	p2=proj(2,&matcheasy,x,0);
	open=apply(y,XI0); close=apply(y,XI1);
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
			if(open->tag) base=matchtag(x,xt(open->tag));
			else base=xb(1);
			opens=signaljoin(base,AS_l(opens,0));
			xfree(base);
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
		if(open->tag) base=matchtag(x,xt(open->tag));
		else base=xb(1);
		out=bracketjoin(xb(1), xln(2,consecutivejoin(base,opens),closes)); 
		xfree(base);
		DUMP(base);
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
		PF("nest x\n");
		out=splice(split(x,xi0()),where,rep);
		if(x->tag) out->tag=x->tag;
		PF("nest out\n");DUMP(out);
		if(!LIST(out)) out=xl(out);
		x=out;
		goto renest;
	} else { out = x; }
	PF("nest returning\n"); DUMP(out);
	return out;
}
VP matchany(VP obj,VP pat) {
	IF_EXC(!SIMPLE(obj) && !LIST(obj),Tt(type),"matchany only works with simple or list types in x",obj,pat);
	// IF_EXC(!SIMPLE(pat),Tt(type),"matchany only works with simple types in y",obj,pat);
	IF_EXC(SIMPLE(obj) && obj->t != pat->t, Tt(type),"matchany only works with matching simple types",obj,pat);
	int j,n=obj->n,typerr=-1;VP item, acc;
	// PF("matchany\n"); DUMP(obj); DUMP(pat);
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
	IF_EXC(!SIMPLE(obj) && !LIST(obj) && !TABLE(obj),Tt(type),
		"matcheasy only works with simple types, lists, and tables in x",obj,pat);
	int j,n=obj->n,typerr=-1;VP item, acc;
	PF("matcheasy\n"); DUMP(obj); DUMP(pat);
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
VP matchtag(const VP obj,const VP pat) {
	IF_EXC(!SIMPLE(obj) && !LISTDICT(obj),Tt(type),"matchtag only works with simple types or lists",obj,pat);
	IF_EXC(!IS_t(pat), Tt(type), "matchtag y must be tag",obj,pat);
	int j,n,typerr=-1;VP searchobj, item, acc;
	PF("matchtag\n"); DUMP(obj); DUMP(pat);

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
	return NULL;
}
VP list2vec(VP obj) {
	// Collapses lists that contain all the same kind of vector items into a
	// single vector [1,2,3i] = (1,2,3i) Will NOT collapse [1,(2,3),4] - use
	// flatten for this. (See note below for non-flat first items) The original
	// list will be returned when rejected for massaging.
	int i, t=0;
	VP acc,this;
	// PF("list2vec\n"); DUMP(obj);
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
	PF("list2vec result\n"); DUMP(acc); 
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
	PF("parsenum\n");DUMP(x);
	VP res=flatten(x);
	if(IS_c(res)) return str2num(res);
	else return res;
}
VP parselambda(VP x) {
	int i,arity=0,typerr=-1,tname=Ti(name),traw=Ti(raw); VP this;
	PF("parselambda\n");DUMP(x);
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
	// PF("PARSESTRLIT!!!\n");DUMP(x);
	if(LIST(x) && IS_c(AS_l(x,0)) && AS_c(AS_l(x,0),0)=='"') {
		VP res=xlsz(x->n), el, next; int ch,nextch,last;
		last=x->n-1;
		for(i=0;i<x->n;i++) {
			// PF("parsestrlit #%d/%d\n",i,last);
			el=AS_l(x,i);
			if(!el) continue;
			DUMP(el);
			if(IS_c(el)) {
				ch=AS_c(el,0);
				// PF("parselit ch=%c\n",ch);
				if ((i==0 || i==last) && ch=='"')
					continue; // skip start/end quotes
				if (i<last &&
				    (ch=AS_c(el,0))=='\\') {
					// PF("investigating %d\n",i+1);
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
			} else 
				res=append(res,el);
		}
		// due to the looping logic, we would wind up with an empty list - we want an empty list with an empty string! :)
		if(res->n==0) res=append(res,xc0()); 
		// PF("flattenin\n");DUMP(res);
		res=flatten(res);
		// DUMP(res);
		return res;
	} else {
		PF("parsestrlit not interested in\n");
		DUMP(x);
		return x;
	}
}
VP parseloopoper(VP x) {
	PF("parseloopoper\n");DUMP(x);
	VP st=xfroms(":|"), en=xfroms("#|:\\/<>',.");
	st->tag=Ti(raw); en->tag=Ti(raw);
	VP tmp1=matchany(x,st); 
	// PF("parseloopoper tmp1\n");DUMP(tmp1);
	if (!_any(tmp1)) { xfree(st); xfree(en); xfree(tmp1); return x; }
	VP tmp2=matchany(x,en);
	// PF("parseloopoper tmp2\n");DUMP(tmp2);
	if (!_any(tmp2)) { xfree(st); xfree(en); xfree(tmp1); xfree(tmp2); return x; }
	VP tmp3=xln(2,tmp2,tmp1);
	VP join=consecutivejoin(XB1,tmp3);
	if(_any(join)) {
		int diff=0, j=0; VP idx,rep,indices=partgroups(condense(join));
		for (; j<indices->n; j++) {  // probably needs an abstraction
			idx = plus(ELl(indices, j), xi(diff));
			idx = append(idx, plus(idx,XI1));
			rep = list2vec(apply(x, idx));
			rep->tag = Ti(oper);
			// PF("parsemulticharoper"); DUMP(rep);
			x=splice(x, idx, rep);
			diff += 1 - idx->n;
			xfree(idx);
			xfree(rep);
		}
		xfree(indices); 
	}
	xfree(st); xfree(en); xfree(tmp1); xfree(tmp2); xfree(tmp3); xfree(join);
	return x;
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
		proj(2,&nest,0,xln(5, entags(xfroms("\""),"raw"), xfroms("\""), xfroms("\\"), Tt(string), x1(&parsestrlit))),
		proj(2,&nest,0,xln(5, entags(xfroms("//"),"raw"), xfroms("\n"), xfroms(""), Tt(comment), x1(&parsecomment))),
		proj(2,&nest,0,xln(5, entags(xfroms("/*"),"raw"), xfroms("*/"), xfroms(""), Tt(comment), x1(&parsecomment)))
	);
	ctx=append(ctx,pats);
	acc=exhaust(acc,ctx);
	PF("parsestr after nest\n");
	xfree(pats);

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
	xfree(acc);
	//xfree(ctx);
	PF("matchexec results\n");DUMP(t1);


	// we only form expression trees after basic parsing - this saves us from
	// having to do a bunch of deep manipulations earlier in this routine (i.e.,
	// its faster to scan a flat list)
	ctx=mkbarectx();
	pats=xln(3,
		proj(2,&nest,0,xln(5, entags(xfroms("{"),"raw"), xfroms("}"), xfroms(""), Tt(lambda), x1(&parselambda))),
		proj(2,&nest,0,xln(5, entags(xfroms("("),"raw"), xfroms(")"), xfroms(""), Tt(expr), x1(&parseexpr))),
		proj(2,&nest,0,xln(5, entags(xfroms("["),"raw"), xfroms("]"), xfroms(""), Tt(listexpr), x1(&parseexpr)))
	);
	t2=t1;
	for(i=0;i<pats->n;i++) {
		PF("parsestr exhausting %d\n", i);
		//t2=exhaust(t2,proj(2,&wide,0,ELl(pats,i)));
		//t2=exhaust(t2,ELl(pats,i)); // wide doesnt seem needed here after tag fixes
		//t2=wide(t2,proj(2,&exhaust,0,ELl(pats,i)));
		t2=wide(t2,ELl(pats,i));
	}
	return t2;
}

VP parse(VP x) {
	return parsestr(sfromx(x));
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
void* thr_run0(void* VPctx) {
	#ifndef THREAD
	#else
	init_thread_locals();
	VP ctx=clone(VPctx);
	printf("thr_run %s\n", reprA(ctx));
	ctx=apply(ctx,0);
	printf("thr_run0 done\n"); DUMP(ctx);
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
	WITHLOCK(thr,if(pthread_create(&THR[NTHR++], &a, &thr_run0, ctx)!=0)PERR("pthread_create"));
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
VP evalinwith(VP tree,VP ctx,VP xarg) {
	if(!tree || !ctx) return NULL;
	if(IS_c(tree)) return evalstrinwith(sfromx(tree),ctx,xarg);
	if(!IS_x(ctx)) ctx=xxn(2,ctx,tree);  // try to make context ouf of dict (hopefully) and parse tree
	else append(ctx,tree);               // parse tree is last item of context (a list, basically)
	VP res=applyctx(ctx,xarg,NULL);
	ctx=curtail(ctx);
	return res;
}
VP evalin(VP tree,VP ctx) {
	return evalinwith(tree,ctx,NULL);
}
VP evalstrin(const char* str, VP ctx) {
	return evalstrinwith(str,ctx,NULL);
}
VP evalstrinwith(const char* str, VP ctx, VP xarg) {
	VP r;
	PF_LVL++;
	PF("evalstrinwith\n\"%s\"\n",str);DUMP(xarg);
	PF_LVL--;
	VP p=parsestr(str);
	r=evalinwith(p,ctx,xarg);
	PF("evalstrin returning\n");DUMP(r);
	return r;
}
VP evalfile(VP ctx,const char* fn) {
	return loadin(xfroms(fn), ctx);
	/*
	VP res,parse,acc = fileget(xfroms(fn));
	PF("evalfile executing\n"); DUMP(acc);
	parse=parsestr(sfromx(acc));
	append(ctx,parse);
	res=applyctx(ctx,NULL,NULL); // TODO define global constant XNULL=xl0(), XI1=xi(1), XI0=xi(0), etc..
	ctx=curtail(ctx);
	xfree(parse);
	return res;
	*/
}
VP loadin(VP fn,VP ctx) {
	char cwd[1024];
	VP subctx,res,parse,acc = fileget(fn);
	RETURN_IF_EXC(acc);
	subctx=clone(ctx);
	
	#ifdef STDLIBFILE
	VP tmp=xln(2,subctx,filedirname(fn));
	set(tmp,Tt(_dir)); // set() returns y value, not x context - dont preserve
	xfree(tmp);
	#endif

	parse=parsestr(sfromx(acc));
	append(subctx,parse);
	res=applyctx(subctx,NULL,NULL);
	xfree(subctx);
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
		printf("TESTS PASSED\n");
		if(MEM_WATCH) {
			PF("alloced = %llu, freed = %llu\n", MEM_ALLOC_SZ, MEM_FREED_SZ);
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
}
void init(){
	XB0=xb(0); XB1=xb(1);
	XI0=xi(0); XI1=xi(1);
	init_thread_locals();
	thr_start();
}
void args(VP ctx, int argc, char* argv[]) {
	if(argc > 1) {
		int i; VP a=xlsz(argc); 
		for(i=1; i<argc; i++) {
			printf("args %d\n%s\n", i, argv[i]);
			VP item=xfroms(argv[i]);
			if(AS_c(item,0)=='-') 
				show(evalin(show(catenate(xfroms("1 "),behead(item))),ctx));
			else if (access(argv[i],R_OK) != -1) { 
				show(evalfile(ctx,argv[i]));
			} else {
				VP xarg;
				#ifdef STDLIB_FILE
					xarg=fileget(xfroms("-"));
				#else
					xarg=0;
				#endif
				printf("evaluating as str:\n%s\n", argv[i]);
				show(evalstrinwith(argv[i],ctx,xarg));
				exit(1);
			}
			a=append(a,item);
		}
		set(xln(2,ctx,a),Tt(argv));
	} else selftest(NULL);
}
int main(int argc, char* argv[]) {
	init();
	VP ctx=mkworkspace();
	args(ctx,argc,argv);
	// net();
	repl(ctx);
	exit(1);
}
/*	
	TODO diff: (1,2,3,4)diff(1,2,55) = [['set,2,55],['del,3]] (plus an easy way to map that diff to funcs to perform)
	TODO mailboxes
	TODO some kind of backing store for contexts cant stand losing my work
	TODO decide operator for typeof
	TODO decide operator for tagof
	TODO decide operator for applytag
	TODO can we auto-parallelize some loops?
*/ 
