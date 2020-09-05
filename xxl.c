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
	CLASS_dispatch(NULL, repr, x, NULL);
	char* s = reprA(x);
	return xfroms(s);
}
char* repr_c(VP x,char* s,size_t sz) {
	int i=0,xn=x->n,ch,skipn=0,skipstart=-1,skipend=-1;
	if(REPR_MAX_ITEMS && xn > REPR_MAX_ITEMS) {
		skipn=xn-REPR_MAX_ITEMS; skipstart=(xn-skipn)/2; skipend=(xn+skipn)/2;
	}
	FMT_into_s(sz,"\"",0);
	for(;i<xn;i++){
		if(skipn && i==skipstart) {
			FMT_into_s(sz,".. (%d omitted) ..",skipn);
			i=skipend; continue;
		}
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
	int i=0, xn=LEN(x); VP a;
	int skipstart=-1, skipend=-1, skipn=0;
	if(REPR_MAX_ITEMS && xn > REPR_MAX_ITEMS) {
		skipn=xn-REPR_MAX_ITEMS; skipstart=(xn-skipn)/2; skipend=(xn+skipn)/2;
	}
	FMT_into_s(sz,"[",0);
	for(i=0;i<xn;i++){
		if(skipn && i==skipstart) {
			FMT_into_s(sz,".. (%d omitted) ..", skipn);
			i=skipend; continue;
		}
		a = ELl(x,i);
		if (a==NULL) FMT_into_s(sz,"null",0);  
		else repr0(a,s,sz);
		if(i!=xn-1) FMT_into_s(sz,", ",0);
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
	if (vals && LIST(vals)) {
		k=vals; kn=LEN(k);
		for(i=0; i<kn; i++) {
			item=ELl(k,i);
			if(item==NULL) continue;
			if(LIST(item) && TAGGED(item,Ti(lambda))) 
				s=repr_xlambda(NULL,item,s,sz);
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

type_info_t typeinfo(const type_t n) { 
	if(n <= MAX_TYPE) return TYPES[n];
	else return (type_info_t){0}; 
}
type_info_t typechar(const char c) { 
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
VP xfillrange(VP x,int from,int to,int byteval) {
	XRAY_log("xfillrange %d .. %d = %d\n", from, to, byteval);
	ARG_MUTATING(x);
	x=XREALLOC(x,to);
	memset(BUF(x)+from, byteval, to-from+1);
	x->n=MAX(x->n,to);
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
int _equalm(const VP x,const int xi,const VP y,const int yi) {
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
		if(LEN(x)==3 && IS_c(ELl(x,1)) && AS_c(ELl(x,1),0)==':') {
			XRAY_log("creating new empty dict\n");
			return dict(xl0(),xl0()); // empty
		}
		VP res=xlsz(LEN(x)+1);
		res=append(res,xl0());
		if(LEN(x)>2) {
			res=append(res,entags(xc(','),"raw"));
			for(i=1;i<LEN(x)-1;i++) 
				res=append(res,ELl(x,i));
		}
		return entags(res,"listexpr");
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
			x=xsplice(x, idx, rep);
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
		tree=nest(tree,xln(5, entags(bracket,"raw"), entags(xfroms("]"),"raw"), xfroms(""), NULL, x1(&parseexpr)));
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
	#include "tests/test-basics.h"
}
void test_ctx() {
	VP ctx,tmp1,tmp2; printf("TEST_CTX\n");
	#include "tests/test-ctx.h"	
}
void test_eval() {
	#include"tests/test-eval.h"
}
void test_logic() {
	VP a,b,c;
	printf("TEST_LOGIC\n");
	#include"tests/test-logic.h"
}
void test_nest() {
	VP a,b,c;
	printf("TEST_NEST\n");
	#include"tests/test-nest.h"
	xfree(a);xfree(b);xfree(c);
}
void test_semantics() {
	printf("TEST_SEMANTICS\n");
	#include"tests/test-semantics.h"
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
	VP res=applyctx(ctx,NULL,NULL);
	xfree(parsetree);
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
	return NULL;
}
void init_thread_locals() {
	XXL_SYS=xd0();
	XXL_SYS=assign(XXL_SYS,Tt(ver),xfroms(XXL_VER));
	XXL_SYS=assign(XXL_SYS,Tt(src),xfroms(XXL_SRC));
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
