@echo off

@rem   Batch file to create symbolic links after installation, executed
@rem   by the installer program.

mklink /j xic %cd%\xic.current
if not exist bin\ (
    if exist bin ( del bin )
    mkdir bin
)
@rem mklink bin\xic.exe %cd%\xic\bin\xic.exe
@rem mklink bin\xic.dll %cd%\xic\bin\xic.dll
mklink bin\wrdecode.exe %cd%\xic\bin\wrdecode.exe
mklink bin\wrencode.exe %cd%\xic\bin\wrencode.exe
mklink bin\wrsetpass.exe %cd%\xic\bin\wrsetpass.exe
copy %cd%\xic\bin\xic.bat bin
copy %cd%\xic\bin\xic.sh bin\xic

