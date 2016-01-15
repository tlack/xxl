/* some prototypes */
VP xalloc(type_t t,I32 initn);
VP xrealloc(VP x,I32 newn);
VP xfree(VP x);
char* repr0(VP x,char* s,size_t len);
char* reprA(VP x);
int _find(VP x,VP y);
VP entag(VP x,VP t);
VP append(VP x,VP y);
VP entags(VP x,const char* name);
VP tagname(I32 tag);
VP tagv(const char* name, VP x);
int _tagnum(VP name);
int _tagnums(const char* name);
const char* sfromx(VP x);
VP match(VP obj,VP pat);

