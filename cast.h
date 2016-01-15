int taglist=Ti(list);
int tagtag=Ti(tag);
int tagbyte=Ti(byte);
int tagint=Ti(int);
int taglong=Ti(long);
int tagocto=Ti(octo);
int tagchar=Ti(char);
int tagdict=Ti(dict);
int tagf1=Ti(f1);
int tagf2=Ti(f2);
int tagproj=Ti(proj);
int tagctx=Ti(ctx);
if(x->t==1&&(typenum==1||typetag==tagtag)) { // tag -> tag 
  int from;int to; res=xalloc(1,x->n); 
  FOR(0,x->n,{from=AS_t(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==1&&(typenum==2||typetag==tagbyte)) { // tag -> byte 
  int from;int8_t to; res=xalloc(2,x->n); 
  FOR(0,x->n,{from=AS_t(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==1&&(typenum==3||typetag==tagint)) { // tag -> int 
  int from;int to; res=xalloc(3,x->n); 
  FOR(0,x->n,{from=AS_t(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==1&&(typenum==4||typetag==taglong)) { // tag -> long 
  int from;__int64_t to; res=xalloc(4,x->n); 
  FOR(0,x->n,{from=AS_t(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==1&&(typenum==5||typetag==tagocto)) { // tag -> octo 
  int from;__int128_t to; res=xalloc(5,x->n); 
  FOR(0,x->n,{from=AS_t(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==1&&(typenum==6||typetag==tagchar)) { // tag -> char 
  int from;char to; res=xalloc(6,x->n); 
  FOR(0,x->n,{from=AS_t(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==2&&(typenum==1||typetag==tagtag)) { // byte -> tag 
  int8_t from;int to; res=xalloc(1,x->n); 
  FOR(0,x->n,{from=AS_b(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==2&&(typenum==2||typetag==tagbyte)) { // byte -> byte 
  int8_t from;int8_t to; res=xalloc(2,x->n); 
  FOR(0,x->n,{from=AS_b(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==2&&(typenum==3||typetag==tagint)) { // byte -> int 
  int8_t from;int to; res=xalloc(3,x->n); 
  FOR(0,x->n,{from=AS_b(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==2&&(typenum==4||typetag==taglong)) { // byte -> long 
  int8_t from;__int64_t to; res=xalloc(4,x->n); 
  FOR(0,x->n,{from=AS_b(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==2&&(typenum==5||typetag==tagocto)) { // byte -> octo 
  int8_t from;__int128_t to; res=xalloc(5,x->n); 
  FOR(0,x->n,{from=AS_b(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==2&&(typenum==6||typetag==tagchar)) { // byte -> char 
  int8_t from;char to; res=xalloc(6,x->n); 
  FOR(0,x->n,{from=AS_b(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==3&&(typenum==1||typetag==tagtag)) { // int -> tag 
  int from;int to; res=xalloc(1,x->n); 
  FOR(0,x->n,{from=AS_i(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==3&&(typenum==2||typetag==tagbyte)) { // int -> byte 
  int from;int8_t to; res=xalloc(2,x->n); 
  FOR(0,x->n,{from=AS_i(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==3&&(typenum==3||typetag==tagint)) { // int -> int 
  int from;int to; res=xalloc(3,x->n); 
  FOR(0,x->n,{from=AS_i(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==3&&(typenum==4||typetag==taglong)) { // int -> long 
  int from;__int64_t to; res=xalloc(4,x->n); 
  FOR(0,x->n,{from=AS_i(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==3&&(typenum==5||typetag==tagocto)) { // int -> octo 
  int from;__int128_t to; res=xalloc(5,x->n); 
  FOR(0,x->n,{from=AS_i(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==3&&(typenum==6||typetag==tagchar)) { // int -> char 
  int from;char to; res=xalloc(6,x->n); 
  FOR(0,x->n,{from=AS_i(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==4&&(typenum==1||typetag==tagtag)) { // long -> tag 
  __int64_t from;int to; res=xalloc(1,x->n); 
  FOR(0,x->n,{from=AS_j(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==4&&(typenum==2||typetag==tagbyte)) { // long -> byte 
  __int64_t from;int8_t to; res=xalloc(2,x->n); 
  FOR(0,x->n,{from=AS_j(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==4&&(typenum==3||typetag==tagint)) { // long -> int 
  __int64_t from;int to; res=xalloc(3,x->n); 
  FOR(0,x->n,{from=AS_j(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==4&&(typenum==4||typetag==taglong)) { // long -> long 
  __int64_t from;__int64_t to; res=xalloc(4,x->n); 
  FOR(0,x->n,{from=AS_j(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==4&&(typenum==5||typetag==tagocto)) { // long -> octo 
  __int64_t from;__int128_t to; res=xalloc(5,x->n); 
  FOR(0,x->n,{from=AS_j(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==4&&(typenum==6||typetag==tagchar)) { // long -> char 
  __int64_t from;char to; res=xalloc(6,x->n); 
  FOR(0,x->n,{from=AS_j(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==5&&(typenum==1||typetag==tagtag)) { // octo -> tag 
  __int128_t from;int to; res=xalloc(1,x->n); 
  FOR(0,x->n,{from=AS_o(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==5&&(typenum==2||typetag==tagbyte)) { // octo -> byte 
  __int128_t from;int8_t to; res=xalloc(2,x->n); 
  FOR(0,x->n,{from=AS_o(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==5&&(typenum==3||typetag==tagint)) { // octo -> int 
  __int128_t from;int to; res=xalloc(3,x->n); 
  FOR(0,x->n,{from=AS_o(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==5&&(typenum==4||typetag==taglong)) { // octo -> long 
  __int128_t from;__int64_t to; res=xalloc(4,x->n); 
  FOR(0,x->n,{from=AS_o(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==5&&(typenum==5||typetag==tagocto)) { // octo -> octo 
  __int128_t from;__int128_t to; res=xalloc(5,x->n); 
  FOR(0,x->n,{from=AS_o(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==5&&(typenum==6||typetag==tagchar)) { // octo -> char 
  __int128_t from;char to; res=xalloc(6,x->n); 
  FOR(0,x->n,{from=AS_o(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==6&&(typenum==1||typetag==tagtag)) { // char -> tag 
  char from;int to; res=xalloc(1,x->n); 
  FOR(0,x->n,{from=AS_c(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==6&&(typenum==2||typetag==tagbyte)) { // char -> byte 
  char from;int8_t to; res=xalloc(2,x->n); 
  FOR(0,x->n,{from=AS_c(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==6&&(typenum==3||typetag==tagint)) { // char -> int 
  char from;int to; res=xalloc(3,x->n); 
  FOR(0,x->n,{from=AS_c(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==6&&(typenum==4||typetag==taglong)) { // char -> long 
  char from;__int64_t to; res=xalloc(4,x->n); 
  FOR(0,x->n,{from=AS_c(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==6&&(typenum==5||typetag==tagocto)) { // char -> octo 
  char from;__int128_t to; res=xalloc(5,x->n); 
  FOR(0,x->n,{from=AS_c(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

if(x->t==6&&(typenum==6||typetag==tagchar)) { // char -> char 
  char from;char to; res=xalloc(6,x->n); 
  FOR(0,x->n,{from=AS_c(x,_i);
  to=from; 
  appendbuf(res,&to,1); }); }

