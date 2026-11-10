#! /bin/bash

# This can only be run from the git repository!!!

# Previously, FastCap-2.0wr was versioned by a date code.  The
# present framework requires a "1.2.3" format for version.  We'll keep
# 2.0 for major/minor, and increment the release.  The 033020
# distribution is release number 14.

# release.sh script for FastCap
PRENUM="2.0"
PRETOK="fastcap-$PRENUM"
sequence_number="18"

# If no argument, print the tag string with appname stripped.
if [[ -z "$1" ]]; then
    echo $PRENUM.$sequence_number
    exit 0
fi

# See this file for documentation about the arguments to and use of
# this script.
dir=$(dirname "$(realpath "$0")")
source "$dir/../xt_base/util/release-sub.sh"

