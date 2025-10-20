.PHONY: all clean

all: readpup

clean:
	rm *.o readpup

readpup: readpup.o dawg.o
