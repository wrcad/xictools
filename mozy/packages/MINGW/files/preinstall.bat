@echo off

@rem   Delete the links associated with the application installation.
@rem   This is called by the installer before installation, and after
@rem   deinstallation.

@rem   Remove links.
@rem   Seems that you must use rmdir rather than del to remove a link
@rem   created with mklink /j.  These can be identified with a directory
@rem   test "if exist link\".  We also blow away regular directories or
@rem   files that would conflict.

@rem   rd /s /q is safe for links, and will completely remove a regular
@rem   directory.

set prog=mozy

if exist bin\%prog%.exe\ ( rmdir bin\%prog%.exe
) else ( if exist bin\%prog%.exe del bin\%prog%.exe )
if exist bin\%prog%.bat ( del bin\%prog%.bat )
if exist bin\%prog% ( del bin\%prog% )
if exist bin\xeditor.exe\ ( rmdir bin\xeditor.exe
) else ( if exist bin\xeditor.exe del bin\xeditor.exe )
if exist bin\xeditor.bat ( del bin\xeditor.bat )
if exist bin\xeditor ( del bin\xeditor )
if exist bin\hlpsrv.exe\ ( rmdir bin\hlpsrv.exe
) else ( if exist bin\hlpsrv.exe del bin\hlpsrv.exe )
if exist bin\hlp2html.exe\ ( rmdir bin\hlp2html.exe
) else ( if exist bin\hlp2html.exe del bin\hlp2html.exe )
if exist bin\httpget.exe\ ( rmdir bin\httpget.exe
) else ( if exist bin\httpget.exe del bin\httpget.exe )

