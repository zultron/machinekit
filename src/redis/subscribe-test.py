import redis

rc = redis.Redis()
ps = rc.pubsub()
ps.subscribe('param')
for item in ps.listen():
    #    print "type=%s channel=%s pattern=%s data=%s " %  (item['type'],item['channel'],item['pattern'],item['data'])

    if item['channel'] == "param":
        p = item['data']
        t = rc.hget('param',p);
        print "%s changed: %s" % (p,t)
