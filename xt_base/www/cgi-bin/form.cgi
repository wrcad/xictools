#!/usr/bin/perl

require ('/home/webadmin/wrcad.com/cgi-bin/cgi-lib.pl');

# This should match the mail program on your system.
$mailprog = '/usr/sbin/sendmail';

$mydomain = "wrcad.com";
$recipient = "feedback\@$mydomain";

$text = "text";
$email = "email";
$pcode = "pcode";
$passcode = "4H718";

# Print out a content-type for HTTP/1.0 compatibility
print "Content-type: text/html\r\n\r\n";

# Print a title and initial heading
print "<Head><Title>Feedback Form</Title></Head>";

&ReadParse;

if ($#in_key ne 2) {
  print "ABUSE DETECTED go away!\n";  
  exit 0;  
}

foreach $i (0 .. $#in_key){
  $in_val[$i] =~ s/\r//ge;
  $in_val[$i] =~ s/\n/"\n# "/ge;
  if ($in_key[$i] eq $text) {
    if ($in_val[$i] eq "body") {
      print "ABUSE DETECTED go away!\n";
      exit 0;
    }
  }
  elsif ($in_key[$i] eq $email) {   
    @tv = split(' ', $in_val[$i], 2);
    if ($tv[0] eq "") {
      print "BAD EMAIL ADDRESS (none given)\n";
      exit 0;
    }
    if ($tv[1] ne "") {
      print "BAD EMAIL ADDRESS (multiple tokens)\n";
      exit 0;
    }
  }
  elsif ($in_key[$i] eq $pcode) {   
    @pv = split(' ', $in_val[$i], 2);
    if ($pv[0] eq "") {
      print "NO PASSCODE GIVEN\n";
      exit 0;
    }
    if ($pv[0] ne $passcode) {
      print "BAD PASSCODE\n";
      exit 0;
    }
  }
  else {
    print "ABUSE DETECTED go away!\n";  
    exit 0;  
  }
}

open (MAIL, "|$mailprog $recipient") || die "Unable to send request\n\
 Please send e-mail to $recipient, Thank you\n";
#open (MAIL, ">test") || die "Cannot open STDOUT: $!\n"; 
print MAIL "Reply-to: $in{$email}\n";
print MAIL "Subject: Feedback Form\n";
print MAIL "\n\n";
#print "*$#in_key\n";
foreach $i (0 .. $#in_key){
  if ($in_key[$i] ne $pcode) {
    print MAIL "$in_key[$i] = $in_val[$i]\n"; 
  }
}
print MAIL "-----------------------------------------------\n\n\n";
close (MAIL);

print "Your message has been sent, thanks.<P>";
print '<A HREF="/index.html"><H3>Return to Home Page</H3></A>';
print "\n"

