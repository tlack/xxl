Implementing XXL
================

In a [thread on Hacker News](https://news.ycombinator.com/item?id=11379487),
someone mentioned wanting to implement something like XXL in Javascript.

This idea specifically has occurred to me and is of great interest but the C
implementation is more important right now and I'm already woefully behind.

I say "like XXL" because I think some of the ideas may be valid outside of the
tiny stab at it I've made. So maybe these ideas can be used elsewhere in
different forms. I'm totally OK with that. I think that is how ideas are
supposed to work, and XXL is mainly an idea.

More generally I thought it might be interesting to describe how XXL works in
case other care. Plus I'm currently stuck in one of those nightmarish recursive
"I can't produce usable code right now" bug spirals, so maybe writing would be
a worthwhile and productive diversion.

In some of the examples, I'll be referring to Javascript, but the ideas are
applicable anywhere, or in other languages.

## Trickery in all forms

I've always had trouble producing parsers that I'm happy with. It's not because
I just can't figure out how they work gash darnut. It's because the waterfall
of `if` statements drives me nuts. That's not supposed to be how you express
your intent. It may be fast, or it may be common, but it's not a good use of
our time writing or reading.

Parser combinators are a start, but they seem difficult to express concisely in
most languages. The most interesting of the quasi-parser libraries that's
available to us mortals is [OMeta](http://www.tinlizzie.org/ometa/) but I can't
quite tell if it's a spec or a style or a library or a general idea.

There had to be a shortcut! I found it in the idea of the [Nanopass
Compiler](http://www.cs.indiana.edu/~dyb/pubs/nano-jfp.pdf), which simply
builds an intermediate tree representation that starts out with the literal
characters you typed in and applies successive passes to tease out the final
form.

What's great about the concept of "successive passes" is that you can build the
"tree" of these passes -- the logic behind them, how they combine, what they do
next -- with other code and data. 

You're basically abstracting the rigid hard coded `if` statements into a
data-driven metaprogram of sorts, and using that to transform data in a
progressive way. To change those rules, you just change the source data.

This is how XXL works in a vague sense.

## Warning

This lazy technique is pretty slow, and I'm not sure if it can be made to be
very fast at all. I haven't given up of course, but if you seek true speed, you
may need to use a lower level representation of the parse tree, or one that can
easily feed a JIT compiler.

## Backend

I chose C to implement because I stood the best chance of making it work and
hitting an OK place performance wise. I don't think there's anything in XXL
that necessitates C so there is probably a lot of leeway to generate LLVM,
JS, etc.

## Fundamentals

1. Values (variables)

2. Contexts. A context is a thing that contains values which have been given
	 names; I used a dictionary, and it may contain a value called parent which
	 is its parent contexts dictionary. It's kind of like a stack frame, except a
	 lambda often has a context attached, so in xxl.c we think of them as a
	 single unit.

3. Verbs. Blobs of code. One or two args. Zero args or 2+ not yet implemented.

4. Projections. These are basically a list of a verb which has been partially
	 applied. For instance: `[*,2]`. You then apply it with 3 and get 6 back.

## Values and types

An implementation of XXL needs two classes of types:

1. Vector types, which are variables containing 0 or more values of a certain
	 type. For instance, a vector of characters is a string.  All of the values
	 inside a vector variable are of the same type, and they're stored in a
	 space-efficient way that makes it fast to access individual items. There are
	 7 of them at the time I write this: tag, byte, char, int, long, octa, float. 

2. List types, which are collections of vectors. They're stored as a vector of
	 pointers in C. More complex types are usually built out of lists. These list
	 types need to contain other lists. In most cases this should be pretty easy
	 in object-oriented languages.

If one was implementing this in Javascript, I'd probably use an
fixed-size array for the vector types and implement the rest as
an array with a header indicating the type.

You can see the current types defined for your copy of XXL inside
`common.js`.

## Tools

The approach I took was to build out some ideas about how XXL's
matching/parsing verbs might work, and then build the parser using those. That
way, as I build the parser, I am also developing the core of XXL.

You can use a different approach, but these are the tools I needed. This is a
highly abbreviated list, mentioning only the parts that are crucial to building
a parser using this style.

### Vectors and lists

* Need a way to append to a value, either list or vector
* Need something like `splice` that will update items in the middle of a list by replacing
them with other items. This can be used to create sub-lists inside a list.

### Searching

* Need something like `match` that can find an item inside a vector or list.

### Lexing with `matchexec`

`matchexec` scans the list for a given class of literals, using a list of
allowed character values, and then splits them into groups and calls your
callback.

[Here's the implementation](https://github.com/tlack/xxl/blob/a7fe0c06b4151bfcafb0edc2f8cdc03f81367220/xxl.c#L3242)

### Grouping with `nest`

You'll need something like `data nest ["(",")",{`parens#x}]`, which is a function that looks
through a list and finds matching pairs of an opening and closing delimeter, being intelligent
about nested sets, and applies a function you apply to them, replacing the original items
with the result of this function.

Sometimes the opening and closing delimeters are the same. In my experience that requires
a different set of logic.

[Here's my implementation](https://github.com/tlack/xxl/blob/a7fe0c06b4151bfcafb0edc2f8cdc03f81367220/xxl.c#L3094).

It's pretty ugly. I used a K-inspired boolean representation of the predicate
functions' output, and then used binary logic to combine the states into an
output state. You can use your own approach. 

### A note on regular expressions

I didn't want to tie XXL to a regex engine, especially with complex bactracking behavior.
My first while with XXL was spent trying to build my own regex-inspired open ended
`match` verb, that would take a description of what you wanted to match on the right,
and perform some specified behavior, including calling callbacks.

In theory, this would obviate `nest` and a whole host of other functions, but I
found it difficult to make work reliably. You may have better luck and I
encourage exploration of the idea.

### Getting deep in nested lists

I created two verbs `deep` and `wide`, which correspond to depth first traversal
and breadth first traversal of a tree. In this case, the tree is a list with
other lists inside it, but the concept is the same. 

`data wide {code...}` for instance calls `code` on data, then `wide` on each item
of the resulting value, and so on. 

`data deep {code...}` looks for a list inside data. If there is one, it calls
`deep` again on it. If not, it calls `code`, and returns the result.

I use this to deeply create structure in lists. You can use a different approach

## A note about whitespace

Please for the sake of humanity don't make white space significant if you take
a crack at an implementation of something XXLy. Let's create a future where no
one cares about line endings. Think of the device form factors. For me, please.

## The simplest possible parse tree

XXL is intended to be very simple to use in a meta-programming fashion,
where you build your code logic yourself rather than creating very literal functions.

Here's an example of a fully valid parse tree:

```
0. [2,*,7] evalin .
14
```

Keep that in mind as you begin your work at understanding this next part.

## Creating the parse tree

You can see how this works inside the [parse family of functions in xxl.c]
(https://github.com/tlack/xxl/blob/a7fe0c06b4151bfcafb0edc2f8cdc03f81367220/xxl.c#L3539).

1. Start with the string to parse. Explode it into a list of individual characters.

2. Pull out the groups that require no further parsing. These are comments `//`
	 and `/*`..`*/`, and strings `"`..`"`. You should probably also do triple
	 quoted strings here, though I have not. 
	
```
	acc=nest(acc,xln(5, entags(xfroms("\""),"raw"), xfroms("\""), xfroms("\\"), Tt(string), x1(&parsestrlit)));
	acc=nest(acc,xln(5, entags(xfroms("//"),"raw"), xfroms("\n"), xfroms(""), Tt(comment), x1(&parsecomment)));
	acc=nest(acc,xln(5, entags(xfroms("/*"),"raw"), xfroms("*/"), xfroms(""), Tt(comment), x1(&parsecomment)));
```

I.. ahem.. uhh.. unrolled this loop for performance. It's obvious how to make this
table driven.

3. Use `matchexec` (here wrapped with `mklexer`) to combine groups. Each group has its own
function to verify the contents of the extracted sigil, do further processing, etc. I'll
omit those for now.

```
	lex=mklexer("0123456789.","num");
	append(pats,ELl(lex,0));
	append(pats,x1(&parsenum));
	xfree(lex);
	lex=mklexer("'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_.?","name");
	append(pats,ELl(lex,0));
	append(pats,x1(&parsename));
	xfree(lex);
	lex=mklexer(" \n\t","ws");
	append(pats,ELl(lex,0));
	append(pats,ELl(lex,1));
	xfree(lex);
	t1=matchexec(acc,pats);
```

4. We take the liberty in these callbacks to replace things like numbers with
	 their literal values.  At this point, we're sort of pre-parsing some of
	 these, which makes sense because the interpreter is a bit slow, so any
	 front-loaded work we can do here will benefit us every additional time we
	 evaluate the code.. which may be millions of times.

4. Form groups using `nest` again, this time for `{`..`}`, `[`..`]`, `(`..`)`. This should be done
recursively, because they may be deeply nested, and you don't know what the precedence order
might be. I use `wide` for this.

```
	t2=wide(t2,x1(&parseallexprs));
```

which is:

```
VP parseallexprs(VP tree) {
	VP brace=xfroms("{"), paren=xfroms("("), bracket=xfroms("[");
	if(_find1(tree,brace)!=-1)
		tree=nest(tree,xln(5, entags(brace,"raw"), entags(xfroms("}"),"raw"), xfroms(""), Tt(lambda), x1(&parselambda)));
	if(_find1(tree,paren)!=-1)
		tree=nest(tree,xln(5, entags(paren,"raw"), entags(xfroms(")"),"raw"), xfroms(""), Tt(expr), x1(&parseexpr)));
	if(_find1(tree,bracket)!=-1)
		tree=nest(tree,xln(5, entags(bracket,"raw"), entags(xfroms("]"),"raw"), xfroms(""), Tt(listexpr), x1(&parseexpr)));
	return tree;
}
```

Since these functions get called a lot (recursively) while parsing, you may
want to memorize some of these raw values we create here, like `brace`.

## Example parse tree

Here's an example from XXL's C implementation:

```
0. "2 {x * 7 /* math! */ as 'q; [q,4]}" parse evalin .
(0.0184 sec)
inputs@0: "2 {x * 7 /* math! */ as 'q; [q,4]}" parse evalin .
outputs@0:
[14, 4]

1. "2 {x * 7 /* math! */ as 'q; [q,4]}" parse
(0.0091 sec)
inputs@1: "2 {x * 7 /* math! */ as 'q; [q,4]}" parse
outputs@1:
[2, 'ws#" ", 'lambda#[['name#"x", 'ws#" ", 'raw#"*", 'ws#" ", 7, 'ws#" ",
'comment#"/* math! */", 'ws#" ", 'name#"as", 'ws#" ", 'q, 'raw#";", 'ws#" ",
'listexpr#[[], 'raw#",", 'name#"q", 'raw#",", 4]], 1], 'ws#"\n"]
```

As you can see we take a lot of little steps, but the logic is really simple.

## Evaluate it

Maintain a stack of "left values."

Initally, the left value is set to x, or whatever the interpreter was called
with as an `x` value - possibly null.

As you pull out each item of the parse tree, do something akin to the
following:

1. Take `item` from beginning of parse tree and shift parse tree left by one
	 item.

2. If this is a list, it's some kind of grouped value - recurse. This covers
	 `[`..`]` and the sort. If it's `{`..`}`, you'll need to create a new context
	 as this is a literal function definition. This is the new value of `item`.

3. If this is a literal name, or a "raw" value (which is probably a literal
	 name), look it up in the current context. We use a simpler version of 
	 `get` called `lookup` for this.

4. Now we examine what's on the top of the left value stack.

5. If it's callable, meaning a lambda, or a projection, let's apply it with
	 this `item` value.

6. If it's callable, and it is a binary verb, meaning it requires two
	 arguments, put a new projection of `[verb,item]` on top of the left item
	 stack.

7. Otherwise, put `item` on the stack.

Pretty simple. For speed's sake, steps 1 and 5 should be disconnected, so that
you can easily call binary verbs by immediately getting the next item out of
the parse tree, rather than creating the intermediary projection, which is
slow.

## A special case

In the code `3 {x*y} as 'proj`, how does XXL know to not call `[3,as] from
{x*y}`? Right now, 'as' and 'is' are special cases handled in step 3. There is
some logic that could be used to avoid this, utilizing the concept of "active"
and "inert" values, but this is a simpler approach for now.

## More on `as` and `is`

XXL has the concept of a "current context", stored in XXL_CUR_CTX. This way,
`as`, `is`, and `lookup` know where to get/set values.

Another approach might be an earlier one taken by XXL, where step 3 above
decodes `as` as a projection like `[set,cur_ctx]`, and the symbol, when
encountered in the next position in the parse tree, is applied to this
projection.

