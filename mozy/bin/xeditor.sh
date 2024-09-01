#! /bin/sh

# Wrapper script for starting xeditor.

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

# Call the program, passing along the argument list.

if [ -z "$grpref" ]; then
    grpref=GTK2
fi
if [ $grpref != GTK2 -a $grpref != QT6 -a $grpref != QT5 ]; then
    echo "Unknown graphics setting $grpref, known are GTK2, QT6, and QT5."
    exit 1
fi

MOZYPATH/$grpref/xeditor $*

