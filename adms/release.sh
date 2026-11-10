#! /bin/bash

# This can only be run from the git repository!!!

# The release number tracks the source distribution release number,
# e.g., 2.3.8, with an additional trailing index for local revisions.
# I.e., we will have 2.3.80, 2.3.81, ...

# adms 2.3.81
# Updated files/configure.in.sed script to avoid warnings about obsolete
# syntax.
# Fixed bug, the git tag was not being extracted (for logging).
# Compiler warnings fixed.

# release.sh script for adms
PRENUM="2.3"
PRETOK="adms-$PRENUM"
sequence_number="81"

# If no argument, print the tag string with appname stripped.
if [[ -z "$1" ]]; then
    echo $PRENUM.$sequence_number
    exit 0
fi

# See this file for documentation about the arguments to and use of
# this script.
dir=$(dirname "$(realpath "$0")")
source "$dir/../xt_base/util/release-sub.sh"

