#!/bin/bash

#set -x

openssl s_client -showcerts -connect weather.yahoo.co.jp:443 </dev/null >hoge

start=`grep -e "-----BEGIN CERTIFICATE-----" -n hoge | sed -e 's/:.*//g' | tail -n 1`

last=`grep -e "-----END CERTIFICATE-----" -n hoge | sed -e 's/:.*//g' | tail -n 1`

sed -n ${start},${last}p hoge > main/weather_yahoo_cert.pem

rm hoge
