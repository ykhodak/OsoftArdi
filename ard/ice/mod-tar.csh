#!/bin/csh

./clean.csh
./prepare.csh

set days_test = $1
if($days_test == "")then
    set days_test = 5
endif

set date_label=`date +"%Y-%m-%d"`
set host_label=`hostname`
set tar_name=../mods-$host_label-$date_label.tar.gz

echo "====================================================="

tar -czvf $tar_name  `find . -mtime -$days_test \( -name "*.cpp" -or -name "*.h" -or -name "*.pro" -or -name "*.csh" -or -name "*.bash" -or -name "*.txt" -or -name "*.qrc" -or -name "*.bat" -or -name "*.json" \) | grep -v __tmp`

echo "====================================================="
tar -tvf $tar_name |sort -nk 3 | awk '{printf("%3.1fK\t%s\t%s\n", $3/1024, $4, $6)}'
split -b 140000 $tar_name ../mseg
echo "====================================================="

echo "older then "$days_test "days.." `ls -lsh $tar_name | awk '{print $1}'`

