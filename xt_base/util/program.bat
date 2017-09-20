@echo off
@rem   ---------------------------------------------------------------------
@rem   PROGRAM.BAT:  Command file to start XicTools programs.

@rem   This allows the programs to start "out of the box", without any
@rem   further setup.  This is used as a wrapper to call programs
@rem   that require DLLs from the gtk2-bundle, to avoid adding
@rem   the gtk2-bundle/bin to the search path.

@rem   INSTRUCTIONS
@rem   1.  If you haven't done so, install the gtk2-bundle package.  This
@rem       contains DLLs and other files needed by the XicTools programs.
@rem       This script will not work unless the gtk2-bundle is installed.

@rem   2.  Copy this file to a new name.
@rem       The name of this file must be the same as a Whiteley
@rem       Research program, e.g., "xic", "wrspice", etc.  The extension
@rem       should be "bat".  Example:  xic.bat
@rem       Do this for each of your XicTools programs that require
@rem       DLLs from the bundle.  These are xic and wrspice, plus
@rem       mozy, xeditor, and httpget from the accessories.
@rem       These .bat files are automatically installed in recent
@rem       distributions.

@rem   The programs can then be started by executing the .bat files
@rem   These files can reside anywhere.

@rem   Feel free to hack this script as needed.
@rem   ---------------------------------------------------------------------

@rem   ---------------------------------------------------------------------
@rem   Set the progname variable to the base name of this script file.
@rem   For example, if this file is named "foo.bat", progname will be
@rem   set to "foo".

set progname=%~n0
@rem   ---------------------------------------------------------------------

@rem   ---------------------------------------------------------------------
@rem   We need a special name as stored in the Registry.  This is set by
@rem   the installer.

if %progname%==xic (
    set appname=xic_is1
) else (
    if %progname%==wrspice (
        set appname=wrspice_is1
    ) else (
        if %progname%==httpget (
            set appname=mozy_is1
        ) else (
            if %progname%==mozy (
                set appname=mozyis1
            ) else (
                if %progname%==xeditor (
                    set appname=mozy_is1
                ) else {
                    set appname=gtk2-bundle_is1
                )
            )
        )
    )
)

@rem   ---------------------------------------------------------------------
@rem   Set the prefix variable to the InstallPrefix saved in the Registry.
@rem   This is where the user installed the program.  This is rather opaque
@rem   and magical.  The default is c:\usr\local\

@rem   If the reg command fails, assume it needs "/reg:32" which is true
@rem   for Win7/8 64 bits.

set reg=

@rem   This is used by inno-5.5.9
set key=HKLM\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall

reg query %key%\%appname% /v InstallLocation > NUL 2>&1
if ERRORLEVEL 1 set reg=/reg:32
reg query %key%\%appname% /v InstallLocation > NUL 2>&1
if ERRORLEVEL 1 (
    echo Error:  %progname% installation location not found in registry.
    exit
)

for /f "Tokens=2,*" %%A in (
    'reg query %key%\%appname% /v InstallLocation %reg%'
) do (
    set prefix=%%B
)

@rem   Do this again for the gtk2-bundle, in case it was installed under
@rem   a different prefix.  Look in the old Registry location, too, so we
@rem   can work with the earlier gtk-bundle installed with inno-5.5.1.

set appname=gtk2-bundle_is1
reg query %key%\%appname% /v InstallLocation  %reg% > NUL 2>&1
if ERRORLEVEL 1 (
@rem   This is used by inno-5.5.1
    set key=HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall
)
reg query %key%\%appname% /v InstallLocation  %reg% > NUL 2>&1
if ERRORLEVEL 1 (
    echo Error: gtk2-bundle installation location not found in registry.
    exit
)

for /f "Tokens=2,*" %%A in (
    'reg query %key%\%appname% /v InstallLocation %reg%'
) do (
    set bundle_prefix=%%B
)
@rem   ---------------------------------------------------------------------

@rem   ---------------------------------------------------------------------
@rem   Locally modify the search path so that the program can find
@rem   the gtk2-bundle DLLs, before any others in the PATH.

setlocal
PATH=%bundle_prefix%gtk2-bundle\bin;%PATH%
@rem   ---------------------------------------------------------------------

@rem   ---------------------------------------------------------------------
@rem   Execute the program, with the same arguments (if any) that were
@rem   given to this script.

if %progname%==xic (
    "%prefix%xictools\xic\bin\xic.exe" %*
) else (
    if %progname%==wrspice (
        "%prefix%xictools\wrspice\bin\wrspice.exe" %*
    ) else (
        "%prefix%xictools\bin\%progname%.exe" %*
    )
)
    
endlocal
@rem   ---------------------------------------------------------------------
@rem   End of script.

