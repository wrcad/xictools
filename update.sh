#! /bin/bash

# update.sh -- A tool for managing tool and tool set releases.
#
# Usage:  ./update.sh [-u] [-d] [adms] [fastcap] ...
# This can be run ONLY from within the xictools git repository.
#
# The keywords used as arguments must match the words below.
# If -u is given, the most recent indicators for a release of
# the tools named will be deleted.
# Otherwise, an indicator will be added for the tools listed
# for a new release.  If -d is given, the steps taken will be
# explained but not actually performed, no change will be made
# to the database in this case.

TOOLS=( "adms 0" "fastcap 0" "fasthenry 0" "mozy 0" "mrouter 0" \
    "vl 0" "wrspice 0" "xic 0" )

dryrun=""
unupd=""

# This script requires the release.sh scripts found in this directory
# and the tools directories.
if [ ! -x ./release.sh ]; then
    echo "Can't find the release.sh script in the current directory."
    echo "Fatal error, exiting."
    exit 1
fi

# Handle the arguments.
for a in $*; do
    if [ "$a" == -d ]; then
        dryrun=yes
        continue
    fi
    if [ "$a" == -u ]; then
        unupd=yes
        continue
    fi
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
done

if [[ -n "$unupd" && "$unupd" == yes ]]; then
    # Option -u: undo the previous release recording.
    # For each given tool, 1) erase the @RELEASE@ indicator in the
    # corresponding repo tag message, and 2) decrement the tool
    # sequence count.  Presently, we DO NOT commit the file changes
    # at this point.  XXX should we commit?
    #
    # Note: one can't change old commit messages without changing all
    # the hashes which can be a huge mess.  That is why we save this
    # info in annotated tag text, which can be freely modified.
    # git tag <tag-name> <tag-name>^{} -f -m "Your new tag message"

for t in "${TOOLS[@]}"; do
    set -- $t
    tool=$1
    doit=$2
    if [[ -n "$doit" && "$doit" == 1 ]]; then
        file=$tool/release.sh
        tooltag=$($file -lt)
        if [ -z "$tooltag" ]; then
            echo "\'$file -lt\' failed, skipping."
            continue;
        fi
        cid=$($file -lc)
        if [ -z "$cid" ]; then
            echo "\'$file -lc\' failed, skipping."
            continue;
        fi
        echo "Removing $tooltag from commit tag message."
        repotag=$($file -lr)
        if [ -z "$repotag" ]; then
            echo "\'$file -lr\' failed, skipping."
            continue;
        fi
        tagtext=$(git tag -l --format='%(contents)' $repotag)
        if [[ -z "$tagtext" ]]; then
            echo "\'$file -lr\' failed, skipping."
            continue;
        fi

        newtt=$(echo $tagtest |sed -e s/@RELEASE $tooltag@// )
        if [[ -n "$newtt" && "$newtt" != "$tagtext" ]]; then
            git tag $repotag $repotag^{} -f -m "$newtt"
        fi
        # We have removed the label from the tag text, now decrement
        # the sequence number.
        echo "Decrementing sequence_number in $file."

        if [[ -n "$dryrun" && "$dryrun" == yes ]]; then 
            echo awk -F'"' '/^sequence_number="/ {OFS="\""; $2=$2-1} {print}' \
            '$file > $file.tmp && mv -f $file.tmp $file && chmod 755 $file'
        else
            awk -F'"' '/^sequence_number="/ {OFS="\""; $2=$2-1} {print}' \
            $file > $file.tmp && mv -f $file.tmp $file && chmod 755 $file
        fi
    fi
done
    exit 0
fi  # End of option -u.


# We want to start in the devel branch, so that the increment/decrement
# of the release.sh files goes there.
if [[ "$(git branch --show-current)" != "devel" ]]; then
    if [[ -n "$(git status --untracked-files=no --porcelain)" ]]; then
        # Current branch is dirty.
        echo "The current branch needs a commit!  It should start clean."
        echo "Exiting."
        exit 1
    fi
    git checkout devel
fi
# It is fine if the devel branch is dirty, we do a commit later.

# This is the repo tag to be used for the next commit.  If it is
# in use increment until the tag is unused.
repo_tag=$(./release.sh -t)
file=./release.sh
while [ -n "$(git rev-parse -q --verify "refs/tags/$repo_tag")" ]; do
    if [[ -n "$dryrun" && "$dryrun" == yes ]]; then 
        echo awk -F'"' '/^sequence_number="/ {OFS="\""; $2=$2+1} {print}' \
            '$file > $file.tmp && mv -f $file.tmp $file && chmod 755 $file'
        break
    else
        awk -F'"' '/^sequence_number="/ {OFS="\""; $2=$2+1} {print}' \
            $file > $file.tmp && mv -f $file.tmp $file && chmod 755 $file
        repo_tag=$(./release.sh -t)
    fi
done

# Get the repository sequence number.
repo_seq=$(./release.sh -s)
if [ -z "$repo_seq" ]; then
    echo "Error getting sequence number for repo."
    exit 1
fi
prev_seq=$(($repo_seq - 1))

# This is the short commit hash from the most recent repo commit from
# this script.
prev_cid=$(./release.sh -lc)
if [ -z "$prev_cid" ]; then
    echo "Error getting commit id for last release."
    exit 1
fi

docfile="docfile$repo_seq.txt"
echo "LAST CHANCE: Are you ready to commit?"
echo ".  Have you run repodoc.sh and used $docfile to update docs?"
echo ".  Are all updates complete and files ready for distribution?"
echo -n "Enter "yes" to proceed, anything else aborts: "
read rdval
if [[ -z "$rdval" || "$rdval" != yes ]]; then
    echo "Exiting."
    exit 0
fi

# User has affirmed that all needed changes have been made, and we
# are ready to mark the repo for a distribution.  This is done by
# adding an annotated tag to the commit from a final check-in, whose
# text a contains tokens the store the tool tags for the releases
# to be marked here.

# Cycke through the active tools.  While the tool sequence number is
# already registered for a release, increment it.  Otherwise, add the
# tool tag to the commit message.
commit_msg=
for t in "${TOOLS[@]}"; do
    set -- $t
    tool=$1
    doit=$2
    if [[ -z "$doit" || "$doit" != "1" ]]; then
        continue
    fi
    file=$tool/release.sh
    while [ -n "$($tool/release.sh -c)" ]; do
        # The tool sequence number is alredy in use, increment it.
        if [[ -n "$dryrun" && "$dryrun" == yes ]]; then 
            echo awk -F'"' '/^sequence_number="/ {OFS="\""; $2=$2+1} {print}' \
            '$file > $file.tmp && mv -f $file.tmp $file && chmod 755 $file'
            break
        else
            awk -F'"' '/^sequence_number="/ {OFS="\""; $2=$2+1} {print}' \
            $file > $file.tmp && mv -f $file.tmp $file && chmod 755 $file
            echo "Incremented sequence_number in $file"
        fi
    done
    tooltag=$($tool/release.sh -t)
    commit_msg="$commit_msg @RELEASE $tooltag@"
    echo "Added tag $tooltag to message."
done
if [[ -z "$commit_msg" ]]; then
    commit_msg="no tool release"
fi

# Check in changes.
echo "Committing changes into devel."
if [[ -n "$dryrun" && "$dryrun" == yes ]]; then 
    echo git commit -a -m "$commit_msg"
else
    git commit -a -m "$commit_msg"
fi

# Tag the HEAD with the repo_tag string.
# Note that we also added text to the commit message, so the tag is
# possibly redundant, but the tag cuts down the number of commits we
# have to search.  We know that $repo_tag is unused.
#
echo "Tagging the repo with $repo_tag, message $commit_msg"
if [[ -n "$dryrun" && "$dryrun" == yes ]]; then 
    echo "git tag -a -m \"New sequence number $repo_seq, $commit_msg\" \
$repo_tag" 
else
    git tag -a -m "Sequence number $repo_seq, $commit_msg" $repo_tag 
fi

# Switch to master, merge devel into master.
echo "Switch to master, merge devel into master."
if [[ -n "$dryrun" && "$dryrun" == yes ]]; then 
    echo git checkout master
    echo git merge devel
else
    git checkout master
    git merge devel
fi

# This completes to operation.  The master branch can be used to generate
# release packages.
#
# Here are some guidelines:
# 1. The distributions are always prepared from the master branch.
# 2. All tools in the master branch must be production quality.
# 3. Xic and WRspice share common code.  It is likely that both
#    tools will be released on the same schedule.  Otherwise if one
#    of these tools is released, the other must include support
#    for any changes to the common code.  It is best to give the
#    second program a release number if there are any significant
#    changes made.

