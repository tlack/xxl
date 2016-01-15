/* accessors for type #0 or l (list) implemented as VP */
#define T_l 0
#define IS_l(v) ( (v)->t==0 )
#define AS_l(v,n) ({ \
	ASSERT(IS_l(v)==1, "AS_l: type not l");\
	VP __x=EL(v,VP,n); __x; })
inline VP xl(VP x) { VP a; a=xalloc(0,1); EL(a,VP,0)=x; a->n=1; return a; }
inline VP xl0() { VP a=xalloc(0,1); return a; }
inline VP xlsz(sz) { VP a=xalloc(0,sz); return a; }
inline VP xla(VP a, VP x) { a=xrealloc(a,a->n++);EL(a,VP,a->n-1)=x; return a; }
inline VP xln(int nargs,...) { VP a; va_list args; int i; VP x; a=xalloc(0,nargs); a->n=nargs; va_start(args,nargs);\
	for(i=0;i<nargs;i++){ x=va_arg(args,VP); EL(a,VP,i)=x; }\
	return a; }
inline VP xlan(VP a, int nargs,...) { va_list args; int i; VP x; a=xrealloc(a,a->n+nargs); va_start(args,nargs);\
	for(i=0;i<nargs;i++){ x=va_arg(args,VP);
	EL(a,VP,(a->n)+i)=x; }\
	a->n+=nargs; return a; }
/* accessors for type #1 or t (tag) implemented as int */
#define T_t 1
#define IS_t(v) ( (v)->t==1 )
#define AS_t(v,n) ({ \
	ASSERT(IS_t(v)==1, "AS_t: type not t");\
	int __x=EL(v,int,n); __x; })
inline VP xt(int x) { VP a; a=xalloc(1,1); EL(a,int,0)=x; a->n=1; return a; }
inline VP xt0() { VP a=xalloc(1,1); return a; }
inline VP xtsz(sz) { VP a=xalloc(1,sz); return a; }
inline VP xta(VP a, int x) { a=xrealloc(a,a->n++);EL(a,int,a->n-1)=x; return a; }
inline VP xtn(int nargs,...) { VP a; va_list args; int i; int x; a=xalloc(1,nargs); a->n=nargs; va_start(args,nargs);\
	for(i=0;i<nargs;i++){ x=va_arg(args,int); EL(a,int,i)=x; }\
	return a; }
inline VP xtan(VP a, int nargs,...) { va_list args; int i; int x; a=xrealloc(a,a->n+nargs); va_start(args,nargs);\
	for(i=0;i<nargs;i++){ x=va_arg(args,int);
	EL(a,int,(a->n)+i)=x; }\
	a->n+=nargs; return a; }
/* accessors for type #2 or b (byte) implemented as int8_t */
#define T_b 2
#define IS_b(v) ( (v)->t==2 )
#define AS_b(v,n) ({ \
	ASSERT(IS_b(v)==1, "AS_b: type not b");\
	int8_t __x=EL(v,int8_t,n); __x; })
inline VP xb(int8_t x) { VP a; a=xalloc(2,1); EL(a,int8_t,0)=x; a->n=1; return a; }
inline VP xb0() { VP a=xalloc(2,1); return a; }
inline VP xbsz(sz) { VP a=xalloc(2,sz); return a; }
inline VP xba(VP a, int8_t x) { a=xrealloc(a,a->n++);EL(a,int8_t,a->n-1)=x; return a; }
inline VP xbn(int nargs,...) { VP a; va_list args; int i; int x; a=xalloc(2,nargs); a->n=nargs; va_start(args,nargs);\
	for(i=0;i<nargs;i++){ x=va_arg(args,int); EL(a,int8_t,i)=x; }\
	return a; }
inline VP xban(VP a, int nargs,...) { va_list args; int i; int8_t x; a=xrealloc(a,a->n+nargs); va_start(args,nargs);\
	for(i=0;i<nargs;i++){ x=va_arg(args,int);
	EL(a,int8_t,(a->n)+i)=x; }\
	a->n+=nargs; return a; }
/* accessors for type #3 or i (int) implemented as int */
#define T_i 3
#define IS_i(v) ( (v)->t==3 )
#define AS_i(v,n) ({ \
	ASSERT(IS_i(v)==1, "AS_i: type not i");\
	int __x=EL(v,int,n); __x; })
inline VP xi(int x) { VP a; a=xalloc(3,1); EL(a,int,0)=x; a->n=1; return a; }
inline VP xi0() { VP a=xalloc(3,1); return a; }
inline VP xisz(sz) { VP a=xalloc(3,sz); return a; }
inline VP xia(VP a, int x) { a=xrealloc(a,a->n++);EL(a,int,a->n-1)=x; return a; }
inline VP xin(int nargs,...) { VP a; va_list args; int i; int x; a=xalloc(3,nargs); a->n=nargs; va_start(args,nargs);\
	for(i=0;i<nargs;i++){ x=va_arg(args,int); EL(a,int,i)=x; }\
	return a; }
inline VP xian(VP a, int nargs,...) { va_list args; int i; int x; a=xrealloc(a,a->n+nargs); va_start(args,nargs);\
	for(i=0;i<nargs;i++){ x=va_arg(args,int);
	EL(a,int,(a->n)+i)=x; }\
	a->n+=nargs; return a; }
/* accessors for type #4 or j (long) implemented as __int64_t */
#define T_j 4
#define IS_j(v) ( (v)->t==4 )
#define AS_j(v,n) ({ \
	ASSERT(IS_j(v)==1, "AS_j: type not j");\
	__int64_t __x=EL(v,__int64_t,n); __x; })
inline VP xj(__int64_t x) { VP a; a=xalloc(4,1); EL(a,__int64_t,0)=x; a->n=1; return a; }
inline VP xj0() { VP a=xalloc(4,1); return a; }
inline VP xjsz(sz) { VP a=xalloc(4,sz); return a; }
inline VP xja(VP a, __int64_t x) { a=xrealloc(a,a->n++);EL(a,__int64_t,a->n-1)=x; return a; }
inline VP xjn(int nargs,...) { VP a; va_list args; int i; __int64_t x; a=xalloc(4,nargs); a->n=nargs; va_start(args,nargs);\
	for(i=0;i<nargs;i++){ x=va_arg(args,__int64_t); EL(a,__int64_t,i)=x; }\
	return a; }
inline VP xjan(VP a, int nargs,...) { va_list args; int i; __int64_t x; a=xrealloc(a,a->n+nargs); va_start(args,nargs);\
	for(i=0;i<nargs;i++){ x=va_arg(args,__int64_t);
	EL(a,__int64_t,(a->n)+i)=x; }\
	a->n+=nargs; return a; }
/* accessors for type #5 or o (octo) implemented as __int128_t */
#define T_o 5
#define IS_o(v) ( (v)->t==5 )
#define AS_o(v,n) ({ \
	ASSERT(IS_o(v)==1, "AS_o: type not o");\
	__int128_t __x=EL(v,__int128_t,n); __x; })
inline VP xo(__int128_t x) { VP a; a=xalloc(5,1); EL(a,__int128_t,0)=x; a->n=1; return a; }
inline VP xo0() { VP a=xalloc(5,1); return a; }
inline VP xosz(sz) { VP a=xalloc(5,sz); return a; }
inline VP xoa(VP a, __int128_t x) { a=xrealloc(a,a->n++);EL(a,__int128_t,a->n-1)=x; return a; }
inline VP xon(int nargs,...) { VP a; va_list args; int i; __int128_t x; a=xalloc(5,nargs); a->n=nargs; va_start(args,nargs);\
	for(i=0;i<nargs;i++){ x=va_arg(args,__int128_t); EL(a,__int128_t,i)=x; }\
	return a; }
inline VP xoan(VP a, int nargs,...) { va_list args; int i; __int128_t x; a=xrealloc(a,a->n+nargs); va_start(args,nargs);\
	for(i=0;i<nargs;i++){ x=va_arg(args,__int128_t);
	EL(a,__int128_t,(a->n)+i)=x; }\
	a->n+=nargs; return a; }
/* accessors for type #6 or c (char) implemented as char */
#define T_c 6
#define IS_c(v) ( (v)->t==6 )
#define AS_c(v,n) ({ \
	ASSERT(IS_c(v)==1, "AS_c: type not c");\
	char __x=EL(v,char,n); __x; })
inline VP xc(char x) { VP a; a=xalloc(6,1); EL(a,char,0)=x; a->n=1; return a; }
inline VP xc0() { VP a=xalloc(6,1); return a; }
inline VP xcsz(sz) { VP a=xalloc(6,sz); return a; }
inline VP xca(VP a, char x) { a=xrealloc(a,a->n++);EL(a,char,a->n-1)=x; return a; }
inline VP xcn(int nargs,...) { VP a; va_list args; int i; int x; a=xalloc(6,nargs); a->n=nargs; va_start(args,nargs);\
	for(i=0;i<nargs;i++){ x=va_arg(args,int); EL(a,char,i)=x; }\
	return a; }
inline VP xcan(VP a, int nargs,...) { va_list args; int i; char x; a=xrealloc(a,a->n+nargs); va_start(args,nargs);\
	for(i=0;i<nargs;i++){ x=va_arg(args,int);
	EL(a,char,(a->n)+i)=x; }\
	a->n+=nargs; return a; }
/* accessors for type #7 or d (dict) implemented as VP */
#define T_d 7
#define IS_d(v) ( (v)->t==7 )
#define AS_d(v,n) ({ \
	ASSERT(IS_d(v)==1, "AS_d: type not d");\
	VP __x=EL(v,VP,n); __x; })
inline VP xd(VP x) { VP a; a=xalloc(7,1); EL(a,VP,0)=x; a->n=1; return a; }
inline VP xd0() { VP a=xalloc(7,1); return a; }
inline VP xdsz(sz) { VP a=xalloc(7,sz); return a; }
inline VP xda(VP a, VP x) { a=xrealloc(a,a->n++);EL(a,VP,a->n-1)=x; return a; }
inline VP xdn(int nargs,...) { VP a; va_list args; int i; VP x; a=xalloc(7,nargs); a->n=nargs; va_start(args,nargs);\
	for(i=0;i<nargs;i++){ x=va_arg(args,VP); EL(a,VP,i)=x; }\
	return a; }
inline VP xdan(VP a, int nargs,...) { va_list args; int i; VP x; a=xrealloc(a,a->n+nargs); va_start(args,nargs);\
	for(i=0;i<nargs;i++){ x=va_arg(args,VP);
	EL(a,VP,(a->n)+i)=x; }\
	a->n+=nargs; return a; }
/* accessors for type #8 or 1 (f1) implemented as unaryFunc* */
#define T_1 8
#define IS_1(v) ( (v)->t==8 )
#define AS_1(v,n) ({ \
	ASSERT(IS_1(v)==1, "AS_1: type not 1");\
	unaryFunc* __x=EL(v,unaryFunc*,n); __x; })
inline VP x1(unaryFunc* x) { VP a; a=xalloc(8,1); EL(a,unaryFunc*,0)=x; a->n=1; return a; }
inline VP x10() { VP a=xalloc(8,1); return a; }
inline VP x1sz(sz) { VP a=xalloc(8,sz); return a; }
inline VP x1a(VP a, unaryFunc* x) { a=xrealloc(a,a->n++);EL(a,unaryFunc*,a->n-1)=x; return a; }
inline VP x1n(int nargs,...) { VP a; va_list args; int i; unaryFunc* x; a=xalloc(8,nargs); a->n=nargs; va_start(args,nargs);\
	for(i=0;i<nargs;i++){ x=va_arg(args,unaryFunc*); EL(a,unaryFunc*,i)=x; }\
	return a; }
inline VP x1an(VP a, int nargs,...) { va_list args; int i; unaryFunc* x; a=xrealloc(a,a->n+nargs); va_start(args,nargs);\
	for(i=0;i<nargs;i++){ x=va_arg(args,unaryFunc*);
	EL(a,unaryFunc*,(a->n)+i)=x; }\
	a->n+=nargs; return a; }
/* accessors for type #9 or 2 (f2) implemented as binaryFunc* */
#define T_2 9
#define IS_2(v) ( (v)->t==9 )
#define AS_2(v,n) ({ \
	ASSERT(IS_2(v)==1, "AS_2: type not 2");\
	binaryFunc* __x=EL(v,binaryFunc*,n); __x; })
inline VP x2(binaryFunc* x) { VP a; a=xalloc(9,1); EL(a,binaryFunc*,0)=x; a->n=1; return a; }
inline VP x20() { VP a=xalloc(9,1); return a; }
inline VP x2sz(sz) { VP a=xalloc(9,sz); return a; }
inline VP x2a(VP a, binaryFunc* x) { a=xrealloc(a,a->n++);EL(a,binaryFunc*,a->n-1)=x; return a; }
inline VP x2n(int nargs,...) { VP a; va_list args; int i; binaryFunc* x; a=xalloc(9,nargs); a->n=nargs; va_start(args,nargs);\
	for(i=0;i<nargs;i++){ x=va_arg(args,binaryFunc*); EL(a,binaryFunc*,i)=x; }\
	return a; }
inline VP x2an(VP a, int nargs,...) { va_list args; int i; binaryFunc* x; a=xrealloc(a,a->n+nargs); va_start(args,nargs);\
	for(i=0;i<nargs;i++){ x=va_arg(args,binaryFunc*);
	EL(a,binaryFunc*,(a->n)+i)=x; }\
	a->n+=nargs; return a; }
/* accessors for type #10 or p (proj) implemented as Proj */
#define T_p 10
#define IS_p(v) ( (v)->t==10 )
#define AS_p(v,n) ({ \
	ASSERT(IS_p(v)==1, "AS_p: type not p");\
	Proj __x=EL(v,Proj,n); __x; })
inline VP xp(Proj x) { VP a; a=xalloc(10,1); EL(a,Proj,0)=x; a->n=1; return a; }
inline VP xp0() { VP a=xalloc(10,1); return a; }
inline VP xpsz(sz) { VP a=xalloc(10,sz); return a; }
inline VP xpa(VP a, Proj x) { a=xrealloc(a,a->n++);EL(a,Proj,a->n-1)=x; return a; }
inline VP xpn(int nargs,...) { VP a; va_list args; int i; Proj x; a=xalloc(10,nargs); a->n=nargs; va_start(args,nargs);\
	for(i=0;i<nargs;i++){ x=va_arg(args,Proj); EL(a,Proj,i)=x; }\
	return a; }
inline VP xpan(VP a, int nargs,...) { va_list args; int i; Proj x; a=xrealloc(a,a->n+nargs); va_start(args,nargs);\
	for(i=0;i<nargs;i++){ x=va_arg(args,Proj);
	EL(a,Proj,(a->n)+i)=x; }\
	a->n+=nargs; return a; }
/* accessors for type #11 or x (ctx) implemented as VP */
#define T_x 11
#define IS_x(v) ( (v)->t==11 )
#define AS_x(v,n) ({ \
	ASSERT(IS_x(v)==1, "AS_x: type not x");\
	VP __x=EL(v,VP,n); __x; })
inline VP xx(VP x) { VP a; a=xalloc(11,1); EL(a,VP,0)=x; a->n=1; return a; }
inline VP xx0() { VP a=xalloc(11,1); return a; }
inline VP xxsz(sz) { VP a=xalloc(11,sz); return a; }
inline VP xxa(VP a, VP x) { a=xrealloc(a,a->n++);EL(a,VP,a->n-1)=x; return a; }
inline VP xxn(int nargs,...) { VP a; va_list args; int i; VP x; a=xalloc(11,nargs); a->n=nargs; va_start(args,nargs);\
	for(i=0;i<nargs;i++){ x=va_arg(args,VP); EL(a,VP,i)=x; }\
	return a; }
inline VP xxan(VP a, int nargs,...) { va_list args; int i; VP x; a=xrealloc(a,a->n+nargs); va_start(args,nargs);\
	for(i=0;i<nargs;i++){ x=va_arg(args,VP);
	EL(a,VP,(a->n)+i)=x; }\
	a->n+=nargs; return a; }
