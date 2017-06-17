#! /bin/sh

# Whiteley Research Inc., No Copyright, Public Domain
# $Id: licinfo.sh,v 1.1 2016/01/10 21:00:52 stevew Exp $

# Find the machine's host name and a key, generate an XtLicenseInfo file.
# For Whiteley Research product licensing on Unix/Linux/OS X.

# Instructions
# 1.  Make sure that this file is executable.  From a terminal window
#     in the directory containing this file, give the command
#       chmod 755 licinfo.sh
# 2.  Run the licinfo.sh script with the command
#       ./licinfo.sh
# 3.  The host name and key, as saved in the XtLicenseInfo file, can be
#     used to obtain a license file from Whiteley Research.

# Get the host name and strip off any domain.
hostname=`hostname`
IFS=.
set $hostname
hostname=$1

if [ $hostname = localhost ]; then
    echo The generic host name \"localhost\" is not accepted.
    echo You must assign a host name to the machine.
    exit
fi

if [ `uname -s` = Darwin ]; then
    # Apple, use the machine serial number as the key.

    snum=`ioreg -l | awk '/IOPlatformSerialNumber/ { print $4; }'`
    key=`echo $snum | sed s/\"//g`
else
    # Use the ethernet address as the key.

    hwline=`/sbin/ip link | grep link/ether`
    IFS=" "
    set -- $hwline
    key=$2
fi

echo Succeeded > XtLicenseInfo
echo Host Name: $hostname >> XtLicenseInfo
echo Key: $key >> XtLicenseInfo

echo
echo Host Name: $hostname
echo Key: $key
echo
echo XtLicenseInfo file created
echo

