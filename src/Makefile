
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

ifeq ($(uname_S),Darwin)
  LIBEV?=/opt/local
  GMP?=/opt/local
else
  LIBEV?=/usr
  GMP?=/usr
endif

CFLAGS += -Wall -I$(LIBEV)/include -I$(GMP)/include -I../vendor/leveldb/include -std=c99
CXXFLAGS += -Wall -I$(LIBEV)/include -I$(GMP)/include -I../vendor/leveldb/include
LDFLAGS += ../vendor/libleveldb.a -lm -L$(LIBEV)/lib -lev -L$(GMP)/lib -lgmp

ifeq ($(uname_S),Linux)
  LDFLAGS += -lpthread
endif

ifeq ($(DEBUG),1)
  CXXFLAGS += -g -DDEBUG
endif

all: redis-leveldb

OBJS = rl_util.o rl_server.o rl_connection.o rl.o \
       rl_request.o rl_kv.o rl_set.o rl_hash.o

rl_util.o: rl_util.h rl_server.h rl_util.cpp
rl_server.o: rl_util.h rl_server.h rl_connection.h rl_server.cpp
rl_connection.o: rl_util.h rl_server.h rl_connection.h rl_request.h rl_connection.cpp
rl_request.o: rl.h rl_util.h rl_server.h rl_connection.h rl_compdata.h rl_request.h rl_request.cpp
rl_kv.o: rl.h rl_util.h rl_server.h rl_connection.h rl_request.h rl_kv.cpp
rl_set.o: rl.h rl_util.h rl_server.h rl_connection.h rl_compdata.h rl_request.h rl_set.cpp
rl_set.o: rl.h rl_util.h rl_server.h rl_connection.h rl_compdata.h rl_request.h rl_hash.cpp
rl.o: rl_util.h rl_server.h rl_connection.h rl_request.h rl.cpp


redis-leveldb: $(OBJS) ../vendor/libleveldb.a
	$(CXX) $^ $(LIBS) $(LDFLAGS) -o ../$@

clean:
	-rm ../redis-leveldb
	-rm *.o

distclean: clean
	-rm vendor/*.a
	cd vendor/leveldb; make clean

../vendor/libleveldb.a:
	cd ../vendor/leveldb; make && cp libleveldb.a ..
