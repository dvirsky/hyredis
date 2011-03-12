import hyredis
h = hyredis.Redis()
print "Set: %s" % h.command(('set', 'foo', 'bar'))
print "Get: %s" % h.command(('get', 'foo',))
print "Testing transaction..."
p = hyredis.Redis(pipeline = True)
assert(p)
p.command(('set', 'bar', 'baz'))
p.command(('get', 'bar',))
p.command(('get', 'foo',))
r = p.execute()
assert(r)
print "Transactino output: %s" % r
