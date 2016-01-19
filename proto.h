// prototypes

// from xxl.c
int _find(VP x,VP y);
VP apply(VP x,VP y);
VP append(VP x,VP y);
VP cast(VP x,VP y);
VP capacity(VP x);
VP each(VP obj,VP fun);
VP entag(VP x,VP t);
VP entags(VP x,const char* name);
int _find1(VP x,VP y);
VP info(VP x);
VP itemsz(VP x);
VP len(VP x); 
VP match(VP obj,VP pat);
int matchpass(VP obj,VP pat);
char* repr0(VP x,char* s,size_t len);
char* reprA(VP x);
const char* sfromx(VP x);
VP tagname(I32 tag);
VP tagv(const char* name, VP x);
int _tagnum(VP name);
int _tagnums(const char* name);
type_info_t typeinfo(type_t n); 
type_info_t typechar(char c);
VP xalloc(type_t t,I32 initn);
VP xfree(VP x);
VP xfroms(const char* str);
VP xrealloc(VP x,I32 newn);

// from net.c
void net(void);
size_t netr(int sock,void* b,size_t maxl);
void netw(int sock,void* b,size_t l);
void netloop(int sock);
