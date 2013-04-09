#!/usr/bin/python
# -*- coding: utf-8 -*-
import sys
import redis
import time

r = redis.StrictRedis(host='localhost', port=8323)

def functional_test():
    # set/get
    print r.set("key", 'b'*5000)
    print len(r.get("key"))

    # incr
    print r.set("incr_key",0)
    print r.get("incr_key")
    print r.incr('incr_key')
    print r.get("incr_key")

    # incrby
    print r.set("incrby_key",0)
    print r.get("incrby_key")
    print r.incr('incrby_key',1111)
    print r.incr('incrby_key',2222)
    print r.incr('incrby_key',3333)
    print r.get("incrby_key")

    # mset/mget
    print r.mset({"key0": 'a'*100, "key1": "b"*10, "中文key": "中文Value"})
    values = r.mget("key0","key1", "key3", "key4", "中文key")
    for v in values:
        sys.stdout.write(str(v)+", ")
    print ""

def pipeline_test():
    # transaction: multi/exec/discard
    pipe = r.pipeline()
    for i in range(30000):
        pipe.set('foo1', 'bar')
        pipe.get('foo1')
        pipe.incr("c")
    print pipe.execute()


def press_test():
    for i in range(10000):
        key = 'foo_%d'%i
        r.set(key, 'b'*i)
        if i%100==0:
            print key, "->", len(r.get(key))

if __name__=="__main__":
    #functional_test()
    #press_test()
    #pipeline_test()
    ks = []
    for x in xrange(230):
        ks.append("%010d" % x)
    print(r.mget(ks))
