@echo off

# Usage:  ./cleanold.bat [-t]
#
# This will remove all pre-4.3 packages, but the user is given the
# option of saving existing Xic/WRspice installations into the Safe
# Install framework.

if -%1-==-- (
    echo
    echo Usage:  cleanold.bat [-t]
    echo
    echo This will remove all pre-4.3 packages, but the user is given the
    echo option of saving existing Xic/WRspice installations into the Safe
    echo Install framework.
    echo
    echo If -t given, print the commands that would be executed, but don't
    echo actually run them.
    exit
)

set dryrun=no
if %1==-t (
    set dryrun=yes
)

@rem   Old existing packages used inno-5.5.1
set key=HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall

for %%name in (Xic,WRspice,XtAccs,Xtlserv) do (

    set appname=%name%_is1
    set reg=

    reg query %key%\%appname% /v UninstallString > NUL 2>&1
    if ERRORLEVEL 1 set reg=/reg:32
    reg query %key%\%appname% /v UninstallString > NUL 2>&1
    if ERRORLEVEL 1 (
        goto advance
    )

    for /f "Tokens=2,*" %%A in (
        'reg query %key%\%appname% /v UninstallString %reg%'
    ) do (
        set ucmd=%%B
    )
    if -%ucmd%-==-- (
        echo Failed to find uninstall function for %1
        goto advance
    )

    echo %ucmd%
    if %dryrun%==no call %ucmd%

    :advance
)
