CFLAGS := -Wall -g

.PHONY: all clean check

all: readpup

clean:
	rm -f *.o readpup *.pup

%.pup: %.txt dawg.py
	python3 dawg.py -m 2 -M 32 -o $@ $<

# Dataset from https://people.sc.fsu.edu/~jburkardt/datasets/words/words.html
check: readpup sowpods.pup anagram.pup
	./readpup sowpods.pup house houses hous
	./readpup anagram.pup house houses hous

readpup: readpup.o dawg.o
