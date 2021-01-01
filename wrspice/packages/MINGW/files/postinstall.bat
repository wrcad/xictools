@echo off

@rem   Batch file to create symbolic links after installation, executed
@rem   by the installer program.

mklink /j wrspice %cd%\wrspice.current
mklink bin\wrspice.bat %cd%\wrspice\bin\wrspice.bat
mklink bin\multidec.exe %cd%\wrspice\bin\multidec.exe
mklink bin\proc2mod.exe %cd%\wrspice\bin\proc2mod.exe
mklink bin\printtoraw.exe %cd%\wrspice\bin\printtoraw.exe
mklink bin\wrspiced.exe %cd%\wrspice\bin\wrspiced.exe

