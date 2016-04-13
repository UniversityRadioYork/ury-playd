#!/bin/sh

# Makes and installs libuv.
# This is primarily for the benefit of travis-ci, whose version of Ubuntu at
# time of writing (12.04) does not have packages for libuv-dev.

# Requires GNU Autotools, gcc(/clang?), sudo, and write permissions to /tmp.

wget https://github.com/libuv/libuv/archive/v1.8.0.tar.gz -O /tmp/libuv.tar.gz
cd /tmp
tar xzf libuv.tar.gz
cd libuv-1.8.0
sh autogen.sh
./configure
make
sudo make install

# libuv installs to /usr/local/lib, which is not in the ld path on some OSes
# (ubuntu 12.04, for instance), so ask ldconfig to cache the contents of that
# directory for us.
sudo ldconfig /usr/local/lib
