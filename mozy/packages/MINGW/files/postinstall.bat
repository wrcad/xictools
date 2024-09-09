@echo off

@rem   Batch file to create symbolic links after installation, executed
@rem   by the installer program.

if not exist bin\ (
    if exist bin ( del bin )
    mkdir bin
)
mklink bin\hlpsrv.exe %cd%\xic\bin\hlpsrv.exe
mklink bin\help2html.exe %cd%\xic\bin\help2html.exe
mklink bin\httpget.exe %cd%\xic\bin\httpget.exe
copy %cd%\xic\bin\mozy.bat bin
copy %cd%\xic\bin\mozy.sh bin\mozy
copy %cd%\xic\bin\xeditor.bat bin
copy %cd%\xic\bin\xeditor.sh bin\xeditor

