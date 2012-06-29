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
    print r.mset({"key0": 'a'*10000, "key1": "b"*1000})
    print r.mget("key0","key1", "key3", "key4")

def pipeline_test():
    # transaction: multi/exec/discard
    pipe = r.pipeline()
    pipe.set('foo', 'bar')
    pipe.get('bing')
    print pipe.execute()


def press_test():
    for i in range(10000):
        key = 'foo_%d'%i
        print r.set(key, 'b'*(i%1000+1))
        if i%100==0:
            print key, "->", len(r.get(key))

if __name__=="__main__":
    #functional_test()
    #press_test()
    pipeline_test()


