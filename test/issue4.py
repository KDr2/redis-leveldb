#!/usr/bin/python
# -*- coding: utf-8 -*-
import sys
import redis
import time

r = redis.StrictRedis(host='localhost', port=8323)

def press_test():
    k = "test"
    v = "test"
    for i in range(10000):
        r.delete(k)
        r.set(k, v*20)

if __name__=="__main__":
    press_test()
