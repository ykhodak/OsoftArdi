REM   win-rard.bat - windows shell script to run program
REM   in console with logging console output into log file (see %logf%)
REM   usually used in autotest/QA procedures

set logf=C:\tmp\r-res.log

if exist %logf% (del %logf% /q)
if defined ARD_HOME_BUILD (
set PATH=C:\Qt\Qt5.4.0\5.4\msvc2013\bin;C:\projects\ariadne\3rd-party\LIB3RD\win32-2013-debug
set exe=C:/projects/ariadne/ard/gui/__tmp/Ardi.exe
) else (
set PATH=%PATH%;C:\Qt\5.1.0\msvc2010\bin;C:\projects\ariadne\3rd-party\LIB3RD\win32-2010-debug
set exe=C:\projects\ariadne\ard\gui\__tmp\testapp.exe
)

REM --------------------------------------------------
REM you can setup path to QT binaries, 3rd party libs 
REM & program executable %exe% here is needed 
REM --------------------------------------------------

if exist %exe% (
 echo "running "%exe%
 call %exe% > %logf% 2>&1
) else (
 echo "executable file not found "%exe%
)

