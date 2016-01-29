![XXL logo](http://www.modernmethod.com/send/files/xxl-logo.gif)

# XXL, a small language

## Status

Kinda works. Usually starts at least. Definitely not suitable for real use. See
"Missing features" below

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
by x. So we're getting the remainder of that whole list, divided by our target
number, `17`. 

This yields the uninteresting `(2,3,4,5,6,7,8,9,10,11,12,13,14,15,16i)`, which
is what we would expect: nothing *evenly* divides into 17, or else it wouldn't
be prime. 

If you try it with a different sequence, you'll see why this works:

```
9 til drop 2 % 9
(1,0,1,4,3,2,1i)
```

This part highlights an interesting and important detail of XXL: most simple
operators, like the math functions `+ - / * % | &` can work with one or
more numbers on both sides of them. This is a big time saver compared to
writing endless horrible loops in other languages.

`min` then allows us to find the lowest number in this sequence. If it's 
zero, that means that the target number can be evenly divided by one of
the numbers lower than it, and thus, it is not prime. Tada!

## Features

- Minimalist syntax. Clean, very easy to understand and parse left-to-right syntax with only three special forms:
comments, strings, and grouping (i.e., `( )` and `{ }`)
- Very fast operations on values, especially large arrays (pretty slow parser, though)
- Vector-oriented and convenient operation on primitives 
- Supports `\\` to exit the REPL, as god intended (`quit` and `exit` too)
- Diverse integer type options, including 128bit octoword (denoted with `o`).
- Threading support, kinda (requires attention)
- BSD license
- Built in web server (half way there at least)
- Very small

## Not yet implemented

XXL is still very much a work in progress and is mostly broken. That said, here are the 
major features I anticipate finishing soon-ish.

- FancyRepl(tm)
- Dates/times
- In-memory tables
- I/O (sockets, mmap)
- Logged updates
- Streams
- Mailboxes/processes

## Maybe later

- LLVM IR 
- GUI

## Installation

Pretty rough right now. We use a build script called `./c` instead of a
Makefile, just to be difficult. You'll need a C compiler and a minimally POSIX
environment but that's about it. Try something like:

```
git clone https://github.com/tlack/xxl.git
cd xxl
./c
```

## Inspiration

K4/Q by Kx Systems, Erlang (process model), and C.

## Size

XXL is about 3,000 lines of hand-written C, plus 2000 lines of auto-generated
.h files. Stripped executable is about 400kb.

I dislike large systems and aim to keep XXL small, though I would like to make
code sharing easy and convenient ala npm (though different in many respects).

## License

BSD-L (3 clause)

## Credits

@tlack

Built at [Building.co](http://building.co).

## Contact

@tlack

