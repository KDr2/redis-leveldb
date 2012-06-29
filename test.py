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
    print r.get("incrby_key")

    # mset/mget
    print r.mset({"key0": 'a', "key1": "b"})
    print r.mget("key0","key1")

    # transaction: multi/exec/discard


def press_test():
    for i in range(10000):
        key = 'foo_%d'%i
        print r.set(key, 'b'*(i%1000+1))
        if i%100==0:
            print key, "->", len(r.get(key))

if __name__=="__main__":
    functional_test()
    press_test()



