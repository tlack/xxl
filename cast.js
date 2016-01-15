var lib = require('./common.js');
var tmpls=[
	"int tag{{2}}=Ti({{2}});",
	"if(x->t=={{x0}}&&(typenum=={{y0}}||typetag==tag{{y2}})) { // {{x2}} -> {{y2}} \n" +
	"  {{x3}} from;{{y3}} to; res=xalloc({{y0}},x->n); \n"+ 
	"  FOR(0,x->n,{from=AS_{{x1}}(x,_i);\n" +
  "  to=from; \n"+
	"  appendbuf(res,&to,1); }); }\n"
];
lib.each(lib.types,function(tx) {
	console.log(lib.exhaust(lib.projr(lib.repl,tx),tmpls[0]));
});
lib.each(lib.types,function(tx) {
	if(lib.dontcast.indexOf(tx[1])!=-1)return;
	lib.each(lib.types,function(ty) {
		if(lib.dontcast.indexOf(ty[1])!=-1)return;
		var a=[];
		tmpl=tmpls[1];
		for(var i in tx)a["x"+i]=tx[i];
		for(var i in ty)a["y"+i]=ty[i];
		console.log(lib.exhaust(lib.projr(lib.repl,a),tmpl));
	});
});

