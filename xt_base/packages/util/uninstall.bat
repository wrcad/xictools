@echo off

@rem   Uninstall script for Microsoft Windows.

if -%1-==-- (
    echo
    echo Usage:  uninstall [-t] prog ...
    echo
    echo Arguments are program names from the following:
    echo adms fastcap fasthenry mozy mrouter vl wrspice xic xtlserv
    echo
    echo Programs listed after -t won't be uninstalled, but the command
    echo that would otherwise run is printed.
    exit
)

set dryrun=no

:start
if -%1-==-- exit

set appname=
if %1==-t (
    set dryrun=yes
    goto :advance
)
if %1==adms set appname=adms_is1
if %1==fastcap set appname=fastcap_is1
if %1==fasthenry set appname=fasthenry_is1
if %1==mozy set appname=mozy_is1
if %1==mrouter set appname=mrouter_is1
if %1==xtlserv set appname=xtlserv_is1
if %1==vl set appname=vl_is1
if %1==wrspice set appname=wrspice_is1
if %1==xic set appname=xic_is1

if -%appname-==-- (
    echo Unown program %1, ignoring.
    goto :advance
)

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
    echo %1 installation location not found in registry.
    goto :advance
)

for /f "Tokens=2,*" %%A in (
    'reg query %key%\%appname% /v UninstallString'
) do (
    set ucmd=%%B
)
if -%ucmd%-==-- (
    echo Failed to find uninstall function for %1
    goto :advance
)

echo %ucmd%
if %dryrun%==no call %ucmd%

:advance
shift
goto :start

