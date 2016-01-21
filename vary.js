// 
// In C, it's difficult to have code that is variadic based on run time
// parameters. For instance you can't make C easily create a long as variable
// "a" in some cases and a byte as variable "a" in other cases based on some
// runtime variable's contents. You have to switch based on the type, often
// allocate memory and do a lot of extra work, and that makes everything
// complicated and time consuming to write.
//
// This code creates some macros that make it easier to write C code that
// varies by type.
//
// TODO VARY_*() macros need to handle general list more robustly
// TODO VARY_*() should be able to cast to best sizes in some circumstances
// TODO consider switching to ribosome of these kinds of scripts
// TODO VARY_*() macros should be rewritten in terms of the minimum subset of types they accept
//
var lib = require('./common.js');
function skip(t) {
	t=t[1];
	return (lib.dontcast.indexOf(t)!=-1)?true:false;
}
console.log(lib.prelude + 
	"// vary on a single element. unpacks x[i] into a C variable \n"+
	"// called _x, of the correct c native type. then executes \n"+
	"// your code (stmt). If it's a type that can't be unpacked, \n"+
	"// the TAG of the type is set in failvar, you can handle it. \n\n"+
	"// VARY_EL varies over one element: x[i]. \n"+
	"#define VARY_EL(x,i,stmt,failvar) ({ \\");
var tmpls=[
	"\tif(x->t=={{x0}}){/*cant vary {{x2}}*/ failvar={{x0}};}\\",
	"\tif(x->t=={{x0}}){/*{{x2}}*/\\\n"+
	"\t\t{{x3}} _x=AS_{{x1}}(x,i);\\\n"+
	"\t\tstmt;}\\" 
];
lib.each(lib.types,function(tx) {
	if(skip(tx))tmpl=tmpls[0];
	else tmpl=tmpls[1];
	var a=[];
	for(var i in tx)a["x"+i]=tx[i];
	console.log(lib.exhaust(lib.projr(lib.repl,a),tmpl));
});
console.log("})");

console.log("#define VARY_EACH(x,stmt,failvar) ({ \\\n" +
	"\tint _i=0,_xn=x->n,_xt=x->t; /*PF(\"VE\");DUMP(x);*/\\");
var tmpl="\tif(_xt=={{x0}}){/*cant vary {{x2}}*/ failvar={{x0}}; }\\";
lib.each(lib.types,function(tx) {
	if(skip(tx)){
		var a=[];
		for(var i in tx)a["x"+i]=tx[i];
		console.log(lib.exhaust(lib.projr(lib.repl,a),tmpl));
	}
});
var tmpl="\tif(_xt=={{x0}}){/*{{x2}}*/ \\\n" +
	"\t\t{{x3}} _x;\\\n" + 
	"\t\twhile (_i < _xn) { _x=AS_{{x1}}(x,_i); /* printf(\"%d {{5}}\\n\", _i, _x); */ stmt; _i++; }\\\n" + 
	"\t}\\";
lib.each(lib.types,function(tx) {
	if(skip(tx))return;
	var a=[];
	for(var i in tx)a["x"+i]=tx[i];
	console.log(lib.exhaust(lib.projr(lib.repl,a),tmpl));
});
console.log("})");

console.log("#define VARY_EACHBOTH(x,y,stmt,failvar) ({ \\\n" +
	"\tint _i=0,_j=0,_xn=x->n,_yn=y->n,_xt=x->t,_yt=y->t;\\");
var tmpl="\tif(_xt=={{x0}}||_yt=={{x0}}){/*cant vary {{x2}}*/ failvar={{x0}}; }\\";
lib.each(lib.types,function(tx) {
	if(skip(tx)){
		var a=[];
		for(var i in tx)a["x"+i]=tx[i];
		console.log(lib.exhaust(lib.projr(lib.repl,a),tmpl));
	}
});
var tmpl="\tif(_xt=={{x0}}&&_yt=={{y0}}){/*{{x2}} x {{y2}}*/ \\\n" +
	"\t\t{{x3}} _x;{{y3}} _y;\\\n" + 
	"\t\twhile (_i < _xn && _j < _yn) { _x=AS_{{x1}}(x,_i); _y=AS_{{y1}}(y,_j); stmt; _i++; _j++; }\\\n" + 
	"\t}\\";
lib.each(lib.types,function(tx) {
	if(skip(tx))return;
	lib.each(lib.types, function(ty) { 
		if(skip(ty))return;
		var a=[];
		for(var i in tx)a["x"+i]=tx[i];
		for(var i in ty)a["y"+i]=ty[i];
		console.log(lib.exhaust(lib.projr(lib.repl,a),tmpl));
	});
});
console.log("})");

console.log("#define VARY_EACHLEFT(x,y,stmt,failvar) ({ \\\n" +
	"\tint _i=0,_j=0,_xn=x->n,_yn=y->n,_xt=x->t,_yt=y->t;\\");
var tmpl="\tif(_xt=={{x0}}||_yt=={{x0}}){/*cant vary {{x2}}*/ failvar={{x0}}; }\\";
lib.each(lib.types,function(tx) {
	if(skip(tx)){
		var a=[];
		for(var i in tx)a["x"+i]=tx[i];
		console.log(lib.exhaust(lib.projr(lib.repl,a),tmpl));
	}
});
var tmpl="\tif(_xt=={{x0}}&&_yt=={{y0}}){/*{{x2}} x {{y2}}*/ \\\n" +
	"\t\t{{x3}} _x;{{y3}} _y; _y=AS_{{x1}}(y,0);\\\n" + 
	"\t\twhile (_i < _xn) { _x=AS_{{x1}}(x,_i); stmt; _i++; }\\\n" + 
	"\t}\\";
lib.each(lib.types,function(tx) {
	if(skip(tx))return;
	lib.each(lib.types, function(ty) { 
		if(skip(ty))return;
		var a=[];
		for(var i in tx)a["x"+i]=tx[i];
		for(var i in ty)a["y"+i]=ty[i];
		console.log(lib.exhaust(lib.projr(lib.repl,a),tmpl));
	});
});
console.log("})");

console.log("#define VARY_EACHRIGHT(x,y,stmt,failvar) ({ \\\n" +
	"\tint _i=0,_j=0,_xn=x->n,_yn=y->n,_xt=x->t,_yt=y->t;\\");
var tmpl="\tif(_xt=={{x0}}||_yt=={{x0}}){/*cant vary {{x2}}*/ failvar={{x0}}; }\\";
lib.each(lib.types,function(tx) {
	if(skip(tx)){
		var a=[];
		for(var i in tx)a["x"+i]=tx[i];
		console.log(lib.exhaust(lib.projr(lib.repl,a),tmpl));
	}
});
var tmpl="\tif(_xt=={{x0}}&&_yt=={{y0}}){/*{{x2}} x {{y2}}*/ \\\n" +
	"\t\t{{x3}} _x;{{y3}} _y; _x=AS_{{x1}}(x,0);\\\n" + 
	"\t\twhile (_j < _yn) { _y=AS_{{y1}}(y,_j); stmt; _j++; }\\\n" + 
	"\t}\\";
lib.each(lib.types,function(tx) {
	if(skip(tx))return;
	lib.each(lib.types, function(ty) { 
		if(skip(ty))return;
		var a=[];
		for(var i in tx)a["x"+i]=tx[i];
		for(var i in ty)a["y"+i]=ty[i];
		console.log(lib.exhaust(lib.projr(lib.repl,a),tmpl));
	});
});
console.log("})");

