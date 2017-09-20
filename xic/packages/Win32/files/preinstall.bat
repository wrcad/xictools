@echo off

@rem   Delete the links associated with the application installation.
@rem   This is called by the installer before installation, and after
@rem   deinstallation.

@rem   Seems that you must use rmdir rather than del to remove a link
@rem   created with mklink /j.

rmdir xic
rmdir bin\xic.bat
rmdir bin\wrdecode.exe
rmdir bin\wrencode.exe
rmdir bin\wrsetpass.exe

