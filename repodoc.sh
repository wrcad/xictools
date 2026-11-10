#! /bin/bash

# This can only be run from the git repository!!!
#
# repodoc.sh -- utility to cat the log text since the previous commit
# into a file.  This may be useful for updating documentation within
# the tools.  The program takes no arguments, and if successful
# creates a file docfile<seq#>.txt in the current directory, where
# <seq#> is the current sequence number of the repository.

# Get the repository sequence number.
repo_seq=$(./release.sh -s)
if [ -z "$repo_seq" ]; then
    echo "Error getting sequence number for repo."
    exit 1
fi
prev_cid=$(./release.sh -lc)
if [ -z "$prev_cid" ]; then
    echo "Error getting commit id for last release."
    exit 1
fi

docfile="docfile$repo_seq.txt"

# Extract all git log entries since the last release.
git log $prev_cid..HEAD > $docfile

echo "Done.  Commit messages saved in file $docfile."

