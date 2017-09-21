@echo off

@rem   Delete the links associated with the application installation.
@rem   This is called by the installer before installation, and after
@rem   deinstallation.

@rem   Seems that you must use rmdir rather than del to remove a link
@rem   created with mklink /j.

rmdir wrspice
rmdir bin\wrspice.bat
rmdir bin\multidec.exe
rmdir bin\proc2mod.exe
rmdir bin\printtoraw.exe
rmdir bin\wrspiced.exe

@rem   Save backup for Safe Install.

for /f "Tokens=1-3 delim= " %%A in (
    'wrspice.current/bin/wrspice.exe --v'
) do (
    set version=%%A
)

if (x%version%!=x) then (
    rd /s /q wrspice-%version%
    xcopy /s /i /q wrspice.current wrspice-%version%
)

