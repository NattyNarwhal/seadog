#!/usr/bin/python3
# By Steve Hanov, 2011. Released to the public domain.
# Please see http://stevehanov.ca/blog/index.php?id=115 for the accompanying article.
#
# Based on Daciuk, Jan, et al. "Incremental construction of minimal acyclic finite-state automata." 
# Computational linguistics 26.1 (2000): 3-16.
#
# Updated 2014 to use DAWG as a mapping; see 
# Kowaltowski, T.; CL. Lucchesi (1993), "Applications of finite automata representing large vocabularies", 
# Software-Practice and Experience 1993
#
# Modified by Calvin Buckley in 2025 to write out a compact DAWG w/o mapping, see
# http://www.wutka.com/dawg.html
import sys
import time
import struct
import argparse
import re

parser = argparse.ArgumentParser(prog="dawg",
                                 description="Generate a directed acyclic word graph in a compact form")
parser.add_argument("filename")
parser.add_argument("-m", "--minimum", type=int, help="Minimum length of a word", default=1)
parser.add_argument("-M", "--maximum", type=int, help="Maximum length of a word", default=32)
parser.add_argument("-o", "--output", help="Output to the file (.pup recommended extension)")
args = parser.parse_args()

letters = re.compile(r"^[a-z]+$")

# This class represents a node in the directed acyclic word graph (DAWG). It
# has a list of edges to other nodes. It has functions for testing whether it
# is equivalent to another node. Nodes are equivalent if they have identical
# edges, and each identical edge leads to identical states. The __hash__ and
# __eq__ functions allow it to be used as a key in a python dictionary.
class DawgNode:
    NextId = 0
    
    def __init__(self):
        self.id = DawgNode.NextId
        DawgNode.NextId += 1
        self.final = False
        self.edges = {}

        # Number of end nodes reachable from this one.
        self.count = 0

    def __str__(self):        
        arr = []
        if self.final: 
            arr.append("1")
        else:
            arr.append("0")

        for (label, node) in self.edges.items():
            arr.append( label )
            arr.append( str( node.id ) )

        return "_".join(arr)

    def __hash__(self):
        return self.__str__().__hash__()

    def __eq__(self, other):
        return self.__str__() == other.__str__()

    def numReachable(self):
        # if a count is already assigned, return it
        if self.count: return self.count

        # count the number of final nodes that are reachable from this one.
        # including self
        count = 0
        if self.final: count += 1
        for label, node in self.edges.items():
            count += node.numReachable()

        self.count = count
        return count

class Dawg:
    def __init__(self):
        self.previousWord = ""
        self.root = DawgNode()

        # Here is a list of nodes that have not been checked for duplication.
        self.uncheckedNodes = []

        # Here is a list of unique nodes that have been checked for
        # duplication.
        self.minimizedNodes = {}

    def insert( self, word ):
        if word <= self.previousWord:
            raise Exception("Error: Inserted {0} after {1}; must be inserted in alphabetical order." \
                    .format(word, self.previousWord))

        # find common prefix between word and previous word
        commonPrefix = 0
        for i in range( min( len( word ), len( self.previousWord ) ) ):
            if word[i] != self.previousWord[i]: break
            commonPrefix += 1

        # Check the uncheckedNodes for redundant nodes, proceeding from last
        # one down to the common prefix size. Then truncate the list at that
        # point.
        self._minimize( commonPrefix )

        # add the suffix, starting from the correct node mid-way through the
        # graph
        if len(self.uncheckedNodes) == 0:
            node = self.root
        else:
            node = self.uncheckedNodes[-1][2]

        for letter in word[commonPrefix:]:
            nextNode = DawgNode()
            node.edges[letter] = nextNode
            self.uncheckedNodes.append( (node, letter, nextNode) )
            node = nextNode

        node.final = True
        self.previousWord = word

    def finish( self ):
        # minimize all uncheckedNodes
        self._minimize( 0 );

        # go through entire structure and assign the counts to each node.
        self.root.numReachable()

    def _minimize( self, downTo ):
        # proceed from the leaf up to a certain point
        for i in range( len(self.uncheckedNodes) - 1, downTo - 1, -1 ):
            (parent, letter, child) = self.uncheckedNodes[i];
            if child in self.minimizedNodes:
                # replace the child with the previously encountered one
                parent.edges[letter] = self.minimizedNodes[child]
            else:
                # add the state to the minimized nodes.
                self.minimizedNodes[child] = child;
            self.uncheckedNodes.pop()

    def lookup( self, word ):
        node = self.root

        for letter in word:
            if letter not in node.edges: return False
            node = node.edges[letter]

        return True

    def nodeCount( self ):
        return len(self.minimizedNodes)

    def edgeCount( self ):
        count = 0
        for node in self.minimizedNodes:
            count += len(node.edges)
        return count

    def display(self):
        stack = [self.root]
        done = set()
        while stack:
            node = stack.pop()
            if node.id in done: continue
            done.add(node.id)
            print("{}: ({})".format(node.id, node))
            for label, child in node.edges.items():
                print("    {} goto {}".format(label, child.id))
                stack.append(child)

    def renumber(self):
        id = 0
        for node in self.minimizedNodes:
            node.id = id
            id += 1
        DawgNode.NextId = id

    def encode_words(self):
        elements = {0: ["", 0, True, True]}
        edgeId = 1
        nodeFirstEdgeDict = {-1: 0}
        allNodes = [self.root] + list(self.minimizedNodes)
        for node in allNodes:
            if len(node.edges.items()) == 0: # special first node for nulls, we inserted already
                continue
            nodeFirstEdgeDict[node.id + 1] = edgeId
            edges = dict(sorted(node.edges.items()))
            for char, pointedNode in edges.items():
                isEOL = list(node.edges)[-1] == char
                elements[edgeId] = [char, pointedNode.id, pointedNode.final, isEOL]
                edgeId += 1
        # transform node IDs into edge IDs and pack into a word
        processedElements = []
        for _, element in elements.items():
            char = (str.encode(element[0])[0] - 0x60) if len(element[0]) else 0
            if char > 26 or char < 0:
                raise Exception("Can't handle characters beyond a-z, handled {}".format(char))
            nodeId = element[1]
            index = nodeFirstEdgeDict[nodeId + 1] if nodeId > 0 else 0
            final = element[2]
            eol = element[3]
            word = (char) | (int(final) << 5) | (int(eol) << 6) | ((index & 0x1FFFFFF) << 7)
            #print("{4:X}: {0} {1} {2} {3}".format(chr(char + 0x60), int(final), int(eol), index, word))
            #print("{4:X}: {0:X} {1:X} {2:X} {3:X}".format(char + 0x60, int(final)<< 5, int(eol) << 6, index << 7, word))
            #Dawg.decode_word(word)
            processedElements.append(word)
        return processedElements

    def decode_word(word):
        char = chr((word & 0x1F) + 0x60)
        final = (word & 0x20) >> 5
        eol = (word & 0x40) >> 6
        index = (word & 0xffffff80) >> 7
        print("{4:X}: {0} {1} {2} {3}".format(char, final, eol, index, word))

def should_include(word):
    return len(word) >= args.minimum and len(word) <= args.maximum and letters.match(word)

dawg = Dawg()
WordCount = 0
words = open(args.filename, "rt") \
    .read() \
    .split()
# The word must be lower case to reduce the character range.
# Skip characters that we can't represent by default;
# perhaps we can provide a mapping table to use the rest of them
words = list(
        set(
            filter(should_include,
                   map(str.lower, words)
                   )
            )
        )
# Sort after filtering characters we don't want
words.sort()
start = time.time()    
for word in words:
    WordCount += 1
    dawg.insert(word)
    if ( WordCount % 100 ) == 0: print("{0}\r".format(WordCount), end="")
dawg.finish()
print("Dawg creation took {0} s".format(time.time()-start))

NodeCount = dawg.nodeCount()
EdgeCount = dawg.edgeCount()
print("Read {0} words into {1} nodes and {2} edges".format(
    WordCount, NodeCount, EdgeCount))

if args.output:
    dawg.renumber()
    words = dawg.encode_words()
    # for word in words:
    #     Dawg.decode_word(word)
    # print(len(words) * 4)
    dawg_file = open(args.output, "wb")
    byte_size = 4
    if EdgeCount < 0x1FFFF:
        byte_size = 3
    elif EdgeCount < 0x1FF:
        byte_size = 2
    print("{2} will be {0} bytes ({1} byte words)".format(EdgeCount * byte_size, byte_size, args.output))
    # ensure last word has EOL; make sure iterating past edge word won't work
    if (words[-1] & (1 << 6)) == 0:
        raise Exception("Last word does not have end of list bit set")
    # first byte is final/EOL but also byte size and edge count
    words[0] = (1 << 5) | (1 << 6) | byte_size | ((EdgeCount & 0x1FFFFFF) << 7)
    for word in words:
        b = struct.pack("<I", word)
        dawg_file.write(b[0:byte_size])
    # last words are word count (always 4 bytes, as wordCount can > edgeCount)
    # and then node count
    b = struct.pack("<I", WordCount)
    dawg_file.write(b)
    b = struct.pack("<I", NodeCount)
    dawg_file.write(b)
    dawg_file.close()
