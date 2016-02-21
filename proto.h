// prototypes

// from xxl.c
VP abss(VP x);
int _any(VP x);
VP amend(VP x,VP y);
VP and(VP x,VP y);
VP any(VP x);
VP apply(VP x,VP y);
VP apply_simple_(VP x,int i);          // faster way to index a simple type as new value
VP apply2(const VP f,const VP x,const VP y);
VP applyctx(VP ctx,VP x,VP y);
VP append(VP x,VP y);
VP appendbuf(VP x,buf_t buf,size_t nelem);
static inline int _arity(VP x);
static inline VP arity(VP x);
VP assign(VP x,VP k,VP val);
VP behead(VP x); // everything except the first element
VP cast(VP x,VP y);
VP capacity(VP x);
VP catenate(VP x,VP y);
VP clone(VP obj);
VP condense(VP x);
VP curtail(VP x); // everything except the last element
VP deal(VP x,VP y);
VP deep(VP obj,VP f);
VP dict(VP x,VP y);
static inline VP divv(VP x,VP y);
VP drop_(VP x,int i);
VP drop(VP x,VP y);
VP each(VP obj,VP fun);
VP eachboth(VP obj,VP fun);
VP eachleft(VP obj,VP fun);
VP eachright(VP obj,VP fun);
VP entag(VP x,VP t);
VP entags(VP x,const char* name);
static inline int _equalm(const VP x,const int xi,const VP y,const int yi);
int _equal(const VP x,const VP y);
VP evalstrin(const char* str,VP ctx);
VP evalin(VP str,VP ctx);
int _find1(VP x,VP y);
int _findbuf(const VP x,const buf_t y); // returns index or -1 on not found
VP first(VP x);
VP flatten(VP x);
VP get(VP x,VP y);
VP greater(VP x,VP y); 
VP info(VP x);
VP iftrue(VP x,VP y);
VP ifelse(VP x,VP y);
VP itemsz(VP x);
VP join(VP list,VP sep);
VP last(VP x);
VP len(VP x); 
VP lesser(VP x,VP y); 
VP list2vec(VP obj);
VP loadin(VP fn,VP ctx);
VP match(VP obj,VP pat);
VP matcheasy(VP obj,VP pat);
VP matchtag(VP obj,VP pat);
int matchpass(VP obj,VP pat);
VP proj(int type, void* func, VP left, VP right);
VP make_table(VP keys,VP vals);
VP mkworkspace();
VP minus(VP x,VP y);
VP mod(VP x,VP y);
VP not(VP x);
VP over(VP x,VP y);
VP or(VP x,VP y);
VP parse(VP x);
VP parseloopoper(VP x);
VP parsestr(const char* str);
VP plus(VP x,VP y);
void repl(VP ctx);
VP repr(VP x);
char* repr0(VP x,char* s,size_t len);
char* reprA(VP x);
VP reverse(VP x);
VP rootctx();
VP selftest(VP x);
VP set(VP x,VP y);                     // used for 'as'
VP set2(VP x,VP y);                    // same as set, but input arg order switched (for 'is')
const char* sfromx(VP x);
VP shift_(VP x,int i);
VP shift(VP x,VP y);
VP show(VP x);
VP split(VP x,VP tok);
VP str(VP x);
static inline VP str2num(VP x);
static inline VP str2tag(VP str); // turns string, or list of strings, into tag vector
VP sum(VP x);
VP sums(VP x);
VP sys(VP x);
VP table_row_dict_(VP tbl, int row);
VP table_row_list_(VP tbl, int row);
static inline VP tagname(tag_t tag);
static inline const char* tagnames(const tag_t tag);
/*static inline */
VP tagv(const char* name, VP x);
VP take_(VP x,int i);
VP take(VP x,VP y);
static inline tag_t _tagnum(VP name);
/*static inline */
tag_t _tagnums(const char* name);
void thr_run(VP ctx);
static type_info_t typeinfo(type_t n); 
static type_info_t typechar(char c);
VP type(VP x);
VP wide(VP obj,VP f);
VP xalloc(type_t t,I32 initn);
VP xfree(VP x);
VP xfroms(const char* str);
VP xor(VP x,VP y);
VP xrealloc(VP x,I32 newn);
VP xray(VP x);
VP xref(VP x);

#ifdef STDLIBNET
// from net.c
VP netbind(VP opts,VP callback);
VP netloop(VP xsock,VP cb);
#endif

#ifdef STDLIBFILE
// stdlib
VP filebasename(VP fn);
VP filedirname(VP fn);
VP filecwd(VP dummy);
VP fileget(VP fn);
VP filepath(VP pathlist);
VP fileset(VP str,VP fn);
#endif

#ifdef STDLIBSHAREDLIB
VP sharedlibget(VP fn);
VP sharedlibset(VP fn,VP funs);
#endif

#ifdef STDLIBSHELL
VP shellget(VP cmd);
#endif
