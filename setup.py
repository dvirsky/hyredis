from distutils.core import setup, Extension

setup(name = "HyRedis",
      version = "0.1",
      ext_modules = [Extension("hyredis", ['src/reader.c', 'src/net.c', 'src/hiredis.c', 'src/sds.c', 'src/hyredis.c'],
                    extra_compile_args = ['-std=c99', '-O3'] )]
      )
