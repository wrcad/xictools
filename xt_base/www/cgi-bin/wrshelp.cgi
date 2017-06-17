#! /bin/sh

export HLPSRV_CGIPATH="/cgi-bin/wrshelp.cgi?h="
export HLPSRV_IMPATH="/help-images/"
export HLPSRV_PATH="/home/webadmin/wrcad.com/html/restricted/helplib/wrspice/help"

IFS='='
set $QUERY_STRING
/usr/local/bin/hlpsrv $2

