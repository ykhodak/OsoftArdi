#!/bin/csh

set doxyout = _a-docs

if( -d $doxyout ) then
    \rm -rf $doxyout
endif


doxygen doc/doxy.cfg
tar czvf a-docs.tar.gz $doxyout

