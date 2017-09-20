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

