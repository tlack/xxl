// TODO rename accessor functions to something pretty
var max=0;
var lib=require('./common.js');
console.log(lib.prelude);
lib.each(lib.types, function(t) {
	if(t[8]==false) {
		var tmpls = [
			'/* accessors for simple type #{{0}} or {{1}} ({{2}}) implemented as {{3}} */',
			'#define MAX_{{1}} {{7}}',
			'#define CTYPE_{{1}} {{3}}',
			'#define T_{{1}} {{0}}',
			'#define IS_{{1}}(v) ( (v)->t=={{0}} )',
			'#define AS_{{1}}(v,n) ({ \\\n\tASSERT(IS_{{1}}(v)==1, "AS_{{1}}: type not {{1}}");\\\n\t{{3}} __x=EL(v,{{3}},n); __x; })',
			'static inline VP x{{1}}({{3}} x) { VP a; a=xalloc({{0}},1); EL(a,{{3}},0)=x; a->n=1; return a; }',
			'static inline VP x{{1}}0() { VP a=xalloc({{0}},1); return a; }',
			'static inline VP x{{1}}sz(sz) { VP a=xalloc({{0}},sz); return a; }',
			'static inline VP x{{1}}a(VP a, {{3}} x) { a=xrealloc(a,a->n++);EL(a,{{3}},a->n-1)=x; return a; }',
			'static inline VP x{{1}}n(int nargs,...) { VP a; va_list args; int i; {{4}} x; a=xalloc({{0}},nargs); a->n=nargs; va_start(args,nargs);\\\n\t'+
				'for(i=0;i<nargs;i++){ x=va_arg(args,{{4}}); EL(a,{{3}},i)=x; }\\\n\treturn a; }',
			'static inline VP x{{1}}an(VP a, int nargs,...) { va_list args; int i; {{3}} x; a=xrealloc(a,a->n+nargs); va_start(args,nargs);\\\n\t'+
				'for(i=0;i<nargs;i++){ x=va_arg(args,{{4}});\n\tEL(a,{{3}},(a->n)+i)=x; }\\\n\ta->n+=nargs; return a; }'
		];
		lib.each(tmpls,function(tmpl) {
			console.log(lib.exhaust(lib.projr(lib.repl,t),tmpl));
		});
		if(t[0]>max) max=t[0];
	}
});
console.log("\n");
lib.each(lib.types, function(t) {
	if(t[8]==true) {
		var tmpls = [
			'/* accessors for container type #{{0}} or {{1}} ({{2}}) implemented as {{3}} */',
			'#define MAX_{{1}} {{7}}',
			'#define CTYPE_{{1}} {{3}}',
			'#define T_{{1}} {{0}}',
			'#define IS_{{1}}(v) ( (v)->t=={{0}} )',
			'#define AS_{{1}}(v,n) ({ \\\n\tASSERT(IS_{{1}}(v)==1, "AS_{{1}}: type not {{1}}");\\\n\t{{3}} __x=EL(v,{{3}},n); __x; })',
			'static inline VP x{{1}}({{3}} x) { ',
			'  VP a; a=xalloc({{0}},1); EL(a,{{3}},0)=xref(x); a->n=1; return a; }',
			'static inline VP x{{1}}0() { VP a=xalloc({{0}},1); return a; }',
			'static inline VP x{{1}}sz(int sz) { VP a=xalloc({{0}},sz); return a; }',
			'static inline VP x{{1}}a(VP a, {{3}} x) { a=xrealloc(a,a->n++); EL(a,{{3}},a->n-1)=xref(x); return a; }',
			'static inline VP x{{1}}n(int nargs,...) { VP a; va_list args; int i; {{4}} x;',
			'  a=xalloc({{0}},nargs); a->n=nargs; va_start(args,nargs);\\\n\t'+
			'  for(i=0;i<nargs;i++){ x=va_arg(args,{{4}}); EL(a,{{3}},i)=xref(x); }\\\n\treturn a; }',
			'static inline VP x{{1}}an(VP a, int nargs,...) { va_list args; int i; {{3}} x; a=xrealloc(a,a->n+nargs); va_start(args,nargs);\\\n\t'+
			'  for(i=0;i<nargs;i++){ x=va_arg(args,{{4}});\n\tEL(a,{{3}},(a->n)+i)=xref(x); }\\\n\ta->n+=nargs; return a; }'
		];
		lib.each(tmpls,function(tmpl) {
			console.log(lib.exhaust(lib.projr(lib.repl,t),tmpl));
		});
		if(t[0]>max) max=t[0];
	}
});
console.log('\n\nstatic int MAX_TYPE = '+max+';\n\n');
