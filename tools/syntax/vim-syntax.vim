" Vim syntax file
" Language: XXL <http://github.com/tlack/xxl/>
" Maintainer: Thomas Lackner
" Latest Revision: 29 Feb 2016
if exists("b:current_syntax")
	finish
endif
syn keyword xxlTodo TODO FIXME FIX XXX "NB." NOTE GOTCHA contained
syn match xxlComment "/\*.*\*/" contains=xxlTodo
syn match xxlComment "//.*$" contains=xxlTodo
syn region xxlComment start="/\*" end="\*/" contains=xxlTodo
syn region xxlString start=+"+ skip=+\\.+ end=+"+
syn region xxlBlock start="{" end="}" fold transparent
syn match xxlSym "'[A-Za-z_\?\.]\+\s*" 
syn match xxlName "[A-Za-z_\?\.]\+\s*" 
syn match xxlTypeSym "\'\(list\|tag\|byte\|char\|int\|long\|float\|dict\|table\)\W"
syn match xxlSep ";"
syn match xxlNum '\d\+' 
syn match xxlNum '[-+]\d\+' 
syn match xxlNum '\d\+\.\d*' 
syn match xxlNum '[-+]\d\+\.\d*' 
syn match xxlNum '[-+]\=\d[[:digit:]]*[eE][\-+]\=\d\+' 
syn match xxlNum '\d[[:digit:]]*[eE][\-+]\=\d\+' 
syn match xxlNum '[-+]\=\d[[:digit:]]*\.\d*[eE][\-+]\=\d\+'
syn match xxlNum '\d[[:digit:]]*\.\d*[eE][\-+]\=\d\+'
syn keyword xxlSpecial .
syn keyword xxlPrim as
syn keyword xxlPrim is
syn keyword xxlPrim get
syn keyword xxlPrim split
syn keyword xxlPrim join
syn keyword xxlPrim take
syn keyword xxlPrim drop
syn keyword xxlPrim enlist
syn keyword xxlPrim except
syn keyword xxlPrim flat
syn keyword xxlPrim case
syn keyword xxlPrim each
syn keyword xxlPrim parse
syn keyword xxlPrim show
syn keyword xxlPrim sys
syn keyword xxlPrim any
syn keyword xxlPrim arity
syn keyword xxlPrim behead
syn keyword xxlPrim condense
syn keyword xxlPrim count
syn keyword xxlPrim clone
syn keyword xxlPrim curtail
syn keyword xxlPrim info
syn keyword xxlPrim first
syn keyword xxlPrim last
syn keyword xxlPrim list
syn keyword xxlPrim len
syn keyword xxlPrim key
syn keyword xxlPrim min
syn keyword xxlPrim max
syn keyword xxlPrim neg
syn keyword xxlPrim not
syn keyword xxlPrim repr
syn keyword xxlPrim rev
syn keyword xxlPrim selftest
syn keyword xxlPrim str
syn keyword xxlPrim sum
syn keyword xxlPrim sums
syn keyword xxlPrim tag
syn keyword xxlPrim type
syn keyword xxlPrim val
syn keyword xxlPrim vec
syn keyword xxlPrim xray
syn keyword xxlPrim ::
syn keyword xxlPrim amend
syn keyword xxlPrim and
syn keyword xxlPrim aside
syn keyword xxlPrim base
syn keyword xxlPrim bracketj
syn keyword xxlPrim call
syn keyword xxlPrim consecj
syn keyword xxlPrim deal
syn keyword xxlPrim deep
syn keyword xxlPrim eachl
syn keyword xxlPrim eachr
syn keyword xxlPrim emit
syn keyword xxlPrim evalin
syn keyword xxlPrim exhaust
syn keyword xxlPrim get
syn keyword xxlPrim iftrue
syn keyword xxlPrim ifelse
syn keyword xxlPrim in
syn keyword xxlPrim loadin
syn keyword xxlPrim make
syn keyword xxlPrim nest
syn keyword xxlPrim pick
syn keyword xxlPrim or
syn keyword xxlPrim orelse
syn keyword xxlPrim over
syn keyword xxlPrim range
syn keyword xxlPrim ravel
syn keyword xxlPrim recurse
syn keyword xxlPrim rot
syn keyword xxlPrim scan
syn keyword xxlPrim wide

let b:current_syntax = "xxl"
hi def link xxlBlock       Statement
hi def link xxlComment     Comment
hi def link xxlNum         Constant
hi def link xxlName        Identifier
hi def link xxlPrim        Special
hi def link xxlSep         Separator
hi def link xxlSpecial     Special
hi def link xxlString      Constant
hi def link xxlSym         Identifier
hi def link xxlTodo        Todo
hi def link xxlTypeSym     Type

