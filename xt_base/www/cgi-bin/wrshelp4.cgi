#! /bin/sh

export HLPSRV_CGIPATH="/cgi-bin/wrshelp4.cgi?h="
export HLPSRV_IMPATH="/help-images/"
export HLPSRV_PATH="/home/webadmin/wrcad.com/html/restricted/helplib4/wrspice/help"

IFS='='
set $QUERY_STRING
/usr/local/bin/hlpsrv -DWRspice $2

