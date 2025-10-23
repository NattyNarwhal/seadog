# Compact DAWGs

This generates a compact representation of a directed acyclic word graph, a
data structure, like a trie, useful for looking up words in a dictionary.
Sometimes a DAWG is also called a suffix automaton or minimum acyclic finite
state automation, because unlike a trie, its nodes have been compacted to
avoid identical branches, saving space.

In particular, this repo includes:

* A tool to generate the DAWG; this is based on a [Python script][hanov1]
  written by Steve Hanov, extended to write out said compact DAWG.
* A C library that reads said compact DAWG; this is written in portable C89
  and should work on anything with standard C file I/O and 8 bit bytes.
* A small test corpus to test generation and reading with.

You can run `make check` to build the test program, a sample corpus, and runs
some queries on said corpus. You can also take `dawg.py` to build a DAWG on a
dictionary, and embed `dawg.[ch]` to use it from a C program.

## Format details

The DAWG, after being reduced, is stored in a form where each node is stored
as a list of its edges, similar to what is described by [Wutka][wutka]. Each
edge is packed into a word; the size of the word is 2-4 bytes, depending on
the minimal size needed to index the first edge of a node. The lowest 5 bits
of the word are used for the character index (this is perhaps overfitted for
English dictionaries, by subtracing 0x60, currently no character remapping),
with an additional bit for end of word, another for end of list (when reading
the node edge list), and the rest of the 9 to 25 bits for the index of the
node in words.

The dictionary used to build a DAWG will have cases crushed to lower case and
any non-alphabetical characters will be stripped.

The words are stored little endian. The first word is a "null" word, with the
final and EOL bits set, and the character index used for indicating the word
size. The second word is the index of the root node, where lookups start.
Because the first word is stored little endian, you can read the first byte,
mask for the character, and determine the word size for future index reads.

Because the nodes are a fixed size, indexing is just a matter of multiplying
the index by the word size, so there's no need for parsing the whole structure
or needing an index of node positions.

```
+---------+---------+---------+---------+
|iiii iiii|iiii iiii|iiii iiii|iEFc cccc|
+---------+---------+---------+---------+
^         ^         ^          ^^^   ^
|         |         |          |||   |
|         |         |          |||    \- 5 bit character
|         |         |          || \----- word termination
|         |         |          | \------ list termination
|         |         |           \------- 9/17/25 bit index
|         |         |
|         |          \------------------ two byte words (edges < 0x1FFF)
|         |
|          \---------------------------- three byte words (edges < 0x1FFFFF)
|
 \-------------------------------------- four byte words
```

### Size

The resulting DAWG is surprisingly efficient. Despite not being as tightly
packed as possible, it gets favourable results compared to the bit-level
packing of [Hanov's Go DAWG library][hanov2]; my suspicions are overfitting
this for 5 bits (necessitating crushing case and excluding symbols) means we
can save space, as the bit level DAWG does not remap, just uses the minimal
amount of bits for existing ASCII. It also means that reading the DAWG is much
simpler; the lookup function fits on the back of a pop can.

Using the corpus from James Cherry's anagram program and SOWPODS:

```
216K anagram.pup
860K anagram.txt
290K anagram.txt.dawg
725K sowpods.pup
2.6M sowpods.txt
778K sowpods.txt.dawg
```

## Possible optimizations and changes

* Include node/edge/word count: Edge count in index seems possible to include
  in the first word, could add additional words at end for other info.
* Character mappings, to make it possible to use more than just `[a-z]`, or a
  means to widen the character address space at the cost of the index size.
* Variable length words: Would complicate indexing without a lot of additional
  parsing that having fixed length words avoids.
* Relative indexing: Might be able to reduce index address space, but possible
  the gain is neglible.
* Indexing into the middle of nodes to save space: Sounds tricky.

[hanov1]: https://stevehanov.ca/blog/index.php?id=115
[hanov2]: https://github.com/smhanov/dawg
[wutka]: http://www.wutka.com/dawg.html
