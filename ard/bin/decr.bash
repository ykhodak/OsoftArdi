#!/usr/bin/bash

echo "enter password:"
read -s pass

dfile=$1
ofile=`basename $dfile .aes`
echo "$dfile -> $ofile"
openssl aes-256-cbc -d -salt -in $dfile -out $ofile -md md5 -pass pass:$pass
echo "decrypted.."
