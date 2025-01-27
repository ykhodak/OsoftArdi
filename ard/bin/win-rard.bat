REM   win-rard.bat - windows shell script to run program
REM   in console with logging console output into log file (see %logf%)
REM   usually used in autotest/QA procedures

set logf=C:\tmp\r-res.log
set exe=C:\ykh\ariadne\ard\gui\src\release\testapp.exe
REM set exe=C:\ykh\ariadne\ard\gui\src\debug\testapp.exe

REM if exist %logf% (del %logf% /q)
REM if defined ARD_HOME_BUILD (
REM set PATH=%PATH%;C:\tools\Qt\Qt5.8.0\5.8\msvc2015\bin;C:\projects\ariadne\3rd-party\LIB3RD\win32-2015-release
REM set exe=C:/projects/ariadne/ard/gui/__tmp/Ardi.exe
REM ) else (
REM set PATH=%PATH%;C:\tools\Qt\Qt5.8.0\5.8\msvc2015\bin;C:\projects\ariadne\3rd-party\LIB3RD\win32-2015-release
REM set exe=C:\projects\ariadne\ard\gui\__tmp\testapp.exe
REM )

REM --------------------------------------------------
REM you can setup path to QT binaries, 3rd party libs 
REM & program executable %exe% here is needed 
REM --------------------------------------------------

set PATH=%PATH%;C:\tools\Qt\Qt5.8.0\5.8\msvc2015\bin;C:\ykh\ariadne\3rd-party\LIB3RD\win32-2015-release
echo %PATH%


if exist %exe% (
 echo "running "%exe%
 call %exe% > %logf% 2>&1
) else (
 echo "executable file not found "%exe%
)

