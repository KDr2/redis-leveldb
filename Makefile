
redis-leveldb: redis-leveldb.c \
		vendor/libleveldb.a
	gcc -Wall -Ivendor/leveldb/include -std=c99 -c -o redis-leveldb.o -O0 -ggdb redis-leveldb.c
	g++ -o redis-leveldb redis-leveldb.o -lev vendor/libleveldb.a -lm

clean:
	rm redis-leveldb *.o

distclean: clean
	rm vendor/*.a

vendor/libleveldb.a:
	cd vendor/leveldb; make && cp libleveldb.a ..
