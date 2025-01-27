#!/usr/bin/bash

find . -name "*.cpp" -or -name "*.h"| grep -v moc_ | grep -v qrc_ | xargs ls -la | awk '{SUM += $5} END {print SUM " bytes"}'
find . -name "*.cpp" -or -name "*.h" | grep -v moc_ | grep -v qrc_ | xargs ls | xargs wc -l | grep total | awk '{print $1 " lines"}'
echo `find . -name "*.h" -or -name "*.cpp" | xargs grep class | grep -v \; | grep -v template | wc -l` " classes"
echo `find . -name "*.h" -or -name "*.cpp" | xargs grep virtual | wc -l` " virtuals"
echo `find . -name "*.h" -or -name "*.cpp" | xargs grep // | wc -l` " comments"
echo `find . -name "*.h" -or -name "*.cpp" | xargs grep @todo | wc -l` " todos"
