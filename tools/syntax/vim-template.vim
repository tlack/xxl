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
__KEYWORDS__

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
