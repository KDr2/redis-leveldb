
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

ifeq ($(uname_S),Darwin)
  LIBEV?=/opt/local
  GMP?=/opt/local
else
  LIBEV?=/usr
  GMP?=/usr
endif

CFLAGS += -I$(LIBEV)/include -I$(GMP)/include -Ivendor/leveldb/include -std=c99
CXXFLAGS += -I$(LIBEV)/include -I$(GMP)/include -Ivendor/leveldb/include 
LDFLAGS += vendor/libleveldb.a -lm -L$(LIBEV)/lib -lev -L$(GMP)/lib -lgmp

ifeq ($(uname_S),Linux)
  LDFLAGS += -lpthread
endif

ifeq ($(DEBUG),1)
  CXXFLAGS += -g -DDEBUG
endif


OBJS = rl_util.o rl_server.o rl_connection.o rl_request.o rl.o

redis-leveldb: $(OBJS) vendor/libleveldb.a
	$(CXX) $(LDFLAGS) $^ $(LIBS) -o $@


clean:
	rm redis-leveldb *.o

distclean: clean
	rm vendor/*.a

vendor/libleveldb.a:
	cd vendor/leveldb; make && cp libleveldb.a ..
