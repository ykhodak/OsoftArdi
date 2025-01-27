#!/usr/bin/bash

./clean.bash
./prepare.bash

cur_dir=`pwd|awk -F\/ '{print $(NF-0)}'`
tar_name=ard.t

cd ..

if [ -f $tar_name ]; then
    \rm $tar_name
fi

tar czvf $tar_name  $cur_dir --exclude "gui/images"

echo "====================================================="
#tar -tvf $tar_name | awk '{printf("%3.1fK\t%s\t%s\n", $3/1024, $4, $6)}'
tar -tvf $tar_name |sort -nk 3 | awk '{printf("%3.1fK\t%s\t%s\n", $3/1024, $4, $6)}'
echo "====================================================="

./ard/bin/encr.bash $tar_name
\rm $tar_name
