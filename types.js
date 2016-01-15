var lib = require('./common.js');
console.log('static type_info_t TYPES[] = { ');
lib.each(lib.types,function(t) {
	var tmpls=[
		"{ {{0}}, '{{1}}', sizeof({{3}}), \"{{2}}\", &repr_{{1}} },"
	];
	lib.each(tmpls,function(tmpl) {
		console.log(lib.exhaust(lib.projr(lib.repl,t),tmpl));
	});
});
console.log('};');

