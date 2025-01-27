#!/usr/bin/bash

##################################
# asseble splitted files back
#
#  cat mseg* > m.tgz
##################################

##################################
# reset files modified in the future
#
# find . -type f -newermt "0 days" | xargs touch
#################################


find . -type d -exec chmod 700 {} \; >& /dev/null
find . -type f -exec chmod 600 {} \; >& /dev/null
find . -type f -name "*.csh" -exec chmod 700 {} \; >& /dev/null
find . -type f -name "*.bat" -exec chmod 700 {} \; >& /dev/null

find snc/include -name "*.h" -exec chmod 400 {} \; >& /dev/null
find snc/src -name "*.cpp" -exec chmod 400 {} \; >& /dev/null


