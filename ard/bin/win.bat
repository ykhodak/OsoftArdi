@echo off
REM setlocal enableDelayedExpansion

set slnfile=../Ardi.sln
set PLATFORM=Win32

if defined ARD_DEBUG (
 set CONFIG=Debug
) else (
 set CONFIG=Release
)

qmake -version

if not exist "snc\src" (
    echo "snc\src not found, exiting.."
    goto SKIPB
)

cd snc/src/
echo.
qmake -tp vc
cd ../../
echo.
echo.
REM set /p CONFIRM="Please confirm configure & build of the projects (Y/N)>"

REM if NOT %CONFIRM% == Y goto SKIPB
echo "processing %CONFIRM%"

for %%p in (snc dbp exg) do (
if exist %%p/src (
    echo ------- %%p
    cd %%p/src/
    qmake -tp vc
    cd ../../
    msbuild.exe /p:Configuration=%CONFIG%;Platform=%PLATFORM% %slnfile% /t:%%p /maxcpucount:3
)
)

cd gui/src/
qmake -tp vc
cd ../../  

if defined ARD_HOME_BUILD (
        msbuild.exe /p:Configuration=%CONFIG%;Platform=%PLATFORM% %slnfile% /t:Ardi /maxcpucount:3
    ) else (
        msbuild.exe /p:Configuration=%CONFIG%;Platform=%PLATFORM% %slnfile% /t:testapp /maxcpucount:3
    )


:SKIPB
