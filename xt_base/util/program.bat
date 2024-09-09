@echo off
@rem   ---------------------------------------------------------------------
@rem   PROGRAM.BAT:  Command file to start XicTools programs.

@rem   This allows the programs to be run from outside of the MSYS2
@rem   environment.
@rem   These .bat files are automatically installed in recent
@rem   distributions.

@rem   INSTRUCTIONS
@rem   Copy this file to a new name.
@rem   The name of this file must be the same as a Whiteley
@rem   Research program, e.g., "xic", "wrspice", etc.  The extension
@rem   should be "bat".  Example:  xic.bat
@rem   Do this for each of your XicTools programs that require
@rem   graphical DLLs from MSYS2.  These are xic and wrspice, plus
@rem   mozy, xeditor.
@rem   Edit prefix and other entries below if necessary.  These will
@rem   need changing if you installed MSYS2 or the programs in non-
@rem   default locations.  The grpref selects GTK or Qt graphics
@rem   package, the right side must be one of GTK2 or QT6.

set prefix=c:\usr\local
set mingw_dll_path=c:\msys64\mingw64\bin
set grpref=GTK2

@rem   The programs can then be started by executing the .bat files
@rem   These files can reside anywhere.

@rem   Feel free to hack this script as needed.
@rem   ---------------------------------------------------------------------


set progname=%~n0

@rem   ---------------------------------------------------------------------
@rem   Locally modify the search path so that the program can find
@rem   the MINGW DLLs, before any others in the PATH.

setlocal
PATH=%mingw_dll_path%;%PATH%

@rem   ---------------------------------------------------------------------
@rem   Execute the program, with the same arguments (if any) that were
@rem   given to this script.

%prefix%\xictools\%progname%\bin\%grpref%\%progname%.exe %*

endlocal
@rem   ---------------------------------------------------------------------
@rem   End of script.

