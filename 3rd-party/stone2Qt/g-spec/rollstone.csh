#!/bin/csh

set P=python
if ( -f python3 ) then
  P=python3
endif


foreach m("gmail" "gtask" "gdrive" "gcontact")
    setenv STONE_MODULE_ROOT $m
    $P -m stone.cli -a :all ../qgen/q4s.stoneg.py . ./$m/*.stone
end

