@echo off
setlocal EnableDelayedExpansion

@rem   Usage:  ./cleanold.bat [-t]

@rem   This will remove all pre-4.3 gen4 packages.  The user is prompted
@rem   whether to retain existing current Xic and WRspice installations
@rem   in the Safe Install format, allowing reversion.

@rem   If -t given, print the commands that would be executed, but don't
@rem   actually run them.

set dryrun=no
if x%1==x-t (
    set dryrun=yes
)

for %%i in (Xic,WRspice,XtAccs,Xtlserv) do (call :action %%i)
goto :eof

:action
@rem   Gen-4 magic name for inno installer.
set appname=%1-4_is1

@rem   32-bit app in a 64-bit registry view, e.g., Cygwin64 or native64.
set key=HKLM\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall

reg query %key%\%appname% /v UninstallString > NUL 2>&1
if ERRORLEVEL 1 (
@rem   32-bit app in a 32-bin registtry view, e.g., Cygwin32, or 32-bit
@rem   Windows if there is such a thing anymore.
    set key=HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall
)
reg query %key%\%appname% /v UninstallString > NUL 2>&1
if ERRORLEVEL 1 (
    goto :eof
)

for /f "Tokens=2,*" %%A in (
    'reg query %key%\%appname% /v UninstallString'
) do (
    set ucmd=%%~B
)
if -%ucmd%-==-- (
    echo Failed to find uninstall function for %1
    goto :eof
)

if %1==Xic (
    set /P inp=Enter y to save existing Xic installation:  
    echo xx !inp!
    if x!inp!==xy (
        call :save xic %ucmd% %dryrun% wrdecode wrencode wrsetpass
    )
)
if %1==WRspice (
    set /P inp=Enter y to save existing WRspice installation:  
    if x!inp!==xy (
        call :save wrspice %ucmd% %dryrun% multidec proc2mod printtoraw wrspiced
    )
)

echo Removing %1 installation
if %dryrun%==yes (
    echo %ucmd%
) else (
    %ucmd%
)
goto :eof

:save
set prog=%1
set ucmd=%2
set dryrun=%3

@rem   Strip \%prog%\uninstall\unins000.exe
if %prog%==xic (
    set loc=%ucmd:~0,-27%
) else (
    set loc=%ucmd:~0,-31%
)
for /f "Tokens=1-3" %%a in (
    '%loc%\bin\%prog%.exe --v'
) do (
    set version=%%a
)

if not x%version%==x (
    if exist %loc%\%prog%-%version% (
        if %dryrun%==yes (
            echo rd /s /q %loc%\%prog%-%version%
        ) else (
            rd /s /q %loc%\%prog%-%version%
        )
    )
    if %dryrun%==yes (
        echo xcopy /s /i /q %loc%\%prog% %loc%\%prog%-%version%
        echo rd /s /q %loc%\%prog%-%version%\uninstall
        echo md %loc%\%prog%-%version%\bin
        echo copy %loc%\bin\%prog%.exe %loc%\%prog%-%version%\bin
        echo copy %loc%\bin\%prog%.dll %loc%\%prog%-%version%\bin
        echo copy %loc%\bin\%prog%.bat %loc%\%prog%-%version%\bin
        if not -%4-==-- (
            echo copy %loc%\bin\%4.exe %loc%\%prog%-%version%\bin
        )
        if not -%5-==-- (
            echo copy %loc%\bin\%5.exe %loc%\%prog%-%version%\bin
        )
        if not -%6-==-- (
            echo copy %loc%\bin\%6.exe %loc%\%prog%-%version%\bin
        )
        if not -%7-==-- (
            echo copy %loc%\bin\%7.exe %loc%\%prog%-%version%\bin
        )
    ) else (
        xcopy /s /i /q %loc%\%prog% %loc%\%prog%-%version%
        rd /s /q %loc%\%prog%-%version%\uninstall
        md %loc%\%prog%-%version%\bin
        copy %loc%\bin\%prog%.exe %loc%\%prog%-%version%\bin
        copy %loc%\bin\%prog%.dll %loc%\%prog%-%version%\bin
        copy %loc%\bin\%prog%.bat %loc%\%prog%-%version%\bin
        if not -%4-==-- (
            copy %loc%\bin\%4.exe %loc%\%prog%-%version%\bin
        )
        if not -%5-==-- (
            copy %loc%\bin\%5.exe %loc%\%prog%-%version%\bin
        )
        if not -%6-==-- (
            copy %loc%\bin\%6.exe %loc%\%prog%-%version%\bin
        )
        if not -%7-==-- (
            copy %loc%\bin\%7.exe %loc%\%prog%-%version%\bin
        )
    )
)
goto :eof

