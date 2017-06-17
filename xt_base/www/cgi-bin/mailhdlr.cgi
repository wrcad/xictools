#! /bin/sh

mydomain="wrcad.com"

echo "Content-type: text/html"
echo
echo "<html><body background=\"images/tmbg.gif\" TEXT=\"#000000\" \
 LINK=\"#9c009E\" VLINK=\"#551A8B\" ALINK=\"#FF0000\">"
echo "<table border=1 cellpadding=4 height=40>"
echo "<tr><td bgcolor=\"#ffffff\"><center><h3>Click"

IFS='='
set $QUERY_STRING

echo "<a href=\"mailto:$2@$mydomain\">HERE</a>"
echo "to send email</h3></center></td></tr>"
echo "<tr><td bgcolor=\"#ffffe0\"><center>"
echo "<h3>Why does this page exist?</h3></center>"
echo "Spammers search web pages for <b>mailto</b> tags and extract the"
echo "email addresses for use as spam targets.  Thus, <b>mailto</b> tags"
echo "have been removed from our pages, and instead mail is handled by a"
echo "<b>cgi</b> script that generates this page."
echo "<p>"
echo "<b>Tip for web page authors:</b> spammers can also extract email address"
echo "from other page text.  To avoid this, never put valid email addresses"
echo "in the form \"someone@mybusiness.com\" in your pages.  Instead, use"
echo "an encoded form such as"
echo "\"someone&#38;&#35;64&#59;mybusiness.c&#38;&#35;111&#59;m\" which is"
echo "much harder to detect with a simple search, and looks the same as the"
echo "standard form on-screen."
echo "</td></tr></table>"
echo "</body></html>"
echo

