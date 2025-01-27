#!/bin/csh

set logf = r-res.log

if( -e $logf ) then
  echo "removing "./$logf
  \rm $logf
endif


set exe = $HOME/projects/a-test/ard/gui/__tmp/testapp
if ($?ARD_HOME_BUILD) then
    set exe = $HOME/projects/ariadne/ard/gui/__tmp/Ardi
endif


if ($?ARD_EXE) then
    set exe = $ARD_EXE
    echo "using env variable ARD_EXE as executable path "$ARD_EXE
endif

$exe >& $logf &
echo "started with log "$logf
