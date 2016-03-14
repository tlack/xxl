![XXL logo](http://www.modernmethod.com/send/files/xxl-logo-2.png)

# What is XXL?

XXL is a new programming language whose goals are simplicity, power, and speed.
We throw out most of conventional programming language thinking in order to
attain these goals.

## Status

Pretty buggy but passes tests. Useful for writing small utilities, for me. Not
yet suitable for real work.

Tested so far on Linux (x86/GCC), OS X (Clang), Windows (Cygwin64). Soliciting
Android runtime on NDK (contact me if interested).

## Examples

### JSON encoder

![json encoder screenshot with syntax highlighting](http://www.modernmethod.com/send/files/jsonencode.png)


```
// enclose (c)urly(b)races, (s)quare(b)brackets, (q)uotes:
'ecb is {"{",x,"}"}; 'esb is {"[",x,"]"}; 'eq is {"\"",x,"\""}; 
'jc is {join ","}; 'jac is {each y jc};  // join x with commas; apply y to each of x then join with commas
'pair is {encode,":",(y encode)};        // key:val pair for dict
'dict is {key as 'k; x val as 'v; [k],v >: pair jc ecb}; // get keys/vals, pair merge, commas, braces

// wrap non-scalar values in appropriate way:
'many is {as 'el type case ('char, {el str eq}, 'dict, {el dict}, {el jac encode esb})};
'encode is {ravel[many,str]};            // ravel calls x y[0] for arrays (len > 1), x y[1] for scalars
```

### Micro Web Counter

Here's an example web server application that acts as a counter. You can run
this as`./c examples/web-ctr.xxl`. Source code in full:

```
0 as 'ctr;
(8080,"localhost").net.bind{
	x show; .ctr + 1 as '.ctr;  // dot in front of name means "parent"
	"HTTP/1.0 200 OK\r\nContent-Type: text/plain\r\n",
	"Connection: close\r\nServer: xxl v0.0\r\n\r\nHello ",
		(.ctr repr),"\r\n" flat}
```

The first line declares a global variable named `ctr`. Variables are defined
using a symbol (also called a tag) containing their name. Symbols start with
apostrophe `'`.  Names starting with `.` are referenced from the root of the
XXL context tree, rather than being resolved in the locals context to start, which makes
them behave something like globals in traditional languages.

The second line invokes the built in `.net.bind` verb. On the left is the configuration
options. In this case, port 8080 on localhost.

On the right side of the `.net.bind` verb we supply an anonymous function body
to act as the callback to be invoked when a connection is made and a request is
received. This callback is invoked with one argument, which is a list
containing the request body in the first member and the remote IP address in
the second.

At the moment, `.net.bind` only supports client-speaks-first-style network
protocols, such as HTTP.

Inside the callback, we display the request on the server's screen (`x show`), increase
and store the counter value, and then generate an HTTP response with a couple of headers.

Since we want to include the counter's current value, we append it to the string after
calling `repr` on the counter value only (since it's in parenthesis). The resulting list
needs to be flattened so we can spit it out as one long string to the client's socket, so
we call XXL's `flat` verb, which removes a level of nesting on a general list (akin to
`raze` in Q).

Voila!

## More examples

This inchoate document is no excuse for real documentation, which is coming soon.

See the various tests for more examples. In particular, you may appreciate
`test-logic.h`, and `test-semantics.h`.

## Philosophy

- *Terse*. It is much better to express yourself clearly in a few words rather
  than many. We provide powerful operators that allow you to do more while
	describing less.

- *Accessible*. XXL avoids niche-y programmer terms like "map" and "reduce" in
	favor of common English 

- *Minimalistic*, but not spare. Provide tools people commonly need, even if
	they're a bit duplicative of other, more primitive approaches.

- *Fast*. XXL cares deeply about performance, for the same reason a person should care
	about the quality of any tool they use for professional work. 

- *Tangible*. Abstractions provided by many systems and platforms make it much harder
  to reason about what your program is doing. XXL just helps you program the computer
	in the way the computer wants to be programmed. Those with a background in assembly
	might see some familiar tones in XXL.

These goals have not yet been attained.

## Motivation

I need a swiss army knife for data manipulation with predictable performance and concise
syntax. I want to use some of the more exotic features I've tasted in other languages,
but not all of them. In particular, I feel like programming has gotten a little too stack-
heavy. 

## Features

### Minimal, easy to grasp syntax

XXL may seem odd at first glance but it's much simpler than other languages
which suffer from complex grammars and rules.

Clean, very easy to understand and parse left-to-right syntax with only three 
special forms: comments, strings, and grouping (i.e., `( )`, `[ ]` and `{ }`).

Everything else is either a noun or a verb.

Verbs are either postfix (unary, one variable, called `x`) or infix (binary,
two variables, called `x` and `y`).

Tags like `'mything` are used as names.

Agnostic about whitespace. Don't let invisible special characters ruin your day.

### Trees

We don't yet have a full tree data type yet, but operations on list-of-lists
are pretty diverse. After all, XXL is written in terms of its own verbs and values.

`x nest [open,close]` creates sublists in x between matching open
and close tags. Create parsers like an animal.

*Note:* In the examples below, `0.`, `1.`, etc are the XXL command line prompt.
Abridged output shown on the line below.

```
0. 1,4,4,0,5,5,6 nest [4,5]
[1i, [4i, (4,0,5i), 5i], 6i]
```

`x ravel [funmany, funone]` calls funone if x is scalar, funmany otherwise.
Pair with `self` to recurse. A napkin sketch of a markdown parser:

```
0. 'body is {behead curtail};
1. 'mdtxt is "My *Markdown* parser";
2. 'html is ['b:{"<b>",x,"</b>"}]; // define some formatting templates
3. mdtxt nest ["*","*"] :: {ravel [{x body html.b},str]} flat
"My <b>Markdown</b> parser"
```

The contents of the html callbacks, the nest control parameters, are all just
data that you can build up or pull in from anywhere. Compare with traditional
control structures used to manipulate data which exist inside the source code
only and are not mutable at runtime.

Or use `deep` and `wide` for more explicit macro-control of
recursion. 

### Erlang-inspired mailboxes

Thread safe for both readers and writers (at some cost to performance of
course). Fast enough to be usable for basic purposes (800 request/reply cycles
a second on a $5 Digital Ocean box). 

```
0. [] Mbox.new as 'myservice;
1. myservice Mbox.watch {x show}
2. myservice Mbox.send 55;
55
```

`watch` spawns a thread to act on mailbox messages. The `55` seen here is from
the `show` statement in the lambda. A `y` variable is available inside the
callback to maintain state; it starts as [] and it set to whatever your
function returns.

See `doc/sect_mbox.xxl` for more.

### Tables

Well, the beginnings of them anyway.

```
0. "name,age,job\nBob,30,Programmer\nJane,25,CFO\nTyler,2,Dinosaur Hunter" as 'data
1. data split "\n" each {split ","} as 'fields      // in a fantasy world where csv is never escaped
2. fields first:(fields behead) as 'emp             // : creates dicts or tables 
3. emp
```

Still no joins, many rough edges, untested performance.

### Other

- Variable names can't contain numbers, so you can build up some pretty clean and
  short expressions that mirror mathematics. `3u b5` means `3 * u b 5`, assuming
	u is a single argument (unary) function and b is a two argument (binary) function.
	If you're recoiling in horror right now at losing your precious numerals in variable
	names, consider how often you're just poorly naming a temp variable.
- Variable names can contain `?`, so you can name your predicate functions in a
	pleasant manner.
- Values can have "tags" associated with them, allowing you to create an
	OOP-like concept of structure within data, while all regular operations work
	seamlessly as if you were using the underlying data type. Consider the
	classic OOP example of a "point", which in XXL would just be a tagged int
	vector like `'point#(100,150)`.
- Vector-oriented and convenient manipulation on primitive values. For instance,
  `1,2,3 * (4,5,6)` is perfectly legal and returns 4,10,18. This is a huge time saver
	and eliminates many loops. It's also pretty fast, which somewhat makes up for the
	dangerously slow interpreter.
- Diverse integer type options, including 128bit octaword (`1 make 'octa`).
- Fast-enough unboxed binary data files. 50 million ints into 201MB output file on $5/month
  Digital Ocean SSD via `1024*1024*50 count Xd.set "/tmp/50m.xd"`
- Threading support, kinda. XXL has the notion of threads available internally, but 
  the only way they are usable from inside XXL code is via `mb Mbox.watch {..}`.
	This is because modifications to contexts' data items are not locked or synchronized,
	So chaos will surely ensue if you modify global data from threads. Thus, the mailbox
	acts as both a safe communication mechanism for threads, and a way to discourage global
	state mutation.
- Built in simple networking. Client-speaks-first protocols are a snap to implement.
  World's easiest echo server: `[8888,""]Net.bind{"Echo: ",x}`. Net.bind creates one
	thread to service each port/service.
- Vim syntax file in tools/syntax/vim-syntax.vim (which is generated by vim-build.xxl
  in that same directory).
- No stinkin loops (and no linked lists, either)
- Supports `\\` to exit the REPL, as god intended (`quit` and `exit` too)
- BSD license

## Not yet implemented

XXL is still very much a work in progress and is mostly broken. That said, here are the 
major features I anticipate finishing soon-ish.

- Currently has severe memory leaks and performance problems
- ~~Floats!~~ We've got floats now - no comparison tolerance yet tho.
- ~~Dictionary literals (dictionaries do work and exist as a primitive type, just
	can't decide on a literal syntax for them)~~
	Settled on and implemented `['a:1,'b:2]` by creating `:` as the make-dict operator
	and making `,` smart about dicts.
- JSON
- FancyRepl(tm)
- Dates/times (need to figure out core representation and a way to express them as literals
in code..)
- ~~In-memory tables~~ Few operations yet, and bugs remain, but the notion of tables has
  slowly seeped in. See tests.
- ~~Non-pointer tags implementation to avoid logic to enc/decode to binary for disk and IPC
  ~~ Temporarily replaced tag representation with 128bit pointer string. Fast to compare,
	almost as fast as possible to create (memcpy), zero overhead on IPC ingest/excrete, 
	not terribly large on today's memory sizes.
- I/O (~~files~~, ~~sockets~~, mmap)
- Logged updates (I like [Kdb's approach to this](http://code.kx.com/wiki/Cookbook/Logging))
- ~~Mailboxes/processes (implemented as a writer-blocks general list)~~ Available in "Mbox"
  class. 
- Streams, laziness (perhaps based on mailboxes? studying other systems now)
- Tail call optimization in functions using the `self` keyword 
	[as per Kuc's approach](https://github.com/zholos/kuc/blob/1cace4608ba0398de6054349abea9b97100386cd/func.c#L726)
- Sorted vectors
- Grouped/index types
- Well-supported Apter trees ([see also APL's approach](https://dfns.dyalog.com/n_Trees.htm)).

## Well known bugs

Work in progress on these:

- Interpreter speed and memory leaks

- The join verb (`,`) is still finnicky about joining like-types of data with
	general lists. In particular, I often find myself a bit puzzled by results
	like `['a,2],3` - should this be a two-item list with two items in the first
	sublist, or a three element list? Before you answer, consider `['a,2] as
	'q;q,3`.
	When in doubt, build parts separately and combine.

## Open questions

- Date and time stamps: Nanosecond precision from uint64 sufficient? What about literal
  representation? Can we use `1997.12.31.09.31.45.333` or something non-pollutey like that?
	Need to patch up apply() to allow you to index `'day` and similar.

## Maybe later

- JS/Emscripten (both in terms of Node.js interop and use of XXL in the browser)
- LLVM IR 
- GUI

## Probably not

- Unicode

## Installation

Pretty rough right now. You'll need a C compiler and a minimally POSIX
environment but that's about it. 

We use a build script called `./c` instead of a
Makefile, just to be difficult. 

Some of the .h files in the source are automatically generated by some
primitive .js scripts. If you don't have Node installed, or don't care to
rebuild those (it's not necessary unless you change the definitions of the
built in data types), just comment out "BUILDH" at the top of the build script.

Try something like:

```
git clone https://github.com/tlack/xxl.git
cd xxl
./c
```

The default build has a TON of debugging info turned on which it will violently
spit at you while you use it. Commented out the "DEBUG=" line in `./c` to make the
system more silent and friendly - but perhaps less predictable when things go
wrong (which they will.. often).

## Size

XXL is about 3,000 lines of hand-written C, plus 2,000 lines of auto-generated
.h files. Stripped executable is about 400kb.

I dislike large systems and aim to keep XXL small, but I also want programmers
to be able to easily get at functionality they need, both through a decent set
of built-ins, and through something akin to npm (still pondering this).

I believe XXL could be reduced to 1,000 lines or less of JavaScript or another
language that offers a more flexible type system than C's. I also wrote different
versions of similar code a lot for speed; implementing everything in terms
of each() (and other forms of iteration) would probably require much less code.

## Inspiration

K4/Q by [Kx Systems&trade;](http://kx.com), the quirky
[Klong by Nils Holm](http://t3x.org/klong/), Erlang
(process model, introspection, parse tree/transforms), 
[Kerf's approachablity](http://kerfsoftware.com), 
Io (self-similarity of objects and asceticism) 
and C (simplicity, performance, rectangularity of data).

## Disclaimer

No warranty. Trademarks right of their respective owners. 

This project is not related in any way to the interesting [XL project by
Dinechin et al](http://xlr.sourceforge.net/).

## License

BSD-L (3 clause)

## Credits

@tlack

Built at [Building.co](http://building.co).

## Contact

@tlack

