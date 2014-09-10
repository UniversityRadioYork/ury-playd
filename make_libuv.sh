#!/bin/sh

# Makes and installs libuv.
# This is primarily for the benefit of travis-ci, whose version of Ubuntu
# at time of writing (12.04) does not have packages for libuv-dev.

# Requires GNU Autotools, gcc(/clang?), sudo, and write permissions to /tmp.

wget https://github.com/joyent/libuv/archive/v0.11.29.tar.gz -O /tmp/libuv.tar.gz
cd /tmp
tar xzf libuv.tar.gz
cd libuv-0.11.29
sh autogen.sh
./configure
make
sudo make install
