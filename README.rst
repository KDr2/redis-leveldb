.. -*- rst auto-fill -*-

Redis-LevelDB
============================================================

A redis-protocol compatible frontend to google's leveldb: Use leveldb
as a Redis-Server.

Current Version: 1.4(stable).

Redis COMMAND Supported
------------------------------------------------------------

* key-value commands:

  - incr/incrby
  - get/set
  - mget/mset

* set commands(New):

  - sadd
  - srem
  - scard
  - smembers
  - sismemeber

* hash commands(New):

  - hget
  - hset
  - hsetnx
  - hdel
  - hexists
  - hgetall
  - hkeys
  - hvals
  - hlen

* transaction commands:

  - multi
  - exec
  - discard

* connection commans:

  - select: select db (when redis-leveldb run in multi-db mode, with
    argument ``-M <num>``)

* server commands:

  - keys
  - info: Different to redis, this info command accepts a flag
    argument, eg ``info``, ``info k``, ``info t``, ``info kt``

      * default: show leveldb.stat info
      * k: show the count of all keys
      * t: show leveldb.sstables info

Dependencies
------------------------------------------------------------
1. libev(>=1.4):

   install with apt-get or port please.

2. gmp(http://gmplib.org/):

   install with apt-get or other package manager please.

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

``./redis-leveldb -h``

options:

* -d:              run redis-level as a daemon process
* -H <host-ip>:    host addr to listen on(eg: 127.0.0.1)
* -P <port>:	   port to listen on(default 8323)
* -D <data-dir>:   leveldb data dir(default "redis.db" under your work
  directory)
* -M <number>:     run in multi-db mode and set its db count to
  <number>, each db in the server is a separatly leveldb database and
  its data directory is a directory named ``db-<num>`` under the
  directory you specified with the option ``-D``; you can use command
  ``select`` to switch db on the client side while redis-leveldb is
  running in this mode.
