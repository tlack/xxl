var lib = require('./common.js');
console.log(lib.prelude);
lib.each(lib.types, function(t) {
	if(t[6] == true) return;
	var tmpls = [
		"char* repr_{{1}}(VP x,char* s,size_t sz) { int i; \n"+
			"IF_RET(x->n==0,APF(sz,\"x{{1}}0()\",0));\n" + 
			"if(x->n>1) APF(sz,\"[\",0);\n"+
			"for(i=0;i<x->n-1;i++) snprintf(s+strlen(s),sz-strlen(s)-1,\"{{5}},\",AS_{{1}}(x,i)); \n"+
			"snprintf(s+strlen(s),sz-strlen(s)-1,\"{{5}}{{1}}\",AS_{{1}}(x,i)); \n"+
			"if(x->n>1) APF(sz,\"]\",0);\n"+
			"return s; }\n"
	];
	lib.each(tmpls,function(tmpl) {
		console.log(lib.exhaust(lib.projr(lib.repl,t),tmpl));
	});
});
