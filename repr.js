var lib = require('./common.js');
console.log(lib.prelude);
lib.each(lib.types, function(t) {
	if(t[6] == true) return;
	var tmpls = [
		"char* repr_{{1}}(VP x,char* s,size_t sz) { int i,xn=x->n,skipn=0,skipstart=-1,skipend=-1; \n"+
			"\tIF_RET(xn==0,FMT_into_s(sz,\"x{{1}}0()\",0));\n" + 
			"\tif(!SIMPLE(x)) { FMT_into_s(sz,\"'{{1}}(...)\",0); return s; }\n"+
			"\telse if(xn>1) FMT_into_s(sz,\"(\",0);\n"+
			"\tif(REPR_MAX_ITEMS && xn > REPR_MAX_ITEMS) {\n"+
			"\t\tskipn=xn-REPR_MAX_ITEMS; skipstart=(xn-skipn)/2; skipend=(xn+skipn)/2; }\n"+
			"\tfor(i=0;i<xn-1;i++) {\n"+
			"\t\tif(skipn && i==skipstart) {\n"+
			"\t\t\tFMT_into_s(sz,\".. (%d omitted) ..\",skipn);\n"+
			"\t\t\ti=skipend;\n"+
			"\t\t\tcontinue;}\n"+
			"\t\tFMT_into_s(sz,\"{{5}},\",AS_{{1}}(x,i)); \n"+
			"\t}\n"+
			"\tif(SIMPLE(x)) FMT_into_s(sz,\"{{5}}\",AS_{{1}}(x,i)); \n"+
			"if(x->n>1) FMT_into_s(sz,\")\",0);\n"+
			"return s; }\n"
	];
	lib.each(tmpls,function(tmpl) {
		console.log(lib.exhaust(lib.projr(lib.repl,t),tmpl));
	});
});
