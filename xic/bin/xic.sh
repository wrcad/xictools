#! /bin/sh

# Wrapper script for starting Xic.

# This file can be customized if necessary, but beware that it may be
# be replaced on package update.  If you need to modify, make a copy
# for yourself and keep it somewhere safe.

# Handle the XIC_LIBRARY_PATH variable by prepending it to the dynamic
# linker search path.  This can be used to pass the location of
# libraries needed by plugins, for example for OpenAccess.  Set
# XIC_LIBRARY_PATH in your shell startup script (e.g., .bashrc)
# instead of LD_LIBRARY_PATH.
#
if [ -n "$XIC_LIBRARY_PATH" ]; then
    if [ -n "$LD_LIBRARY_PATH" ]; then
        LD_LIBRARY_PATH="$XIC_LIBRARY_PATH:$LD_LIBRARY_PATH"
    else
        LD_LIBRARY_PATH="$XIC_LIBRARY_PATH"
    fi
fi

# Call the Xic program, passing along the argument list.

export LD_LIBRARY_PATH
/usr/local/xictools/xic/bin/xic $*

