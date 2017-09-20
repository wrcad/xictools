@echo off

@rem   Batch file to create symbolic links after installation, executed
@rem   by the installer program.

mklink /j xic xic.current
mklink /j bin\xic.bat xic\bin\xic.bat
mklink /j bin\wrdecode.exe xic\bin\wrdecode.exe
mklink /j bin\wrencode.exe xic\bin\wrencode.exe
mklink /j bin\wrsetpass.exe xic\bin\wrsetpass.exe

