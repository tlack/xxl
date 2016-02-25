// configure these:
var PIP_REFRESH = 5*1000;               // how often should picture-in-picture refresh?
// code begins here
var KEYSEND=13, KEYHELP=16;
var PIPS={}, TIME;
function $(x) {return document.querySelector(x);}
function $$(x) {return mkarray(document.querySelectorAll(x));}
function $li() {return $('ol li:last-child ');}
function $pr() {return $('ol li:last-child .pr');}
function domempty($) { $.innerHTML=''; }
function get(url,cb) {
	var req = new XMLHttpRequest();
	req.open('GET', url, true);
	req.onload = function() {
		console.log(req);
		if (req.status >= 200 && req.status < 400) cb(true,req.responseText);
		else { alert('onload error'); cb(false,req); }
	};
	req.onerror = function(why) { alert(JSON.stringify(why)); cb(false, req); }
	req.send();
}
function mkarray(x) {return Array.prototype.slice.call(x);}
function mkdom(htmlstr) {
	var d=document.createElement('div'); d.innerHTML="<div>"+htmlstr+"</div>"; return d.childNodes[0];}
function modal(html) { // open with html string or DOM nodes, close with "" 
	var $b=$('body'), bc=$b.className;
	if(html=="") { $b.className = bc.replace(' modal-active',''); return; } // close
	if(html[0]==".") html=$(html).innerHTML;
	if(typeof(html)==typeof("")) html=mkdom(html);
	domempty($('.modal-inner'));
	$('.modal-inner').appendChild(html);
	$b.className = bc+' modal-active';
	repl.bind();
	return;
}
function init() {repl.bind(); repl.draw(); setInterval(pip.draw, pip.REFRESH); }
function show(x) {console.log(x); return x; }
function Bclose() {return modal(""); }
function Bmenu() {return modal(".menudlg"); }
function Bshare() {return modal(".savedlg"); }
function Bpip() {return modal(".pipdlg"); }
var pip={
	REFRESH:PIP_REFRESH,
	body: function(content) {return "<style>"+($('.pipcss').innerHTML)+'</style>'+content;},
	draw: function() {
		if(PIPS.length == 0) return;
		var $p=$("#pips");
		for(var cmd in PIPS) {
			var pipwin=PIPS[cmd];
			if(!pipwin) {
				var pipwin = document.createElement('iframe'); pipwin.src = 'about:blank'; 
				$p.appendChild(pipwin); PIPS[cmd]=pipwin;
			}
			var cw=pipwin.contentWindow.document;
			server.eval(cmd, function(res,data) { 
				if (!res) data="Error: "+data; 
				cw.open(); cw.write(pip.body(data)); cw.close(); });
		}
	},
}
var server={
	eval: function(cmdstr, cb) { get('?'+encodeURIComponent(cmdstr), cb); }
}
var repl={
	$:$('#repl'),
	bind: function() { 
		$$('.pr').forEach(function(e){
			if(e.className.match(/demo/)) e.addEventListener('click', repl.ev.demo, false);
			else e.addEventListener('keydown', repl.ev.key, false); 
		});
	},
	cmd: {
		'pip': function(args) { 
			if (!args) { domempty($('#pips')); return; }
			PIPS[args]=""; pip.draw(); repl.output('Picture-in-picture started'); return true; } 
	},
	draw:function() {
		repl.$.appendChild(show(mkdom('<ol start=0></ol>')));
		repl.draw_.input();
	},
	draw_:{
		input:function() {
			var item=document.createElement('li'); 
			item.innerHTML=repl.draw_.pr();
			console.log(item);
			$('#repl ol').appendChild(item);
			item.focus();
		},
		pr:function() { return "<input type=text onkeydown='repl.ev.key()' class='pr'/>"; }
	},
	ev:{
		demo:function() {
			var tgt=window.event?window.event.target:e.target;
			repl.draw_.input(); modal(""); 
			var p=$pr(); $pr().value=tgt.value; $pr().focus();
		},
		key:function() {
			var CR=KEYSEND;
			var ch=show(window.event?window.event.keyCode:e.which);
			var tgt=window.event?window.event.target:e.target;
			if(ch==CR) { $pr().blur(); modal(""); repl.evalstr(tgt.value); }
			return true;
		}
	},
	evalresp:function(ok,resp) {
		if(!ok) $li().appendChild(mkdom("<div>Error: "+JSON.stringify(resp)+"</div>"));
		else { 
			if(TIME) { resp = "<span class=perf>"+(Date.now()-TIME)+"ms</span>"+resp; }
			repl.output(resp);
		}
	},
	evalstr:function(s) {
		if(s[0]=='/') { var w=s.substr(1).split(' '); console.log(w); if(repl.cmd[w[0]] && repl.cmd[w[0]](w.slice(1).join(' ')))return false; }
		TIME=Date.now();
		server.eval(s, repl.evalresp);
	},
	output:function(html) {
		$li().appendChild(mkdom("<div class=o>"+html+"</div>"));
		repl.draw_.input();
	}
}
init();
