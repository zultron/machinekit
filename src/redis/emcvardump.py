#!/usr/bin/python
# export emc.var compatible file from redis database

import redis

rc = redis.Redis()
t = rc.hgetall('param');

for key in sorted(t.iterkeys()):
    print key, t[key]
