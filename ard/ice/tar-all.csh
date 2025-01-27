#!/bin/csh

./clean.csh
./prepare.csh

set cur_dir=`pwd|awk -F\/ '{print $(NF-0)}'`
#set date_label=`date +"%Y-%m-%d"`
#set host_label=`hostname`
#set tar_name=ard-$host_label-$date_label.tar.gz

set tar_name=ard.t

cd ..

if ( -f $tar_name ) then
    \rm $tar_name
endif

tar czvf $tar_name  $cur_dir --exclude "gui/images"

echo "====================================================="
#tar -tvf $tar_name | awk '{printf("%3.1fK\t%s\t%s\n", $3/1024, $4, $6)}'
tar -tvf $tar_name |sort -nk 3 | awk '{printf("%3.1fK\t%s\t%s\n", $3/1024, $4, $6)}'
echo "====================================================="

./ard/bin/encr.csh $tar_name
\rm $tar_name
