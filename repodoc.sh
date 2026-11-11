#! /bin/bash

# This can only be run from the git repository!!!
#
# repodoc.sh -- utility to cat the log text since the previous commit
# into a file.  This may be useful for updating documentation within
# the tools.  The program takes no arguments, and if successful
# creates a file docfile<seq#>.txt in the current directory, where
# <seq#> is the current sequence number of the repository.

TOOLS=( "adms 0" "fastcap 0" "fasthenry 0" "mozy 0" "mrouter 0" \
    "vl 0" "wrspice 0" "xic 0" )

# Handle the arguments.
# Take one tool name, we get all the commit messages since the last
# release of the tool.
for a in $*; do
    i=0
    found=0
    for t in "${TOOLS[@]}"; do
        set -- $t
        tool=$1
        doit=$2
        if [ $tool == $a ]; then
            if [ $doit == 0 ]; then
                TOOLS[$i]="$tool 1"
            fi
            found=1
            break
        fi
        i=$(($i+1))
    done
    if [ $found == 0 ]; then
        echo "Error: unknown argument $a given, exiting."
        exit 1
    fi
    break  # just one arg
done

# Get the repository sequence number.
repo_seq=$(./release.sh -s)
if [ -z "$repo_seq" ]; then
    echo "Error getting sequence number for repo."
    exit 1
fi

if [ -n "$tool" ]; then
    prev_cid=$(./$tool/release.sh -c)
    prev_tag=$(./$tool/release.sh -r)
    if [ -z "$prev_cid" ]; then
        prev_cid=$(./$tool/release.sh -lc)
        prev_tag=$(./$tool/release.sh -lr)
    fi
else
    prev_cid=$(./release.sh -c)
    prev_tag=$(./release.sh -t)
    if [ -z "$prev_cid" ]; then
        prev_cid=$(./release.sh -lc)
        prev_tag=$(./release.sh -lt)
    fi
fi
if [ -z "$prev_cid" ]; then
    echo "Error getting commit id for last release."
    exit 1
fi

IFS="."
set -- $prev_tag
docfile="docfile$3.txt"
IFS=

# Extract all git log entries since the last release.
git log $prev_cid..HEAD > $docfile

echo "Done.  Commit messages saved in file $docfile."

