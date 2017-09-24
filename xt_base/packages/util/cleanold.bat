@echo off
@rem   Whiteley Research Inc., open source, Apache 2.0 license
@rem   Stephen R. Whiteley 9/23/2017

@rem   cleanold.bat
@rem   Windows batch script to clean old gen4 commercial releases,
@rem   to be called before installing XicTools-4.3.

setlocal EnableDelayedExpansion
goto :begin

:help
echo.
echo   Usage:  ./cleanold.bat [/t]
echo.
echo   This will remove all pre-4.3 gen4 packages.  The user is prompted
echo   whether to retain existing current Xic and WRspice installations
echo   in the Safe Install format, allowing reversion.
echo.
echo   If -t given, print the commands that would be executed, but don't
echo   actually run them.
echo.
goto :eof

:begin

@rem   Accept /t and -t for dryrun option.
set dryrun=no
if x%1==x/t set dryrun=yes
if x%1==x-t set dryrun=yes

@rem   If any unexpected option, print help and exit.  If no options,
@rem   ask user to confirm before proceding.
if %dryrun%==no (
    if not -%1-==-- (
        call :help
        exit
    )
    set /P ret=Enter y to confirm and remove old packages:  
    if not x!ret!==xy (
        echo Not confirmed, aborting.
        exit
    )
) else (
    if not -%2-==-- (
        call :help
        exit
    )
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
@rem   32-bit app in a 32-bin registry view, e.g., Cygwin32, or 32-bit
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
    if %dryrun%==yes (
        set /P inp=Enter y if you would save the existing Xic installation:  
    ) else (
        set /P inp=Enter y to save the existing Xic installation:  
    )
    if x!inp!==xy (
        call :save xic %ucmd% %dryrun% wrdecode wrencode wrsetpass
    )
)
if %1==WRspice (
    if %dryrun%==yes (
        set /P inp=Enter y if you would save the existing WRspice installation:  
    ) else (
        set /P inp=Enter y to save the existing WRspice installation:  
    )
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
if %1==Xic (
    call :final xic %ucmd% $dryrun%
)
if %1==WRspice (
    call :final wrspice %ucmd% $dryrun%
)
goto :eof

:save
@rem   Save xic/wrspice directories as xic-version, wrspice-version.
@rem   Also create a bin subdirectory and copy the files from xictools/bin.
@rem   The program can be reverted to with a symbolic link, se for the
@rem   xictools-4.3 releases.
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

:final
@rem   The installer may not entirely remove xic and wrspice directories.
@rem   This finishes the job.
set prog=%1
set ucmd=%2
set dryrun=%3

@rem   Strip \%prog%\uninstall\unins000.exe
if %prog%==xic (
    set loc=%ucmd:~0,-27%
) else (
    set loc=%ucmd:~0,-31%
)
if exist %loc%\%prog% (
    if %dryrun%==yes (
        echo rd /s /q %loc%\%prog%
    ) else (
        rd /s /q %loc%\%prog%
    )
)
goto :eof

