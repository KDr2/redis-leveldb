
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

ifeq ($(uname_S),Darwin)
  LIBEV?=/opt/local
  GMP?=/opt/local
else
  LIBEV?=/usr
  GMP?=/usr
endif

CFLAGS += -Wall -I$(LIBEV)/include -I$(GMP)/include -Ivendor/leveldb/include -std=c99
CXXFLAGS += -Wall -I$(LIBEV)/include -I$(GMP)/include -Ivendor/leveldb/include
LDFLAGS += vendor/libleveldb.a -lm -L$(LIBEV)/lib -lev -L$(GMP)/lib -lgmp

ifeq ($(uname_S),Linux)
  LDFLAGS += -lpthread
endif

ifeq ($(DEBUG),1)
  CXXFLAGS += -g -DDEBUG
endif


OBJS = rl_util.o rl_server.o rl_connection.o rl_request.o rl.o

redis-leveldb: $(OBJS) vendor/libleveldb.a
	$(CXX) $^ $(LIBS) $(LDFLAGS) -o $@


clean:
	-rm redis-leveldb
	-rm *.o

distclean: clean
	-rm vendor/*.a
	cd vendor/leveldb; make clean

vendor/libleveldb.a:
	cd vendor/leveldb; make && cp libleveldb.a ..
