#! /bin/bash

# This can only be run from the git repository!!!

# release.sh script for XicTools
PRENUM="4.3"
PRETOK="xt-$PRENUM"
sequence_number="25"
SEARCH_TAGS_DIRECT=yes

# If no argument, print the tag string with appname stripped.
if [[ -z "$1" ]]; then
    echo $PRENUM.$sequence_number
    exit 0
fi

# See this file for documentation about the arguments to and use of
# this script.
dir=$(dirname "$(realpath "$0")")
source "$dir/xt_base/util/release-sub.sh"

