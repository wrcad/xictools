#! /bin/bash

# This is a back-end sourced in the release.sh scripts found in the
# tool directories.  It contains the core code to interact with the
# git database while performing various operations, such as finding
# the commit tags for the tool releases, and assisting in automating
# the check-in process for new tool releases.  Check-ins should be
# performed with the update.sh script found in the repository root
# directory, so users may not need to directly run the release.sh
# scripts.
#
# These can only be run from the git repository!!!

# The scripts take at most one argument.
# With SEARCH_TAGS_DIRECT=yes  (called from repo root)
# -s   Print the saved repo sequence number.
# -t   Print the repo tag string for the repo sequence number.
# -c   Print the commit id if repo tag created.
# -v   Print the repo version string if repo tag created. 
# -ls  print the last sequence number (one less than current).
# -lt  Print the last repo tag string.
# -lc  Print the last commit id.
# -lv  Print the last version string.
# int  Print the version string for given sequence number if it exists.
#
# Otherwise  (called from app roots)
# -s   Print the app sequence number.
# -t   Print the app tag string for the app sequence number.
# -r   Print the repo tag string if repo tag created.
# -c   Print the commit id if repo tag created.
# -v   Print the app version string if repo tag created. 
# -ls  print the last sequence number (one less than current).
# -lt  Print the last app tag string.
# -lr  Print the last repo tag string for app.
# -lc  Print the last commit id for app.
# -lv  Print the last app version string for app.
# int  Print the version string for given sequence number if it exists.

# Formats:
#   PRENUM          each tool has this defined to two integers as X.Y
#   appname         xic, wrspice, etc., same as the subdirectory name
#   tag string      appname-PRENUM.sequence_number
#   example         xic-4.3.24
#   version srtring tagstring-datecode-commithash
#   example         xic-4.3.15-4120071026-8591744a
#   The datecode is an encoding of the git commit date in UTC,
#   MMHHDDMMYY, and the final token is the short commit hash in the
#   git repository where the version string is saved in the tag
#   message.

# The following variables must be assigned in the code that sources
# this file.  PRENUM is the major.minor part of the tag and version
# strings.  This is kept fixed and we append a sequence number to
# identify releases.  The SEARCH_TAGS_DIRECT=yes is found only in
# the call at the top level (in the repo root) which works with tags
# of the repo directly.
#PRENUM="4.3"
#PRETOK="xic-$PRENUM"
#sequence_number="16"
#SEARCH_TAGS_DIRECT=yes

# The caller should handle the no-argument case.  Multiple arguments
# after the first are ignored.

if [[ -n "$1" && "$1" == "-s" ]]; then
    echo $sequence_number
    exit 0
fi
prev_seq=$(($sequence_number - 1))
if [[ -n "$1" && "$1" == "-ls" ]]; then
    echo $prev_seq
    exit 0
fi

cur_tag=$PRETOK.$sequence_number
if [[ -n "$1" && "$1" == "-t" ]]; then
    echo $cur_tag
    exit 0
fi
prev_tag=$PRETOK.$prev_seq
if [[ -n "$1" && "$1" == "-lt" ]]; then
    echo $prev_tag
    exit 0
fi

if [[ -n "$SEARCH_TAGS_DIRECT" && "$SEARCH_TAGS_DIRECT" == yes ]]; then
    # This is true when working from the repository root on the
    # repository itself.  In this case, tag strings reference actual
    # tags and not tokens stored in tag messages.

if [[ -n "$1" && "$1" = "-c" ]]; then
    if git rev-parse --verify --quiet "refs/tags/$cur_tag" \
            >/dev/null 2>&1; then
        cur_cid=$(git rev-parse --short $cur_tag^{})
        echo $cur_cid
        exit 0
    fi
    exit 1
fi
if [[ -n "$1" && "$1" = "-lc" ]]; then
    if git rev-parse --verify --quiet "refs/tags/$prev_tag" \
            >/dev/null 2>&1; then
        prev_cid=$(git rev-parse --short $prev_tag^{})
        echo $prev_cid
        exit 0
    fi
    exit 1
fi

if [[ -n "$1" && "$1" = "-v" ]]; then
    if git rev-parse --verify --quiet "refs/tags/$cur_tag" \
            >/dev/null 2>&1; then
        cur_cid=$(git rev-parse --short $cur_tag^{})
        dt=$(TZ=UTC0 git show -s --format=%cd $cur_cid \
            --date=format-local:'%M%H%d%m%y')
        echo "$PRETOK.$sequence_number-$dt-$cur_cid"
        exit 0
    fi
    exit 1
fi
if [[ -n "$1" && "$1" = "-lv" ]]; then
    if git rev-parse --verify --quiet "refs/tags/$prev_tag" \
            >/dev/null 2>&1; then
        prev_cid=$(git rev-parse --short $prev_tag^{})
        dt=$(TZ=UTC0 git show -s --format=%cd $prev_cid \
            --date=format-local:'%M%H%d%m%y')
        echo "$PRETOK.$prev_seq-$dt-$prev_cid"
        exit 0
    fi
    exit 1
fi

if [[ "$1" =~ ^[0-9]+$ ]]; then
    seq=$1
    tag=$PRETOK.$seq
    if git rev-parse --verify --quiet "refs/tags/$tag" \
            >/dev/null 2>&1; then
        cid=$(git rev-parse --short $tag^{})
        dt=$(TZ=UTC0 git show -s --format=%cd $cid \
            --date=format-local:'%M%H%d%m%y')
        echo "$PRETOK.$seq-$dt-$cid"
        exit 0
    fi
fi

else
    # Not SEARCH_TAGS_DIRECT
    # This branch is used when working in the tools directories, and
    # a tool release event is recorded by adding a token to the message
    # of a tag given to the commit being marked.

if [[ -n "$1" && "$1" = "-r" ]]; then
    for t in $(git tag -l); do
        st=$(git tag -l --format='%(contents)' $t | grep "@RELEASE $cur_tag@")
        if [ -n "$st" ]; then
            echo $t
            exit 0
        fi
    done
    exit 1
fi
if [[ -n "$1" && "$1" = "-lr" ]]; then
    for t in $(git tag -l); do
        st=$(git tag -l --format='%(contents)' $t | grep "@RELEASE $prev_tag@")
        if [ -n "$st" ]; then
            echo $t
            exit 0
        fi
    done
    exit 1
fi

if [[ -n "$1" && "$1" = "-c" ]]; then
    for t in $(git tag -l); do
        st=$(git tag -l --format='%(contents)' $t | grep "@RELEASE $cur_tag@")
        if [ -n "$st" ]; then
            cur_cid=$(git rev-parse --short $t^{})
            echo $cur_cid
            exit 0
        fi
    done
    exit 1
fi
if [[ -n "$1" && "$1" = "-lc" ]]; then
    for t in $(git tag -l); do
        st=$(git tag -l --format='%(contents)' $t | grep "@RELEASE $prev_tag@")
        if [ -n "$st" ]; then
            prev_cid=$(git rev-parse --short $t^{})
            echo $prev_cid
            exit 0
        fi
    done
    exit 1
fi

if [[ -n "$1" && "$1" = "-v" ]]; then
    for t in $(git tag -l); do
        st=$(git tag -l --format='%(contents)' $t | grep "@RELEASE $cur_tag@")
        if [ -n "$st" ]; then
            cur_cid=$(git rev-parse --short $t^{})
            dt=$(TZ=UTC0 git show -s --format=%cd $cur_cid \
                --date=format-local:'%M%H%d%m%y')
            echo "$PRETOK.$sequence_number-$dt-$cur_cid"
            exit 0
        fi
    done
    exit 1
fi
if [[ -n "$1" && "$1" = "-lv" ]]; then
    for t in $(git tag -l); do
        st=$(git tag -l --format='%(contents)' $t | grep "@RELEASE $prev_tag@")
        if [ -n "$st" ]; then
            prev_cid=$(git rev-parse --short $t^{})
            dt=$(TZ=UTC0 git show -s --format=%cd $prev_cid \
                --date=format-local:'%M%H%d%m%y')
            echo "$PRETOK.$prev_seq-$dt-$prev_cid"
            exit 0
        fi
    done
    exit 1
fi

if [[ "$1" =~ ^[0-9]+$ ]]; then
    seq=$1
    tag=$PRETOK.$seq
    for t in $(git tag -l); do
        st=$(git tag -l --format='%(contents)' $t | grep "@RELEASE $tag@")
        if [ -n "$st" ]; then
            cid=$(git rev-parse --short $t^{})
            dt=$(TZ=UTC0 git show -s --format=%cd $cid \
                --date=format-local:'%M%H%d%m%y')
            echo "$PRETOK.$seq-$dt-$cid"
            exit 0
        fi
    done
fi

fi
exit 1

