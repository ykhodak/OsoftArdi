#!/bin/bash

if [ $# -ne 1 ]; then 
    echo "usage search4ice.sh <search-directory>"
    echo "script will search directory for source files not listed in 'pro'"
    exit
fi

if [ ! -d "$1" ]; then
    echo "source directory not found:"$1
    exit
fi

cd $1
echo "entering directory $1"

for p in `find . -name "*.pro"`
do
    echo "project: $p"
    for i in `find . -name "*.h" -exec basename {} \;`
    do
	r=`cat $p|grep $i`
	if [ -z "$r" ]; then
	    echo "no $i in $p"
	fi
    done

    for i in `find . -name "*.cpp" -exec basename {} \;`
    do
	r=`cat $p|grep $i`
	if [ -z "$r" ]; then
	    echo "no $i in $p"
	fi
    done
done
