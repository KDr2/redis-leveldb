import redis
r = redis.StrictRedis(host='localhost', port=8323)

for i in range(10000):
    key = 'foo_%d'%i
    r.set(key, 'b'*i)
    #r.get(key)
    #if i%100==0:
    print key, "->", i
"""
print r.incr('ddd')
print r.get('ddd')
print r.incr('ddd')
print r.get('ddd')
print r.incr('ddd')
print r.get('ddd')

"""


