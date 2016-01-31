![XXL logo](http://www.modernmethod.com/send/files/xxl-logo.gif)

# XXL, a small language

## Status

Gross, crumbly work in progress. Kinda works when it compiles, but mostly a
proof of concept. Definitely *not suitable* for serious work just yet. See also
"not yet implemented" below.

## Examples

Let's dive in with a simple prime number tester (returns 0 if not prime, >0
otherwise):

```
17 {til drop 2 % x}
```

Here's how it works:

`17` is the number 17, obviously, used as an example here.

`{..}` marks an anonymous function (also called a closure or lambda).

`til` returns the numbers from 0 til its left argument. Note that in this
case we're using the "implied" x variable that is silently prepended to
all lines. This is equivalent to saying `x til`. In our example with 17,
the result would be `(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16i)`.

`drop 2` removes the first two items - 0 and 1 in this case. Our expression
currently stands at `(2,3,4,5,6,7,8,8,9,10,11,12,13,14,15,16i)`.

`% x` invokes the "mod" operator, returning the integer remainder of y divided
by x. This part highlights an interesting and important detail of XXL that
separates it from more popular languages: most simple operators, like the math
functions `+ - / * % | &` can work with one or more numbers on both sides of
them. This is a big time saver compared to writing endless horrible loops in
other languages.

So with `(result) % x` we're getting the remainder of that whole list, divided
by our target number, `17`. 

This yields the uninteresting `(2,3,4,5,6,7,8,9,10,11,12,13,14,15,16i)`, which
is what we would expect: nothing *evenly* divides into 17, or else it wouldn't
be prime. 

If you try it with a different sequence, you'll see why this works:

```
9 til drop 2 % 9
(1,0,1,4,3,2,1i)
```

As you can say, 3 cleanly divides 9, so the value in that position is zero.

`min` then allows us to find the lowest number in this sequence. If it's 
zero, that means that the target number can be evenly divided by one of
the numbers lower than it, and thus, it is not prime. Tada!

See the various tests for more examples. 

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

## Inspiration

K4/Q by [Kx Systems&trade;](http://kx.com), 
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
	`( )` and `{ }`)
- Very fast operations on values, especially large arrays (pretty slow parser, though)
- Vector-oriented and convenient operation on primitives 
- Diverse integer type options, including 128bit octoword (denoted with `o`).
- Threading support, kinda (requires attention)
- Values can have `tags` associated with them, allowing you to create an
	OOP-like concept of structure within data, while all regular operations work
	seamlessly as if you were using the underlying data type.
- BSD license
- Built in web server (half way there at least)
- Very small
- No stinkin loops (and no linked lists, either)
- Supports `\\` to exit the REPL, as god intended (`quit` and `exit` too)

## Not yet implemented

XXL is still very much a work in progress and is mostly broken. That said, here are the 
major features I anticipate finishing soon-ish.

- Currently has severe memory leaks 
- Floats! 
- Dictionary literals (dictionaries do work and exist as a primitive type, just
	can't decide on a literal syntax for them)
- JSON
- FancyRepl(tm)
- Dates/times (need to figure out core representation)
- In-memory tables
- I/O (sockets, mmap)
- Logged updates (I like [Kdb's approach to this](http://code.kx.com/wiki/Cookbook/Logging))
- Mailboxes/processes (implemented as a writer-blocks general list)
- Streams (perhaps a mailbox as well; studying other systems now)

## Open questions

- What syntax for `each`, `eachleft`, `eachright`, `eachboth`, and `eachpair`?
  Experimenting with `@` at this time, but open to suggestions.

- What's the best way to represent dates/times and time spans? 

- Should our tag/symbol implementation work with ints or char pointers? 

## Maybe later

- JS/Emscripten (both in terms of Node.js interop and use of XXL in the browser)
- LLVM IR 
- GUI

## Probably not

- proper unicode

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

XXL is about 3,000 lines of hand-written C, plus 2000 lines of auto-generated
.h files. Stripped executable is about 400kb.

I dislike large systems and aim to keep XXL small, though I would like to make
code sharing easy and convenient ala npm (though different in many respects).

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

