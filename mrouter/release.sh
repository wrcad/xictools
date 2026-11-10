#! /bin/bash

# This can only be run from the git repository!!!

# This release is in sync with qrouter-1.3.62.

# Here is where the release number is defined.  Presently, although
# the LDDB and MRouter could be compiled as separate components, we
# merge all code and build a common library.  Thus, for now we need
# only one release version.
#
# ***** IMPORTANT *****
#
# The version is in the form major.minor.release.
# >  The 'major' value is fixed at 1 for now.
# >  The 'minor' value is the "interface level" and is incremented if the
#    exported headers materially change.
# >  The 'release" value is incremented for bug fixes and code changes
#    that don't affect the interface.
#
# The application using the plug-in should check the major and minor
# value and disallow loading if mismatched.  Testing of the release
# value is optional.  If mismatched, features may be missing, but
# there should not be faults due to interface differences.

# The current Interface level is 2.  The present release has been
# merged into the XicTools source tree and makes use of xt_base both
# for configuration and utilities.  The previoulsy existing
# configuration script and local utilities port were discarded.

# release.sh script for mrouter
PRENUM="1.2"
PRETOK="mrouter-$PRENUM"
sequence_number="8"

# If no argument, print the tag string with appname stripped.
if [[ -z "$1" ]]; then
    echo $PRENUM.$sequence_number
    exit 0
fi


# See this file for documentation about the arguments to and use of
# this script.
dir=$(dirname "$(realpath "$0")")
source "$dir/../xt_base/util/release-sub.sh"

