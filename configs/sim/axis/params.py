from interpreter import *
import redis

'''

This module demonstrates the Python API for accessing and saving
numbered interpreter parameters in redis, as well as saving and
restoring values which are persistent across invocations.

'''

debug = 0
def __init__(self):
    '''
        add any one-time initialisation requiring the interpreter instance  here
    '''
    global debug
    debug = self.debugmask & 0x00080000 # EMC_DEBUG_PYTHON_API
    self.rc = redis.Redis()
    try:
        if self.rc.ping() and debug:
            print "Redis connection established"
    except Exception, e:
        return "params.__init__: Redis connection failed: " + str(e)
    return INTERP_OK    

def set_param(self, index, newvalue, notify):
    if debug:
        print "set_param(#%d %f -> %f notify=%d)" % (index, self.parameters[index], newvalue, notify)
    if self.is_persistent(index) or self.is_required(index):
        try:
            p = self.rc.hset('param', index, newvalue)
            if debug:
                print "param",index,"set: ",newvalue
        except Exception,e:
            return "set_param(%d,%f): hset failed: %s" % (index, newvalue, e)
    self.parameters[index] = newvalue
    if notify:
        self.rc.publish('param', index) 
    return INTERP_OK    

def get_param(self, index):
    if debug: 
        print "get_param(%d)" % (index)
    if self.is_persistent(index):
        try:
            p = self.rc.hget('param', index)
            if debug: print "param",index,"retrieved: ",p
            self.parameters[index] = float(p)
            return INTERP_OK    
        
        except Exception,e:
            return "get_param(%d): 'hget param %d' failed: %s" % (index, index, str(e))

    return INTERP_OK    
        

def save_params(self):
    if debug: print "save_params"
    for i in range(1,len(self.parameters)):
        if self.is_persistent(i) or self.is_required(i):
            try:
                self.rc.hset('param', i, self.parameters[i])
            except Exception,e:
                return "save_params(): 'hset param %d' failed: %s" % (i,str(e))
    return INTERP_OK    

def restore_params(self, *args):
    if debug: print "restore_params: ",
    n_readonly = 0
    n_set = 0
    n_default = 0
    n_volatile = 0
    n_bad = 0
    missing = {};
    max_parameters = len(self.parameters)
    
    # assume all required params as missing until found
    for i in range(1, max_parameters):
        missing[i] = 1

    try:
        for k, value  in self.rc.hgetall('param').items():
            i = int(k)
            if (i < 0) or (i >= max_parameters):
                n_bad += 1
                continue
            if self.is_readonly(i):
                n_readonly += 1
                continue
            missing[i] = 0

            # also record params as persistent if we found them in the redis db
            # this will pick up params >30, < 5400 as persistent
            if self.is_volatile(i) and not self.is_readonly(i) and not self.is_required(i):
                self.set_required(i)
                n_volatile += 1
            if not self.is_readonly(i):    
                self.parameters[i] = float(value)
                n_set += 1
            else:
                n_readonly += 1

    except Exception,e:
        return "restore_params(): 'hgetall param' failed: %s" % (str(e))

    # check for missing
    for i in range(1, max_parameters):
        if missing[i]:
            n_default += 1
            self.parameters[i] = 0.0

    if debug:
        print " %d restored, %d R/O skipped, %d bad, %d default, %d volatile" % (n_set, n_readonly, n_bad, n_default, n_volatile)
    return INTERP_OK    



