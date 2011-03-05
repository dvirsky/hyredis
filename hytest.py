import hyredis
h = hyredis.Redis()
print "Set: %s" % h.command('set', ('foo', 'bar'))
print "Get: %s" % h.command('get', ('foo',))
