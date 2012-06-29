.. -*- rst -*-

Redis-LevelDB
============================================================

Note
------------------------------------------------------------

This is a **LEGACY** version, do NOT use this branch, go to
branch master please.

Redis COMMAND Supported
------------------------------------------------------------

* incr/incrby
* get/set
  
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

[LIBEV=LIBEV_PREFIX GMP=GMP_PREFIX] make

Run
------------------------------------------------------------

./redis-leveldb -h

