#!/bin/bash

set -x

#openssl s_client -showcerts -connect weather.yahoo.co.jp:443 </dev/null >hoge
openssl s_client -showcerts -connect api.aoikujira.com:443 </dev/null >hoge


start=`grep -e "-----BEGIN CERTIFICATE-----" -n hoge | sed -e 's/:.*//g' | tail -n 1`

last=`grep -e "-----END CERTIFICATE-----" -n hoge | sed -e 's/:.*//g' | tail -n 1`

sed -n ${start},${last}p hoge > main/cert.pem

rm hoge
