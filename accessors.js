// TODO rename accessor functions to something pretty
var max=0;
var lib=require('./common.js');
console.log(lib.prelude);
lib.each(lib.types, function(t) {
	if(t[8]==false) {
		var tmpls = [
			'',
			'/* accessors for simple type #{{0}} or {{1}} ({{2}}) implemented as {{3}} */',
			'#define MAX_{{1}} {{7}}',
			'#define CTYPE_{{1}} {{3}}',
			'#define T_{{1}} {{0}}',
			'#define IS_{{1}}(v) ( (v)->t=={{0}} )',
			'#define AS_{{1}}(v,n) ({ \\\n\tASSERT(IS_{{1}}(v)==1, "AS_{{1}}: type not {{1}}");\\\n\t{{3}} __x=EL(v,{{3}},n); __x; })',
			'/* create a single-item {{2}} containing C value x (a {{3}}) */',
			'static inline VP x{{1}}({{3}} x) { VP a; a=xalloc({{0}},1); EL(a,{{3}},0)=x; a->n=1; return a; }',
			'/* create an empty {{2}} */',
			'static inline VP x{{1}}0() { VP a=xalloc({{0}},1); return a; }',
			'/* return an empty {{2}} of size sz */',
			'static inline VP x{{1}}sz(int sz) { VP a=xalloc({{0}},sz); return a; }',
			'/* append C value y (a {{3}}) to x */',
			'static inline VP x{{1}}a(VP x, {{3}} y) { x=xrealloc(x,x->n++); EL(x,{{3}},x->n-1)=y; return x; }',
			'/* create a {{2}} using C variable arguments - VP r = x{{1}}n(2, {{3}}_a, {{3}}_b) */',
			'static inline VP x{{1}}n(int nargs,...) { ',
			'\tVP a; va_list args; int i; {{4}} x;',
			'\ta=xalloc(T_{{1}},nargs); a->n=nargs;',
			'\tva_start(args,nargs);',
			'\tfor(i=0;i<nargs;i++){ x=va_arg(args,{{4}}); EL(a,{{3}},i)=x; }',
			'\treturn a;',
			'}',
			'/* append to {{2}} using var args */',
			'static inline VP x{{1}}an(VP a, int nargs,...) {',
			'\tva_list args; int i; {{3}} x;',
			'\ta=xrealloc(a,a->n+nargs); va_start(args,nargs);',
			'\tfor(i=0;i<nargs;i++) {',
			'\t\tx=va_arg(args,{{4}});',
			'\t\tEL(a,{{3}},(a->n)+i)=x;',
			'\t}',
			'\ta->n+=nargs;',
			'\treturn a;',
			'}'
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
			'',
			'/* accessors for container type #{{0}} or {{1}} ({{2}}) implemented as {{3}} */',
			'#define MAX_{{1}} {{7}}',
			'#define CTYPE_{{1}} {{3}}',
			'#define T_{{1}} {{0}}',
			'#define IS_{{1}}(v) ( (v)->t=={{0}} )',
			'#define AS_{{1}}(v,n) ({ \\\n\tASSERT(IS_{{1}}(v)==1, "AS_{{1}}: type not {{1}}");\\\n\t{{3}} __x=EL(v,{{3}},n); __x; })',
			'/* create a {{2}} containing a {{3}} */',
			'static inline VP x{{1}}({{3}} x) { VP a; a=xalloc({{0}},1); EL(a,{{3}},0)=xref(x); a->n=1; return a; }',
			'/* create an empty {{2}} */', 
			'static inline VP x{{1}}0() { VP a=xalloc({{0}},1); return a; }',
			'/* create an empty but presized {{2}} of length sz */',
			'static inline VP x{{1}}sz(int sz) { VP a=xalloc({{0}},sz); return a; }',
			'/* append {{3}} x to {{2}} a */',
			'static inline VP x{{1}}a(VP a, {{3}} x) { a=xrealloc(a,a->n++); EL(a,{{3}},a->n-1)=xref(x); return a; }',
			'/* create a {{2}} from var args */',
			'static inline VP x{{1}}n(int nargs,...) {',
			'\tVP a; va_list args; int i; {{4}} x;',
			'\ta=xalloc({{0}},nargs); a->n=nargs; va_start(args,nargs);',
			'\tfor(i=0;i<nargs;i++){ x=va_arg(args,{{4}}); EL(a,{{3}},i)=xref(x); }',
			'\treturn a;',
			'}',
			'/* append to {{2}} a with var args */',
			'static inline VP x{{1}}an(VP a, int nargs,...) {',
			'\tva_list args; int i; {{3}} x;',
			'\ta=xrealloc(a,a->n+nargs); va_start(args,nargs);',
			'\tfor(i=0;i<nargs;i++) {',
			'\t\tx=va_arg(args,{{4}});',
			'\t\tEL(a,{{3}},(a->n)+i)=xref(x);',
			'\t}',
			'\ta->n+=nargs;',
			'\treturn a;',
			'}'
		];
		lib.each(tmpls,function(tmpl) {
			console.log(lib.exhaust(lib.projr(lib.repl,t),tmpl));
		});
		if(t[0]>max) max=t[0];
	}
});
console.log('\n\nstatic int MAX_TYPE = '+max+';\n\n');
