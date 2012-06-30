
LIBEV?=/usr
GMP?=/usr
CFLAGS += -I$(LIBEV)/include -I$(GMP)/include -Ivendor/leveldb/include -std=c99
LDFLAGS += vendor/libleveldb.a -lm -L$(LIBEV)/lib -lev -L$(GMP)/lib -lgmp

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

ifeq ($(uname_S),Linux)
  LDFLAGS += -lpthread
endif


redis-leveldb: redis-leveldb.c \
		vendor/libleveldb.a
	gcc -Wall $(CFLAGS) -c -o redis-leveldb.o -O0 -ggdb redis-leveldb.c
	g++ -o redis-leveldb redis-leveldb.o $(LDFLAGS)

clean:
	rm redis-leveldb *.o

distclean: clean
	rm vendor/*.a

vendor/libleveldb.a:
	cd vendor/leveldb; make && cp libleveldb.a ..
