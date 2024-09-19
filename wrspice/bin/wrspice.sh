#! /bin/bash

# Wrapper script for starting WRspice.

# This file can be customized if necessary, but beware that it may be
# be replaced on package update.  If you need to modify, make a copy
# for yourself and keep it somewhere safe.

# This is used to change the graphics package to use.  The .xtrc file
# in the home or current directory can contain things to use here,
# specifically a line like "grpref=QT6" or similar (used below).

if [ -f ./.xtrc ]; then 
    source ./.xtrc
elif [ -f $HOME/.xtrc ]; then
    source $HOME/.xtrc
fi

# Handle the WRS_LIBRARY_PATH variable by prepending it to the dynamic
# linker search path.  This can be used to pass the location of
# libraries needed by plugins.
#
if [ -n "$WRS_LIBRARY_PATH" ]; then
    if [ -n "$LD_LIBRARY_PATH" ]; then
        LD_LIBRARY_PATH="$WRS_LIBRARY_PATH:$LD_LIBRARY_PATH"
    else
        LD_LIBRARY_PATH="$WRS_LIBRARY_PATH"
    fi
fi


# Call the WRspice program, passing along the argument list.

if [ -z "$grpref" ]; then
    grpref=GTK2
fi
if [ $grpref != GTK2 -a $grpref != QT6 -a $grpref != QT5 ]; then
    echo "Unknown graphics setting $grpref, known are GTK2, QT6, and QT5."
    exit 1
fi

export LD_LIBRARY_PATH
mypath=$(dirname $(readlink -f "$0"))
# mypath is now the full path to the directory containing this file.
# If the directory is named "bin" and it has the grpref subdirectory,
# execute the binary from grpref.
if [ $(basename $mypath) == bin ]; then
    if [ -d $mypath/$grpref ]; then
        $mypath/$grpref/wrspice $*
        exit $?
    fi    
fi 
# Otherwise, as in installed area, go to ../wrspice/bin.
mypath=$(dirname $mypath)/wrspice/bin
$mypath/$grpref/wrspice $*
exit $?

