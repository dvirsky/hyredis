#! /usr/bin/python

# Copyright 2010 (c) doit9.com
# Copying and/or distribution of this file is prohibited.

__author__="dvirsky"
__date__ ="$Feb 10, 2011 7:02:00 PM$"

#import credis
import redis
import time
import hyredis




def _benchHVALS(mod, num):
    i = 0
    r = mod.Redis()
    r.select(15)
    ids = [142,22,142,194,181,211,213,220,225,241,243]
    while i < num:
        i += 1
        v = r.hvals('experiences:%d' % ids[i % 10])
        
def cbenchHVALS(num):
    _benchHVALS(credis, num)

def pbenchHVALS(num):
    _benchHVALS(redis, num)

def _benchpipe(mod, num):
    ids = [142,22,142,194,181,211,213,220,225,241,243]
    r = mod.Redis()
    r.select(15)
    p = r.pipeline()
    
    

    i = 0
    while i < num:
        i += 1
        p.hvals('experiences:%d' % ids[i % 10])
        if i % 10000 == 0:
            
            x = p.execute()
    x = p.execute()
    #print x[0]



def benchLpush(num):
    r = HyRedis.Redis()
    r.select(15)
    for n in xrange(num):
        r.rpush('mylst', n)
#benchLpush(10000)
def benchLrange(num):
    r =HyRedis.Redis()
    r.select(15)
    for i in range(10000):
        ret = r.lrange('mylst', 0, 1000)
        
	#print ret
    return 10000


def benchSadd(num):
    r = redis.Redis()
    r.select(15)
    for n in xrange(num):
        r.sadd('myset1', n)

def benchSmembs(num):
    r = redis.Redis()
    r.select(15)
    ret = len(r.smembers('myset1'))
    
    return ret

def cbenchPipe(num):
    _benchpipe(credis, num)
    
def pbenchPipe(num):
    _benchpipe(redis, num)


def _benchSort(mod, num):
    r = mod.Redis()
    r.select(15)
    i = 0
    while i < num:
        i +=1
        x = r.sort('k:app_experiences:experienceId=194',  get  = 'app_experiences:*->id')
        print "%d %s" % (i, x)

def testHMGET():

    r = credis.Redis()
    r.select(15)
    for i in xrange(100):
        x = r.hmget('experiences:194',  ['id', 'name'])
        print i
        print x
    
def cbenchSort(num):
    _benchSort(redis_proxy, num)

def pbenchSort(num):
    _benchSort(redis, num)
    
from multiprocessing import Pool
def benchMultiproc(numProcs):
        


    numExps = 10000
    if __name__ == '__main__':
        pool = Pool(processes=numProcs)              # start 4 worker processes
        st = time.time()
        r = pool.map(benchLrange, [numExps for i in range(numProcs)])
        print r
        et = time.time()
        difftime = et - st

        print "Getting %d experiences took %.01fmsec (rate: %f fetches/sec)" % (numProcs * numExps, difftime * 1000, float(numProcs * numExps) / difftime)

def testSort():

    cbenchSort(1)

class invoker(object):

        def __init__(self, conn, command):

            self._command =  command
            self._conn = conn

        def invoke(self, *args):

            return self._conn.command(self._command, args)


class redis_proxy(object):

    def __init__(self, host = 'redis', port = 6379, db = 0):

        self._conn = hyredis.Redis(host = host, port = port, db = db)


    def __getattr__(self, name):

        return lambda *args: self._conn.command(name, args)


class HyRedis(object):

    @staticmethod
    def Redis(**kwargs):
        return redis_proxy(**kwargs)



if __name__ == "__main__":

    def benchSingle(func):
        num = 1
        st = time.time()

        num = func(num)
        et = time.time()
        print "time: %.03f, rate: %.03f" % (et - st, (float(num) / (et-st)))
    #benchSingle(cbenchSort)

    benchMultiproc(8)
    #benchSets()
    #testSort()
    #benchLpush(100)
    #benchLrange(1000)
    #testHMGET()
