import redis
r = redis.StrictRedis(host='192.168.2.61', port=8323)

print r.set("key", 'b'*5000)
print r.get("key")

"""
for i in range(10000):
    key = 'foo_%d'%i
    r.set(key, 'b'*i)
    #if i%100==0:
    print key, "->",r.get(key)

print r.incr('ddd')
print r.get('ddd')
print r.incr('ddd')
print r.get('ddd')
print r.incr('ddd')
print r.get('ddd')

"""


