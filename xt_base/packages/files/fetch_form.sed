<html>
<!-- $Id: fetch_form.sed,v 1.11 2015/07/19 20:05:38 stevew Exp $ -->
<body bgcolor=white>

<form method=post action=DIREC/util/fetch_action>

<table>
<tr><td width=150>
<b>Distributions</b><br>
<input type=checkbox name=Darwin64 value=Darwin64>Darwin64<br>
<input type=checkbox name=LinuxRHEL5 value=LinuxRHEL5>LinuxRHEL5<br>
<input type=checkbox name=LinuxRHEL5_64 value=LinuxRHEL5_64>LinuxRHEL5_64<br>
<input type=checkbox name=LinuxRHEL6_64 value=LinuxRHEL6_64>LinuxRHEL6_64<br>
<input type=checkbox name=LinuxRHEL7_64 value=LinuxRHEL7_64>LinuxRHEL7_64<br>
<input type=checkbox name=Windows value=Windows>Windows<br>
<!--
<input type=checkbox name=Darwin value=Darwin>Darwin<br>
<input type=checkbox name=Debian value=Debian>Debian<br>
<input type=checkbox name=Debian64 value=Debian64>Debian64<br>
<input type=checkbox name=FreeBSD value=FreeBSD>FreeBSD<br>
<input type=checkbox name=FreeBSD7 value=FreeBSD7>FreeBSD7<br>
<input type=checkbox name=Linux value=Linux>Linux<br>
<input type=checkbox name=Linux2 value=Linux2>Linux2<br>
<input type=checkbox name=LinuxRHEL3 value=LinuxRHEL3>LinuxRHEL3<br>
<input type=checkbox name=LinuxRHEL3_64 value=LinuxRHEL3_64>LinuxRHEL3_64<br>
<input type=checkbox name=Solaris value=Solaris>Solaris<br>
<input type=checkbox name=Solaris8 value=Solaris8>Solaris8<br>
<input type=checkbox name=Solaris8_64 value=Solaris8_64>Solaris8_64<br>
-->
</td>

<td>
<b>Programs</b><br>
<!-- <input type=checkbox name=devkit value=devkit>devkit<br> -->
<input type=checkbox name=wrspice value=wrspice>wrspice<br>
<input type=checkbox name=xic value=xic>xic<br>
<input type=checkbox name=xtlserv value=xtlserv>xtlserv<br>
<input type=checkbox name=xt_accs value=xt_accs>xt_accs<br>
<!--
<input type=checkbox name=xicii value=xicii>xicii<br>
<input type=checkbox name=xiv value=xiv>xiv<br>
-->
</td></tr>
</table>

<input type=submit value="Fetch ">
</form>

</body>
</html>
