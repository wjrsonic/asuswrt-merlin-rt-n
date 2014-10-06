#!/bin/sh

# fix neon missing proxy.h
cp /usr/include/proxy.h ~/asuswrt-merlin/release/src/router/neon/
# fix broken configure script for libdaemon
cd ~/asuswrt-merlin/release/src/router/libdaemon
aclocal
# fix broken configure script for libxml2
cd ~/asuswrt-merlin/release/src/router/libxml2
sed -i s/AM_C_PROTOTYPES/#AM_C_PROTOTYPES/g ~/asuswrt-merlin/release/src/router/libxml2/configure.in
aclocal