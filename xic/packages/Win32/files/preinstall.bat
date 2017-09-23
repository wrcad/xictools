@echo off

@rem   Delete the links associated with the application installation.
@rem   This is called by the installer before installation, and after
@rem   deinstallation.

@rem   Remove links.
@rem   Seems that you must use rmdir rather than del to remove a link
@rem   created with mklink /j.

rmdir xic
rmdir bin\xic.bat
rmdir bin\wrdecode.exe
rmdir bin\wrencode.exe
rmdir bin\wrsetpass.exe

@rem   Copy to backup for Safe Install.

set prog=xic
for /f "Tokens=1-3" %%a in (
    '%prog%.current\bin\%prog%.exe --v'
) do (
    set version=%%a
)

if not x%version%==x (
    if exist %prog%-%version% (
        rd /s /q %prog%-%version%
    )
    xcopy /s /i /q %prog%.current %prog%-%version%
    rd /s /q %prog%-%version%/uninstall
)

