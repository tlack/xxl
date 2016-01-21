#define TESTITERS 1

#ifndef DEBUG
#define DEBUG 0
#endif

/* Control structures of sorts: */
#define FOR(st,en,stmt) ({ int _i;for(_i=(st);_i<(en);_i++)stmt; })
#define IF_RET(cond,thing) if((cond)) return thing
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
/* Debugging: */
#define APF(sz,fmt,...) ({ snprintf(s+strlen(s),sz-strlen(s),fmt,__VA_ARGS__); s; })
#define ASSERT(cond,txt) ({ if (!(cond)) { printf("ASSERT: %s\n", txt); raise(SIGABRT); exit(1); } })
#define P0(fmt,x) ({ typeof(x) xx=x; char* s=malloc(1024);snprintf(fmt,1024,xx); xx; })
#define PF(...) (DEBUG && PF_LVL && ({ FOR(0,PF_LVL,printf("  ")); printf(__VA_ARGS__);}))
#define PFIN() (DEBUG && PF_LVL++)
#define PFOUT() (DEBUG && PF_LVL--)
#define PFW(stmt) ({ printf("PFW\n"); PFIN(); stmt; PFOUT(); })
#define MEMPF(...) (DEBUG && MEM_W && PF(__VA_ARGS__))
#if DEBUG 
	#define DUMP(x) ({ char* s = reprA(x); PF("%s", s); free(s); })
	#define DUMPRAW(x,sz) ({ printf("%p ",x); FOR(0,sz,printf("%d ",x[_i])); printf("\n"); })
#else
	#define DUMP(x) ({})
	#define DUMPRAW(x,sz) ({})
#endif

/* Element access and type checks*/
#define BUF(v) ((buf_t)( (v)->alloc ? (v->dyn) : (buf_t)&((v)->st) )) // data ptr
#define EL(v,type,n) (((type*)BUF(v))[n])                             // ..as type, for index n
#define ELl(v,n) ((EL(v,VP,n)))                                       // ..deref linked list item
#define ELb(v,n) ELsz(v,1,n)                                          // ..as a byte* for index n
#define ELi(v,n) ((BUF(v))+((v->itemsz)*n))                           // ..for index n (no type assumed)
#define ELsz(v,sz,n) ((BUF(v))+(sz*n))                                // ..for index n, when casted to size = sz

// Handy functions for manipulating values and their types
#define SCALAR(v) ((v)->n==1)                            // is v a single value?
#define NUM(v) (IS_b(v)||IS_i(v)||IS_l(v)||IS_o(v))      // is v an int type?
#define LIST(v) ((v)->t==0)                              // is v a general list type?
#define DICT(v) (IS_d(v))                                // is v a dictionary?
#define LISTDICT(v) ((v)->t==0||IS_d(v))                 // is v a list or dictionary?
#define ENLISTED(v) (LIST(v)&&SCALAR(v))                 // is v a single item inside a list?
#define KEYS(v) (ELl(v,0))                               // keys for dict v
#define VALS(v) (ELl(v,1))                               // values for dict v
#define Ti(n) (_tagnums(#n))                             // int value for tag n (literal not string)
#define Tt(n) (xt(_tagnums(#n)))                         // tag n (literal not string) as a scalar of type tag
#define XALLOC_AS(x) ({ VP new_ = xalloc(x->t,x->n); if(x->tag)new_->tag=x->tag; new_; })
#define XALLOC_AS_EMPTY(x) ({ VP new_ = xalloc(x->t,1); if(x->tag)new_->tag=x->tag; new_; })

// create an exception value
#define EXC(type,lbl,x,y) tagv("exception",xln(4,type,xfroms(lbl),x,y));
#define IF_EXC(cond,type,msg,x,y) if((cond)) return EXC(type,msg,x,y)

// misc
#define LIKELY(x)       __builtin_expect((x),1)
#define UNLIKELY(x)     __builtin_expect((x),0)
#define TIME(n,expr) ({ int i; clock_t st,en; \
	st=clock(); for(i=0;i<n;i++) { expr; } \
	en=clock(); printf("%0.2f", ((double)(en-st)) / CLOCKS_PER_SEC); })


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
