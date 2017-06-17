#! /bin/sh

if [ x"$PKG_PREFIX" = "x" ]; then
    prefix=/usr/local
else
    prefix=$PKG_PREFIX
fi

libdir=$prefix/xictools/wrspice
relnote=wrs@VERSION@

# Back up config files
cfgfiles="mozyrc \
    news \
    wrspice_mesg \
    wrspiceinit"

for a in $cfgfiles; do
    if [ -f $libdir/startup/$a ]; then
        cp -pf $libdir/startup/$a $libdir/startup/$a.tmporig
    fi
done

if [ -f $libdir/docs/$relnote ]; then
    cp -pf $libdir/docs/$relnote $libdir/docs/$relnote.tmp
fi

