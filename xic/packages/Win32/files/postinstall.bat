@echo off

@rem   Batch file to create symbolic links after installation, executed
@rem   by the installer program.

mklink /j xic %cd%\xic.current
mklink bin\xic.bat %cd%\xic\bin\xic.bat
mklink bin\wrdecode.exe %cd%\xic\bin\wrdecode.exe
mklink bin\wrencode.exe %cd%\xic\bin\wrencode.exe
mklink bin\wrsetpass.exe %cd%\xic\bin\wrsetpass.exe

