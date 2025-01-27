#!/usr/bin/bash

echo "enter password:"
read -s pass
echo "repead password:"
read -s pass_confirm

if [ $pass != $pass_confirm ]; then
    echo "passwords don't match"
    exit 1
fi

dfile=$1
openssl aes-256-cbc -salt -in $dfile -out $dfile".aes" -md md5 -pass pass:$pass
echo "encrypted.."
