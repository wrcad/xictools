#! /bin/sh

if [ x"$PKG_PREFIX" = "x" ]; then
    prefix=/usr/local
else
    prefix=$PKG_PREFIX
fi

libdir=$prefix/xictools/vl

# nothing to do

