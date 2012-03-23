
redis-leveldb: redis-leveldb.c \
		vendor/libleveldb.a \
		vendor/libev.a
	gcc -Wall -Ivendor/libev -Ivendor/leveldb/include -std=c99 -c -o redis-leveldb.o -O0 -ggdb redis-leveldb.c
	g++ -o redis-leveldb redis-leveldb.o vendor/libev.a vendor/libleveldb.a -lm

clean:
	rm redis-leveldb *.o

distclean: clean
	rm vendor/*.a

vendor/libleveldb.a:
	cd vendor/leveldb; make && cp libleveldb.a ..

vendor/libev.a:
	cd vendor/libev; ./configure --disable-shared && make && cp .libs/libev.a ..
