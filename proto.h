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
VP aside(VP x,VP y); 
VP assign(VP x,VP k,VP val);
VP base(VP x,VP y);
VP behead(VP x); // everything except the first element
const char* bfromx(VP x);
VP casee(VP x,VP y);
VP cast(VP x,VP y);
VP capacity(VP x);
VP catenate(VP x,VP y);
VP catenate_table(VP table, VP row);
VP clone(VP obj);
VP condense(VP x);
VP curtail(VP x); // everything except the last element
VP deal(VP x,VP y);
VP deep(VP obj,VP f);
VP del(VP x,VP y);
VP dict(VP x,VP y);
static inline VP divv(VP x,VP y);
VP drop_(VP x,int i);
VP drop(VP x,VP y);
VP each(VP obj,VP fun);
VP eachboth(VP obj,VP fun);
VP eachleft(VP obj,VP fun);
VP eachright(VP obj,VP fun);
VP enlist(VP x);
VP entag(VP x,VP t);
VP entags(VP x,const char* name);
static inline int _equalm(const VP x,const int xi,const VP y,const int yi);
int _equal(const VP x,const VP y);
VP evalin(VP str,VP ctx);
VP evalinwith(VP tree,VP ctx,VP xarg);
VP evalstrin(const char* str,VP ctx);
VP evalstrinwith(const char* str, VP ctx, VP xarg);
VP except(VP x,VP y);
VP extract(VP data,VP parts);
VP extractas(VP data,VP parts);
int _find1(VP x,VP y);
int _findbuf(const VP x,const buf_t y); // returns index or -1 on not found
VP first(VP x);
VP flatten(VP x);
VP resolvekey(VP dict,VP key,int checkparents);
VP lookup(VP x,VP y);
VP get(VP x);
VP greater(VP x,VP y); 
VP identity(VP x);
VP info(VP x);
VP ifelse(VP x,VP y);
VP iftrue(VP x,VP y);
void init_thread_locals();
VP itemsz(VP x);
VP join(VP list,VP sep);
VP last(VP x);
int _len(VP x);
VP len(VP x); 
VP lesser(VP x,VP y); 
VP list2vec(VP obj);
VP loadin(VP fn,VP ctx);
VP key(const VP x);
VP match(VP obj,VP pat);
VP matchany(VP obj,VP pat);
VP matcheasy(VP obj,VP pat);
VP matchtag(VP obj,VP pat);
int matchpass(VP obj,VP pat);
VP proj(int type, void* func, VP left, VP right);
VP make(VP x,VP y);
VP make_table(VP keys,VP vals);
VP mkworkspace();
VP minus(VP x,VP y);
VP mod(VP x,VP y);
VP neg(VP x);
VP not(VP x);
VP numelem2base(VP num,int i,int base);
VP over(VP x,VP y);
VP or(VP x,VP y);
VP parse(VP x);
VP parseloopoper(VP x);
VP parsestr(const char* str);
VP pin(VP x,VP y);
VP plus(VP x,VP y);
VP range_(int start, int end);
VP range(VP x,VP y);
VP ravel(VP x,VP y);
VP recurse(VP x,VP y);
VP resolve(VP ctx,VP ptree);
void repl(VP ctx);
VP repr(VP x);
char* repr0(VP x,char* s,size_t len);
char* reprA(VP x);
VP reverse(VP x);
VP rootctx();
VP selftest(VP x);
VP set(VP ctx,VP k,VP v);
VP set_as(VP x,VP y);                  // as
VP set_is(VP x,VP y);                  // is
char* sfromxA(VP x);
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
VP tag(VP x);
VP tagv(const char* name, VP x);
VP take_(VP x,int i);
VP take(VP x,VP y);
static inline tag_t _tagnum(VP name);
/*static inline */
tag_t _tagnums(const char* name);
static type_info_t typeinfo(type_t n); 
static type_info_t typechar(char c);
VP type(VP x);
VP unionn(VP x,VP y); // available via or()
VP wide(VP obj,VP f);
VP xalloc(type_t t,I32 initn);
VP xfree(VP x);
VP xfroms(const char* str);
VP xor(VP x,VP y);
VP xrealloc(VP x,I32 newn);
VP xray(VP x);
VP xref(VP x);

#ifdef THREAD
void thr_run(VP ctx);
void thr_run1(VP ctx,VP arg);
#endif

#ifdef STDLIBNET
// from net.c
VP netcall(VP data,VP addr);
VP netbind(VP opts,VP callback);
VP netloop(VP xsock,VP cb);
#endif

#ifdef STDLIBMBOX
// from net.c
VP mboxnew(VP x);
VP mboxsend(VP mbox,VP msg);
VP mboxpeek(VP mbox);
VP mboxrecv(VP mbox);
VP mboxquery(VP mbox,VP msg);
VP mboxwait(VP mbox); // equivalent to recv, but will block until something comes in
VP mboxwatch(VP mbox,VP cb);
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

#ifdef STDLIBXD
VP xdget(VP fname);
VP xdset(VP fname,VP data);
#endif

