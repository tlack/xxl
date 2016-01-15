#define TESTITERS 1

#ifndef DEBUG
#define DEBUG 0
#endif

#define APF(sz,fmt,...) snprintf(s+strlen(s),sz-strlen(s),fmt,__VA_ARGS__);
#define ASSERT(cond,txt) ({ if (!(cond)) { printf("ASSERT: %s\n", txt); raise(SIGABRT); exit(1); } })
#define FOR(st,en,stmt) ({ int _i;for(_i=(st);_i<(en);_i++)stmt; })
#define IFR(cond,thing) if((cond)) return thing
#define ITER(thing,n,body) ({ int _i;for(_i=0;_i<sizeof(thing);_i++) { typeof(*thing) _x; _x=thing[_i]; body; } })
#define ITERV(x,body) ({ int _i;for(_i=0;_i<x->n;_i++) { body; } })
#define ITER2(x,y,body) ({ \
	int _i; int _j; \
	if(x==NULL || y==NULL || x->n==0 || y->n==0) { \
	} else if(x->n > 1) { \
		for(_i=0;_i<x->n;_i++) { \
			if(y->n > 1) { \
				for (_j=0;_j<y->n;_j++) { \
					if(IS_i(x)&&IS_i(y)) { \
						int _x = EL(x,int,_i); \
						int _y = EL(y,int,_i); \
						PF("_x %d,_y %d\n", _x, _y); \
						body; \
					} \
				} \
			} \
		} \
	} \
})
#define P0(fmt,x) ({ typeof(x) xx=x; char* s=malloc(1024);snprintf(fmt,1024,xx); xx; })
#define PF(...) (DEBUG && PF_LVL && ({ FOR(0,PF_LVL,printf("  ")); printf(__VA_ARGS__);}))
#define PFIN() (DEBUG && PF_LVL++)
#define PFOUT() (DEBUG && PF_LVL--)
#define PFW(stmt) ({ PFIN(); stmt; PFOUT(); })
#define MEMPF(...) (DEBUG && MEM_W && PF(__VA_ARGS__))


#define BUF(v) ((buf_t)( (v)->alloc ? (v->dyn) : (buf_t)&((v)->st) ))
#define EL(v,type,n) (((type*)BUF(v))[n])
#define ELl(v,n) ((EL(v,VP,n)))
#define ELb(v,n) ELsz(v,1,n)
#define ELi(v,n) ((BUF(v))+((v->itemsz)*n))
#define ELsz(v,sz,n) ((BUF(v))+(sz*n))
#define EXC(type,lbl,x,y) return tagv("exception",xln(4,type,lbl,x,y));
#define SCALAR(v) ((v)->n==1)
#define NUM(v) (IS_b(v)||IS_i(v)||IS_l(v)||IS_o(v))
#define LIST(v) ((v)->t==0)
#define LISTDICT(v) ((v)->t==0||IS_d(v))
#define DICT(v) (IS_d(v))
#define ENLISTED(v) (LIST(v)&&(v)->n==1)
#define KEYS(v) (ELl(v,0))
#define VALS(v) (ELl(v,1))
#define Ti(n) (_tagnums(#n))
#define Tt(n) (xt(_tagnums(#n)))
#if DEBUG == 1
	#define DUMP(x) ({ char* s = reprA(x); PF("%s\n", s); free(s); })
#else
	#define DUMP(x) ({})
#endif
#ifdef DEBUG == 1
	#define DUMPRAW(x,sz) ({ printf("%p ",x); FOR(0,sz,printf("%d ",x[_i])); printf("\n"); })
#else
	#define DUMPRAW(x,sz) ({})
#endif

/* functions ending in A allocate their return value; be sure to free() them */
#define TYD(name,type) typedef type name
TYD(I8,unsigned char); TYD(I32,int); TYD(I64,__int64_t); TYD(I128,__int128_t);
TYD(type_t,I8); TYD(buf_t,I8*);

/* Structure for most values. 'st' and 'dyn' static and dynamic storage for data */
struct V { type_t t; I32 tag; I32 n; I32 cap; I32 itemsz; I32 sz; I32 rc; I8 alloc; buf_t next; union { I8 st[32]; buf_t dyn;};};
typedef struct V* VP; /* value pointer */

typedef VP (unaryFunc)(VP x);
typedef VP (binaryFunc)(VP x,VP y);
typedef char* (reprFunc)(VP x,char*s,size_t sz);

struct Proj0 { int type; union { unaryFunc* f1; binaryFunc* f2; }; VP left; VP right; };
typedef struct Proj0 Proj;

struct type_info { type_t t; char c; int sz; char name[32]; reprFunc* repr; };
typedef struct type_info type_info_t;
