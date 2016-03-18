var lib = require('./common.js');
console.log(lib.prelude);
lib.each(lib.types, function(t) {
	if(t[6] == true) return;
	var tmpls = [
		"char* repr_{{1}}(VP x,char* s,size_t sz) { int i; \n"+
			"\tIF_RET(x->n==0,FMT_into_s(sz,\"x{{1}}0()\",0));\n" + 
			"\tif(!SIMPLE(x)) { FMT_into_s(sz,\"'{{1}}(...)\",0); return s; }\n"+
			"\telse if(x->n>1) FMT_into_s(sz,\"(\",0);\n"+
			"\tfor(i=0;i<x->n-1;i++) {\n"+
			"\t\tif(REPR_MAX_ITEMS && i==(REPR_MAX_ITEMS/2)) {\n"+
			"\t\t\tFMT_into_s(sz,\".. (%d omitted) ..\", (x->n)-REPR_MAX_ITEMS);\n"+
			"\t\t\ti+=REPR_MAX_ITEMS;\n"+
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
