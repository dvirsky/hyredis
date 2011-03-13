#!/bin/sh
# 
# File:   build_ext.sh
# Author: dvirsky
#
# Created on Mar 12, 2011, 6:52:12 PM
#

python setup.py build &&  cp build/lib.linux-x86_64-2.6/hyredis.so ./ && python hytest.py