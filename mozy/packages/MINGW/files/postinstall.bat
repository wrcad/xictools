@echo off

@rem   Batch file to create symbolic links after installation, executed
@rem   by the installer program.

if not exist bin\ (
    if exist bin ( del bin )
    mkdir bin
)
mklink bin\hlpsrv.exe %cd%\mozy\bin\hlpsrv.exe
mklink bin\hlp2html.exe %cd%\mozy\bin\hlp2html.exe
mklink bin\httpget.exe %cd%\mozy\bin\httpget.exe
copy %cd%\mozy\bin\mozy.bat bin
copy %cd%\mozy\bin\mozy.sh bin\mozy
copy %cd%\mozy\bin\xeditor.bat bin
copy %cd%\mozy\bin\xeditor.sh bin\xeditor

