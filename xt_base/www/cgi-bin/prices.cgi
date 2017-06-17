#! /bin/sh

echo -n -e "Content-type: text/html\r\n\r\n"

if [ x$QUERY_STRING = x ]; then
    sed 's/@KEY@//;s/@HOSTNAME@//;s/@Msw@//;s/@LINUX@//;s/@OSX@//;s/@KEYTEXT@//;s/@EMAIL@//;s/@INFO@//'\
 < /home/webadmin/wrcad.com/www/prices.in
else
    /home/webadmin/wrcad.com/www/keyform $QUERY_STRING
fi

