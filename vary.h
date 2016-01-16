// Autogenerated file; see corresponding .js
// vary on a single element. unpacks x[i] into a C variable 
// called _x, of the correct c native type. then executes 
// your code (stmt). If it's a type that can't be unpacked, 
// the TAG of the type is set in failvar, you can handle it. 

// VARY_EL varies over one element: x[i]. 
#define VARY_EL(x,i,stmt,failvar) ({ \
	if(x->t==0){/*cant vary list*/ failvar=0;}\
	if(x->t==1){/*tag*/\
		int _x=AS_t(x,i);\
		stmt;}\
	if(x->t==2){/*byte*/\
		int8_t _x=AS_b(x,i);\
		stmt;}\
	if(x->t==3){/*int*/\
		int _x=AS_i(x,i);\
		stmt;}\
	if(x->t==4){/*long*/\
		__int64_t _x=AS_j(x,i);\
		stmt;}\
	if(x->t==5){/*octo*/\
		__int128_t _x=AS_o(x,i);\
		stmt;}\
	if(x->t==6){/*char*/\
		char _x=AS_c(x,i);\
		stmt;}\
	if(x->t==7){/*cant vary dict*/ failvar=7;}\
	if(x->t==8){/*cant vary f1*/ failvar=8;}\
	if(x->t==9){/*cant vary f2*/ failvar=9;}\
	if(x->t==10){/*cant vary proj*/ failvar=10;}\
	if(x->t==11){/*cant vary ctx*/ failvar=11;}\
})
#define VARY_EACH(x,stmt,failvar) ({ \
	int _i=0,_xn=x->n,_xt=x->t; /*PF("VE");DUMP(x);*/\
	if(_xt==0){/*cant vary list*/ failvar=0; }\
	if(_xt==7){/*cant vary dict*/ failvar=7; }\
	if(_xt==8){/*cant vary f1*/ failvar=8; }\
	if(_xt==9){/*cant vary f2*/ failvar=9; }\
	if(_xt==10){/*cant vary proj*/ failvar=10; }\
	if(_xt==11){/*cant vary ctx*/ failvar=11; }\
	if(_xt==1){/*tag*/ \
		int _x;\
		while (_i < _xn) { _x=AS_t(x,_i); /* printf("%d {{5}}\n", _i, _x); */ stmt; _i++; }\
	}\
	if(_xt==2){/*byte*/ \
		int8_t _x;\
		while (_i < _xn) { _x=AS_b(x,_i); /* printf("%d {{5}}\n", _i, _x); */ stmt; _i++; }\
	}\
	if(_xt==3){/*int*/ \
		int _x;\
		while (_i < _xn) { _x=AS_i(x,_i); /* printf("%d {{5}}\n", _i, _x); */ stmt; _i++; }\
	}\
	if(_xt==4){/*long*/ \
		__int64_t _x;\
		while (_i < _xn) { _x=AS_j(x,_i); /* printf("%d {{5}}\n", _i, _x); */ stmt; _i++; }\
	}\
	if(_xt==5){/*octo*/ \
		__int128_t _x;\
		while (_i < _xn) { _x=AS_o(x,_i); /* printf("%d {{5}}\n", _i, _x); */ stmt; _i++; }\
	}\
	if(_xt==6){/*char*/ \
		char _x;\
		while (_i < _xn) { _x=AS_c(x,_i); /* printf("%d {{5}}\n", _i, _x); */ stmt; _i++; }\
	}\
})
#define VARY_EACHLEFT(x,y,stmt,failvar) ({ \
	int _i=0,_j=0,_xn=x->n,_yn=y->n,_xt=x->t,_yt=y->t;\
	if(xt==0||yt==0){/*cant vary list*/ failvar=0; }\
	if(xt==7||yt==7){/*cant vary dict*/ failvar=7; }\
	if(xt==8||yt==8){/*cant vary f1*/ failvar=8; }\
	if(xt==9||yt==9){/*cant vary f2*/ failvar=9; }\
	if(xt==10||yt==10){/*cant vary proj*/ failvar=10; }\
	if(xt==11||yt==11){/*cant vary ctx*/ failvar=11; }\
	if(xt==1&&yt==1){/*tag x tag*/ \
		int _x;int _y; _y=AS_t(y,0);\
		while (_i < _xn) { _x=AS_t(x,_i); stmt; _i++; }\
	}\
	if(xt==1&&yt==2){/*tag x byte*/ \
		int _x;int8_t _y; _y=AS_t(y,0);\
		while (_i < _xn) { _x=AS_t(x,_i); stmt; _i++; }\
	}\
	if(xt==1&&yt==3){/*tag x int*/ \
		int _x;int _y; _y=AS_t(y,0);\
		while (_i < _xn) { _x=AS_t(x,_i); stmt; _i++; }\
	}\
	if(xt==1&&yt==4){/*tag x long*/ \
		int _x;__int64_t _y; _y=AS_t(y,0);\
		while (_i < _xn) { _x=AS_t(x,_i); stmt; _i++; }\
	}\
	if(xt==1&&yt==5){/*tag x octo*/ \
		int _x;__int128_t _y; _y=AS_t(y,0);\
		while (_i < _xn) { _x=AS_t(x,_i); stmt; _i++; }\
	}\
	if(xt==1&&yt==6){/*tag x char*/ \
		int _x;char _y; _y=AS_t(y,0);\
		while (_i < _xn) { _x=AS_t(x,_i); stmt; _i++; }\
	}\
	if(xt==2&&yt==1){/*byte x tag*/ \
		int8_t _x;int _y; _y=AS_b(y,0);\
		while (_i < _xn) { _x=AS_b(x,_i); stmt; _i++; }\
	}\
	if(xt==2&&yt==2){/*byte x byte*/ \
		int8_t _x;int8_t _y; _y=AS_b(y,0);\
		while (_i < _xn) { _x=AS_b(x,_i); stmt; _i++; }\
	}\
	if(xt==2&&yt==3){/*byte x int*/ \
		int8_t _x;int _y; _y=AS_b(y,0);\
		while (_i < _xn) { _x=AS_b(x,_i); stmt; _i++; }\
	}\
	if(xt==2&&yt==4){/*byte x long*/ \
		int8_t _x;__int64_t _y; _y=AS_b(y,0);\
		while (_i < _xn) { _x=AS_b(x,_i); stmt; _i++; }\
	}\
	if(xt==2&&yt==5){/*byte x octo*/ \
		int8_t _x;__int128_t _y; _y=AS_b(y,0);\
		while (_i < _xn) { _x=AS_b(x,_i); stmt; _i++; }\
	}\
	if(xt==2&&yt==6){/*byte x char*/ \
		int8_t _x;char _y; _y=AS_b(y,0);\
		while (_i < _xn) { _x=AS_b(x,_i); stmt; _i++; }\
	}\
	if(xt==3&&yt==1){/*int x tag*/ \
		int _x;int _y; _y=AS_i(y,0);\
		while (_i < _xn) { _x=AS_i(x,_i); stmt; _i++; }\
	}\
	if(xt==3&&yt==2){/*int x byte*/ \
		int _x;int8_t _y; _y=AS_i(y,0);\
		while (_i < _xn) { _x=AS_i(x,_i); stmt; _i++; }\
	}\
	if(xt==3&&yt==3){/*int x int*/ \
		int _x;int _y; _y=AS_i(y,0);\
		while (_i < _xn) { _x=AS_i(x,_i); stmt; _i++; }\
	}\
	if(xt==3&&yt==4){/*int x long*/ \
		int _x;__int64_t _y; _y=AS_i(y,0);\
		while (_i < _xn) { _x=AS_i(x,_i); stmt; _i++; }\
	}\
	if(xt==3&&yt==5){/*int x octo*/ \
		int _x;__int128_t _y; _y=AS_i(y,0);\
		while (_i < _xn) { _x=AS_i(x,_i); stmt; _i++; }\
	}\
	if(xt==3&&yt==6){/*int x char*/ \
		int _x;char _y; _y=AS_i(y,0);\
		while (_i < _xn) { _x=AS_i(x,_i); stmt; _i++; }\
	}\
	if(xt==4&&yt==1){/*long x tag*/ \
		__int64_t _x;int _y; _y=AS_j(y,0);\
		while (_i < _xn) { _x=AS_j(x,_i); stmt; _i++; }\
	}\
	if(xt==4&&yt==2){/*long x byte*/ \
		__int64_t _x;int8_t _y; _y=AS_j(y,0);\
		while (_i < _xn) { _x=AS_j(x,_i); stmt; _i++; }\
	}\
	if(xt==4&&yt==3){/*long x int*/ \
		__int64_t _x;int _y; _y=AS_j(y,0);\
		while (_i < _xn) { _x=AS_j(x,_i); stmt; _i++; }\
	}\
	if(xt==4&&yt==4){/*long x long*/ \
		__int64_t _x;__int64_t _y; _y=AS_j(y,0);\
		while (_i < _xn) { _x=AS_j(x,_i); stmt; _i++; }\
	}\
	if(xt==4&&yt==5){/*long x octo*/ \
		__int64_t _x;__int128_t _y; _y=AS_j(y,0);\
		while (_i < _xn) { _x=AS_j(x,_i); stmt; _i++; }\
	}\
	if(xt==4&&yt==6){/*long x char*/ \
		__int64_t _x;char _y; _y=AS_j(y,0);\
		while (_i < _xn) { _x=AS_j(x,_i); stmt; _i++; }\
	}\
	if(xt==5&&yt==1){/*octo x tag*/ \
		__int128_t _x;int _y; _y=AS_o(y,0);\
		while (_i < _xn) { _x=AS_o(x,_i); stmt; _i++; }\
	}\
	if(xt==5&&yt==2){/*octo x byte*/ \
		__int128_t _x;int8_t _y; _y=AS_o(y,0);\
		while (_i < _xn) { _x=AS_o(x,_i); stmt; _i++; }\
	}\
	if(xt==5&&yt==3){/*octo x int*/ \
		__int128_t _x;int _y; _y=AS_o(y,0);\
		while (_i < _xn) { _x=AS_o(x,_i); stmt; _i++; }\
	}\
	if(xt==5&&yt==4){/*octo x long*/ \
		__int128_t _x;__int64_t _y; _y=AS_o(y,0);\
		while (_i < _xn) { _x=AS_o(x,_i); stmt; _i++; }\
	}\
	if(xt==5&&yt==5){/*octo x octo*/ \
		__int128_t _x;__int128_t _y; _y=AS_o(y,0);\
		while (_i < _xn) { _x=AS_o(x,_i); stmt; _i++; }\
	}\
	if(xt==5&&yt==6){/*octo x char*/ \
		__int128_t _x;char _y; _y=AS_o(y,0);\
		while (_i < _xn) { _x=AS_o(x,_i); stmt; _i++; }\
	}\
	if(xt==6&&yt==1){/*char x tag*/ \
		char _x;int _y; _y=AS_c(y,0);\
		while (_i < _xn) { _x=AS_c(x,_i); stmt; _i++; }\
	}\
	if(xt==6&&yt==2){/*char x byte*/ \
		char _x;int8_t _y; _y=AS_c(y,0);\
		while (_i < _xn) { _x=AS_c(x,_i); stmt; _i++; }\
	}\
	if(xt==6&&yt==3){/*char x int*/ \
		char _x;int _y; _y=AS_c(y,0);\
		while (_i < _xn) { _x=AS_c(x,_i); stmt; _i++; }\
	}\
	if(xt==6&&yt==4){/*char x long*/ \
		char _x;__int64_t _y; _y=AS_c(y,0);\
		while (_i < _xn) { _x=AS_c(x,_i); stmt; _i++; }\
	}\
	if(xt==6&&yt==5){/*char x octo*/ \
		char _x;__int128_t _y; _y=AS_c(y,0);\
		while (_i < _xn) { _x=AS_c(x,_i); stmt; _i++; }\
	}\
	if(xt==6&&yt==6){/*char x char*/ \
		char _x;char _y; _y=AS_c(y,0);\
		while (_i < _xn) { _x=AS_c(x,_i); stmt; _i++; }\
	}\
})
#define VARY_EACHRIGHT(x,y,stmt,failvar) ({ \
	int _i=0,_j=0,_xn=x->n,_yn=y->n,_xt=x->t,_yt=y->t;\
	if(xt==0||yt==0){/*cant vary list*/ failvar=0; }\
	if(xt==7||yt==7){/*cant vary dict*/ failvar=7; }\
	if(xt==8||yt==8){/*cant vary f1*/ failvar=8; }\
	if(xt==9||yt==9){/*cant vary f2*/ failvar=9; }\
	if(xt==10||yt==10){/*cant vary proj*/ failvar=10; }\
	if(xt==11||yt==11){/*cant vary ctx*/ failvar=11; }\
	if(xt==1&&yt==1){/*tag x tag*/ \
		int _x;int _y; _x=AS_t(x,0);\
		while (_j < _yn) { _y=AS_t(y,_j); stmt; _j++; }\
	}\
	if(xt==1&&yt==2){/*tag x byte*/ \
		int _x;int8_t _y; _x=AS_t(x,0);\
		while (_j < _yn) { _y=AS_b(y,_j); stmt; _j++; }\
	}\
	if(xt==1&&yt==3){/*tag x int*/ \
		int _x;int _y; _x=AS_t(x,0);\
		while (_j < _yn) { _y=AS_i(y,_j); stmt; _j++; }\
	}\
	if(xt==1&&yt==4){/*tag x long*/ \
		int _x;__int64_t _y; _x=AS_t(x,0);\
		while (_j < _yn) { _y=AS_j(y,_j); stmt; _j++; }\
	}\
	if(xt==1&&yt==5){/*tag x octo*/ \
		int _x;__int128_t _y; _x=AS_t(x,0);\
		while (_j < _yn) { _y=AS_o(y,_j); stmt; _j++; }\
	}\
	if(xt==1&&yt==6){/*tag x char*/ \
		int _x;char _y; _x=AS_t(x,0);\
		while (_j < _yn) { _y=AS_c(y,_j); stmt; _j++; }\
	}\
	if(xt==2&&yt==1){/*byte x tag*/ \
		int8_t _x;int _y; _x=AS_b(x,0);\
		while (_j < _yn) { _y=AS_t(y,_j); stmt; _j++; }\
	}\
	if(xt==2&&yt==2){/*byte x byte*/ \
		int8_t _x;int8_t _y; _x=AS_b(x,0);\
		while (_j < _yn) { _y=AS_b(y,_j); stmt; _j++; }\
	}\
	if(xt==2&&yt==3){/*byte x int*/ \
		int8_t _x;int _y; _x=AS_b(x,0);\
		while (_j < _yn) { _y=AS_i(y,_j); stmt; _j++; }\
	}\
	if(xt==2&&yt==4){/*byte x long*/ \
		int8_t _x;__int64_t _y; _x=AS_b(x,0);\
		while (_j < _yn) { _y=AS_j(y,_j); stmt; _j++; }\
	}\
	if(xt==2&&yt==5){/*byte x octo*/ \
		int8_t _x;__int128_t _y; _x=AS_b(x,0);\
		while (_j < _yn) { _y=AS_o(y,_j); stmt; _j++; }\
	}\
	if(xt==2&&yt==6){/*byte x char*/ \
		int8_t _x;char _y; _x=AS_b(x,0);\
		while (_j < _yn) { _y=AS_c(y,_j); stmt; _j++; }\
	}\
	if(xt==3&&yt==1){/*int x tag*/ \
		int _x;int _y; _x=AS_i(x,0);\
		while (_j < _yn) { _y=AS_t(y,_j); stmt; _j++; }\
	}\
	if(xt==3&&yt==2){/*int x byte*/ \
		int _x;int8_t _y; _x=AS_i(x,0);\
		while (_j < _yn) { _y=AS_b(y,_j); stmt; _j++; }\
	}\
	if(xt==3&&yt==3){/*int x int*/ \
		int _x;int _y; _x=AS_i(x,0);\
		while (_j < _yn) { _y=AS_i(y,_j); stmt; _j++; }\
	}\
	if(xt==3&&yt==4){/*int x long*/ \
		int _x;__int64_t _y; _x=AS_i(x,0);\
		while (_j < _yn) { _y=AS_j(y,_j); stmt; _j++; }\
	}\
	if(xt==3&&yt==5){/*int x octo*/ \
		int _x;__int128_t _y; _x=AS_i(x,0);\
		while (_j < _yn) { _y=AS_o(y,_j); stmt; _j++; }\
	}\
	if(xt==3&&yt==6){/*int x char*/ \
		int _x;char _y; _x=AS_i(x,0);\
		while (_j < _yn) { _y=AS_c(y,_j); stmt; _j++; }\
	}\
	if(xt==4&&yt==1){/*long x tag*/ \
		__int64_t _x;int _y; _x=AS_j(x,0);\
		while (_j < _yn) { _y=AS_t(y,_j); stmt; _j++; }\
	}\
	if(xt==4&&yt==2){/*long x byte*/ \
		__int64_t _x;int8_t _y; _x=AS_j(x,0);\
		while (_j < _yn) { _y=AS_b(y,_j); stmt; _j++; }\
	}\
	if(xt==4&&yt==3){/*long x int*/ \
		__int64_t _x;int _y; _x=AS_j(x,0);\
		while (_j < _yn) { _y=AS_i(y,_j); stmt; _j++; }\
	}\
	if(xt==4&&yt==4){/*long x long*/ \
		__int64_t _x;__int64_t _y; _x=AS_j(x,0);\
		while (_j < _yn) { _y=AS_j(y,_j); stmt; _j++; }\
	}\
	if(xt==4&&yt==5){/*long x octo*/ \
		__int64_t _x;__int128_t _y; _x=AS_j(x,0);\
		while (_j < _yn) { _y=AS_o(y,_j); stmt; _j++; }\
	}\
	if(xt==4&&yt==6){/*long x char*/ \
		__int64_t _x;char _y; _x=AS_j(x,0);\
		while (_j < _yn) { _y=AS_c(y,_j); stmt; _j++; }\
	}\
	if(xt==5&&yt==1){/*octo x tag*/ \
		__int128_t _x;int _y; _x=AS_o(x,0);\
		while (_j < _yn) { _y=AS_t(y,_j); stmt; _j++; }\
	}\
	if(xt==5&&yt==2){/*octo x byte*/ \
		__int128_t _x;int8_t _y; _x=AS_o(x,0);\
		while (_j < _yn) { _y=AS_b(y,_j); stmt; _j++; }\
	}\
	if(xt==5&&yt==3){/*octo x int*/ \
		__int128_t _x;int _y; _x=AS_o(x,0);\
		while (_j < _yn) { _y=AS_i(y,_j); stmt; _j++; }\
	}\
	if(xt==5&&yt==4){/*octo x long*/ \
		__int128_t _x;__int64_t _y; _x=AS_o(x,0);\
		while (_j < _yn) { _y=AS_j(y,_j); stmt; _j++; }\
	}\
	if(xt==5&&yt==5){/*octo x octo*/ \
		__int128_t _x;__int128_t _y; _x=AS_o(x,0);\
		while (_j < _yn) { _y=AS_o(y,_j); stmt; _j++; }\
	}\
	if(xt==5&&yt==6){/*octo x char*/ \
		__int128_t _x;char _y; _x=AS_o(x,0);\
		while (_j < _yn) { _y=AS_c(y,_j); stmt; _j++; }\
	}\
	if(xt==6&&yt==1){/*char x tag*/ \
		char _x;int _y; _x=AS_c(x,0);\
		while (_j < _yn) { _y=AS_t(y,_j); stmt; _j++; }\
	}\
	if(xt==6&&yt==2){/*char x byte*/ \
		char _x;int8_t _y; _x=AS_c(x,0);\
		while (_j < _yn) { _y=AS_b(y,_j); stmt; _j++; }\
	}\
	if(xt==6&&yt==3){/*char x int*/ \
		char _x;int _y; _x=AS_c(x,0);\
		while (_j < _yn) { _y=AS_i(y,_j); stmt; _j++; }\
	}\
	if(xt==6&&yt==4){/*char x long*/ \
		char _x;__int64_t _y; _x=AS_c(x,0);\
		while (_j < _yn) { _y=AS_j(y,_j); stmt; _j++; }\
	}\
	if(xt==6&&yt==5){/*char x octo*/ \
		char _x;__int128_t _y; _x=AS_c(x,0);\
		while (_j < _yn) { _y=AS_o(y,_j); stmt; _j++; }\
	}\
	if(xt==6&&yt==6){/*char x char*/ \
		char _x;char _y; _x=AS_c(x,0);\
		while (_j < _yn) { _y=AS_c(y,_j); stmt; _j++; }\
	}\
})
