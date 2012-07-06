.. -*- rst -*-

Redis-LevelDB
============================================================

Redis COMMAND Supported
------------------------------------------------------------

* incr/incrby
* get/set
* mget/mset
* multi/exec/discard
* keys
* info: Different to redis, this info command accepts an flag argument,
  eg: `info`, `info k`, `info t`, `info kt`
  
  * default: show leveldb.stat info
  * k: show the count of all keys
  * t: show leveldb.sstables info

Dependencies
------------------------------------------------------------
1. libev:
   
   install with apt-get or port please.
   
2. gmp(http://gmplib.org/):
   
   install with apt-get or port please.

3. leveldb:
   
  #. git clone git://github.com/appwilldev/redis-leveldb.git
  #. cd redis-leveldb
  #. git submodule init
  #. git submodule update

Compile
------------------------------------------------------------

[LIBEV=LIBEV_PREFIX GMP=GMP_PREFIX DEBUG=1] make

Run
------------------------------------------------------------

./redis-leveldb -h


    
