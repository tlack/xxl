![XXL logo](http://www.modernmethod.com/send/files/xxl-logo-2.png)

# What is XXL?

XXL is a new programming language whose goals are simplicity, power, and speed.
We throw out most of conventional programming language thinking in order to
attain these goals.

## Status

Gross, crumbly work in progress. Kinda works when it compiles, but mostly a
proof of concept. Definitely *not suitable* for serious work just yet. See also
"not yet implemented" below.

Tested so far on Linux (x86/GCC), OS X (Clang), Windows (Cygwin64). Soliciting
Android runtime on NDK (contact me if interested).

## Examples

Remember prime numbers? Those are numbers that can't be divided by anything
else without there being a remainder (a fraction) except for 1. For instance,
you can't perfectly divide 7 by anything other than 1 and 7. 

Stick with me. Prime numbers may not be very exciting, but it makes a good
short example for XXL's purposes.

How can a computer figure out if a number is prime? Let's dive in with a simple
prime number tester. This code will return 1 when a number is prime, and 0
if the number is not prime. 

```
xxl>7 count drop 2 % 7 min not
0
xxl>9 count drop 2 % 9 min not
1
```

Don't freak out. Read it word by word, starting from the first word on the left.
XXL code is just a mix of nouns (numbers, variables, other kinds of values) and 
verbs (functions, operators). Every verb takes either one or two arguments.
Unlike other languages, you never use unwiedly long argument lists to functions.
Here's how to read this code.

We're going to go about this by figuring out if any other number below this
number can evenly divide it.

`7` is the number 7, obviously, used as an example here.

`count` returns the range of numbers from 0 to its left argument. In our
example with 7, the result would be `(0,1,2,3,4,5,6i)`.

`drop 2` removes the first two items from that set of numbers. We do this because
dividing by zero is naughty and because anything can be divided by 1. Now we
have `2,3,4,5,6i` built up as our argument to the next verb.

`% 7 invokes the "mod" operator, returning the integer remainder of x divided
by y. This part highlights an interesting and important detail of XXL that
separates it many common programming languages: most simple operators, like the math
functions `+ - / * % | &` can work with one or more numbers on both sides of
them. The result will be the same size as the larger of the two parameters (x or y).
This is a big time saver compared to writing endless horrible loops in
other languages.

So with `(result) % 7` we're getting the remainder of that whole list, divided
by our target number, `7`. 

This yields the uninteresting `(2,3,4,5,6i)`, which
is what we would expect: nothing *evenly* divides into 17, or else it wouldn't
be prime.

If you try it with a different sequence, you'll see why this works:

```
9 count drop 2 % 3
(1,0,1,4,3,2,1i)
```

As you can say, 3 cleanly divides 9, so the value in that position is zero.

`min` then allows us to find the lowest number in this sequence. If it's 
zero, that means that the target number can be evenly divided by one of
the numbers lower than it, and thus, it is not prime. 

But of course, we want our function to return the opposite - 1 if it's
prime, not 0. So we run it through `not`.

You'd normally give this function a name and call it more easily:

```
{x drop 2 % x min not} as 'prime?; 30 prime?, 31 prime?
```

## Micro Web Counter

Here's an example web server application that acts as a counter. You can run
this as`./c examples/web-ctr.xxl`. Source code in full:

```
0 as '.ctr;
(8080,"localhost").net.bind{
	x show; .ctr + 1 as '.ctr;
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
`test-ctx.h` and `test-logic.h`.

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

## Inspiration

K4/Q by [Kx Systems&trade;](http://kx.com), the quirky
[Klong by Nils Holm](http://t3x.org/klong/), Erlang
(process model, introspection, parse tree/transforms), 
[Kerf's approachablity](http://kerfsoftware.com), 
Io (self-similarity of
objects) and C (simplicity, performance, rectangularity of data).

## Motivation

I need a swiss army knife for data manipulation with predictable performance and concise
syntax.

## Features

- Minimalist syntax. Clean, very easy to understand and parse left-to-right
	syntax with only three special forms: comments, strings, and grouping (i.e.,
	`( )`, `[ ]` and `{ }`)
- Agnostic about whitespace. Don't let invisible special characters ruin your day.
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
	vector like `'point(100,150)`
- Very fast operations on arrays of values, especially large ones (don't get
	too excited - the parser is pretty slow)
- Vector-oriented and convenient manipulation on primitive values. For instance,
  `1,2,3 * (4,5,6)` is perfectly legal and returns 4,10,18. This is a huge time saver
	and eliminates many loops.
- Diverse integer type options, including 128bit octoword (denoted with `o`).
- Threading support, kinda (requires attention)
- Built in web server (half way there at least)
- No stinkin loops (and no linked lists, either)
- Supports `\\` to exit the REPL, as god intended (`quit` and `exit` too)
- BSD license

## Not yet implemented

XXL is still very much a work in progress and is mostly broken. That said, here are the 
major features I anticipate finishing soon-ish.

- Currently has severe memory leaks 
- ~~Floats!~~ We've got floats now - no comparison tolerance yet tho.
- ~~Dictionary literals (dictionaries do work and exist as a primitive type, just
	can't decide on a literal syntax for them)~~
	Settled on and implemented `['a:1,'b:2]` by creating `:` as the make-dict operator
	and making `,` smart about dicts.
- JSON
- FancyRepl(tm)
- Dates/times (need to figure out core representation and a way to express them as literals
in code..)
- In-memory tables
- Non-pointer tags implementation to avoid logic to enc/decode to binary for disk and IPC
- I/O (~~sockets~~, mmap) (try `(8080,"localhost").net.bind{"hello world!"}`)
- Logged updates (I like [Kdb's approach to this](http://code.kx.com/wiki/Cookbook/Logging))
- Mailboxes/processes (implemented as a writer-blocks general list)
- Streams (perhaps a mailbox as well; studying other systems now)
- Tail call optimization in functions using the `self` keyword as 
	[as per Kuc's approach](https://github.com/zholos/kuc/blob/1cace4608ba0398de6054349abea9b97100386cd/func.c#L726)
- Sorted vectors

## Well known bugs

Work in progress on these:

- Comments have some decent tests, but the comment parser/lexer still seems to get confused
in longer code samples. 

- The join verb (`,`) is still finnicky about joining like-types of data with
	general lists. In particular, I often find myself a bit puzzled by results
	like `['a,2],3` - should this be a two-item list with two items in the first
	sublist, or a three element list? Before you answer, consider `['a,2] as
	'q;q,3`.

## Open questions

- Date and time stamps: Nanosecond precision from uint64 sufficient? What about literal
  representation? Can we use `1997.12.31.09.31.45.333` or something non-pollutey like that?
	Need to patch up apply() to allow you to index `'day` and similar.

- What syntax for `each`, `eachleft`, `eachright`, `eachboth`, and `eachpair`?
  Experimenting with `@` at this time, but open to suggestions.

- What's the best way to represent dates/times and time spans? What about datetime literals in
  source code? 

- Should our tag/symbol implementation work with ints or char pointers? 

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

