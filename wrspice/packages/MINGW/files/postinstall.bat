@echo off

@rem   Batch file to create symbolic links after installation, executed
@rem   by the installer program.

mklink /j wrspice %cd%\wrspice.current
if not exist bin\ (
    if exist bin ( del bin )
    mkdir bin
)
mklink bin\wrspice.exe %cd%\wrspice\bin\wrspice.exe
mklink bin\wrspice.dll %cd%\wrspice\bin\wrspice.dll
mklink bin\mmjco.exe %cd%\wrspice\bin\mmjco.exe
mklink bin\multidec.exe %cd%\wrspice\bin\multidec.exe
mklink bin\proc2mod.exe %cd%\wrspice\bin\proc2mod.exe
mklink bin\printtoraw.exe %cd%\wrspice\bin\printtoraw.exe
mklink bin\wrspiced.exe %cd%\wrspice\bin\wrspiced.exe
copy %cd%\wrspice\bin\wrspice.bat bin

