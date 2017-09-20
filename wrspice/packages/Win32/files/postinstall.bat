@echo off

@rem   Batch file to create symbolic links after installation, executed
@rem   by the installer program.

mklink /j wrspice wrspice.current
mklink /j bin\wrspice.bat wrspice\bin\wrspice.bat
mklink /j bin\multidec.exe wrspice\bin\multidec.exe
mklink /j bin\proc2mod.exe wrspice\bin\proc2mod.exe
mklink /j bin\printtoraw.exe wrspice\bin\printtoraw.exe
mklink /j bin\wrspiced.exe wrspice\bin\wrspiced.exe

