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

# Connect to Custom Compiler, PyCellStudio if  found.

if [ -n "$CC_HOME" ]; then
    export SYNOPSYS_CUSTOM_INSTALL=$CC_HOME/auxx
    export CNI_ROOT=$CC_HOME/linux64/PyCellStudio
fi
if [ -n "$CNI_ROOT" ]; then
    cni_bin=$CNI_ROOT/bin

    . $cni_bin/functions.bash

    setup_PYTHON_VERSION "$@"
    check_CNI_ROOT
    get_platform
    check_platform
    setup_OpenAccess
    setup_CNI_env
    setup_Python_env
    setup_LD_LIBRARY_PATH "$@"
    setup_PATH
fi

# Call the Xic program, passing along the argument list.

export LD_LIBRARY_PATH
/usr/local/xictools/xic/bin/xic $*

