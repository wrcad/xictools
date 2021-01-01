@echo off

@rem   Delete the links associated with the application installation.
@rem   This is called by the installer before installation, and after
@rem   deinstallation.

@rem   Remove links.
@rem   Seems that you must use rmdir rather than del to remove a link
@rem   created with mklink /j.  These can be identified with a directory
@rem   test "if exist link\".  We also blow away regular directories or
@rem   files that would conflict.

set prog=xic

@rem   We don't set these, but they might be around from an earlier
@rem   installation that wasn't properly removed.
if exist bin\%prog%.exe del bin\%prog%.exe
if exist bin\%prog%.dll del bin\%prog%.dll

@rem   rd /s /q is safe for links, and will completely remove a regular
@rem   directory.
if exist %prog%\ rmdir /s /q %prog%

@rem   This is the only main executable we export, exe and dll are
@rem   not in the search path.
if exist bin\%prog%.bat\ ( rmdir bin\%prog%.bat
) else ( if exist bin\%prog%.bat del bin\%prog%.bat )

@rem   Xic-specific exported utilities.
if exist bin\wrdecode.exe\ ( rmdir bin\wrdecode.exe
) else ( if exist bin\wrdecode.exe del bin\wrdecode.exe )
if exist bin\wrencode.exe\ ( rmdir bin\wrencode.exe
) else ( if exist bin\wrencode.exe del bin\wrencode.exe )
if exist bin\wrsetpass.exe\ ( rmdir bin\wrsetpass.exe
) else ( if exist bin\wrsetpass.exe del bin\wrsetpass.exe )

@rem   Copy to backup for Safe Install.

if exist %prog%.current\bin\%prog%.exe (
    for /f "Tokens=1-3" %%a in (
        '%prog%.current\bin\%prog%.exe --v'
    ) do (
        set version=%%a
    )
)

if not x%version%==x (
    if exist %prog%-%version% (
        rd /s /q %prog%-%version%
    )
    xcopy /s /i /q %prog%.current %prog%-%version%
    rd /s /q %prog%-%version%/uninstall
)

