import redis
import time

r = redis.StrictRedis(host='localhost', port=8323)
#time.sleep(1)
#print r.set("key", 'b'*50)
#print r.get("key")

for i in range(10000):
    key = 'foo_%d'%i
    r.set(key, 'b'*(i%1000))
    if i%100==0:
        print key, "->"#,r.get(key)

print r.incr('ddd')
print r.get('ddd')
print r.incr('ddd')
print r.get('ddd')
print r.incr('ddd')
print r.get('ddd')



