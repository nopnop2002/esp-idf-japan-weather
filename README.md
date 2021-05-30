# esp-idf-japan-weather
Display the Japanese weather on M5Stack.   
M5Stackに[Yahoo Japan](https://weather.yahoo.co.jp/weather/rss/)が提供する天気予報を表示します。   

![Gothic-1](https://user-images.githubusercontent.com/6020549/89112965-b33c7e80-d4a5-11ea-9073-dc72e9c05fb3.JPG)

---

# Hardware requirements
M5Stack

---

# Install

```
git clone https://github.com/nopnop2002/esp-idf-japan-weather
cd esp-idf-japan-weather
chmod 777 getpem.sh
./getpem.sh
make menuconfig
make flash monitor
```

---

# Firmware configuration
menuconfigを使用してこれらを設定する必要があります。   

- CONFIG_ESP_WIFI_SSID   
WiFIのSSID
- CONFIG_ESP_WIFI_PASSWORD   
Wifiのパスワード
- CONFIG_ESP_MAXIMUM_RETRY   
Wi-Fiに接続するときの最大再試行回数
- CONFIG_ESP_LOCATION   
[こちら](https://weather.yahoo.co.jp/weather/rss/)のページから地域を選び、表示されるURLの末尾の番号を指定します。   
例えば愛知県名古屋市の場合   
https://rss-weather.yahoo.co.jp/rss/days/5110.xml
なので、5110を設定します。   
- CONFIG_ESP_FONT   
使用するフォント

![menuconfig-1](https://user-images.githubusercontent.com/6020549/89112941-753f5a80-d4a5-11ea-804b-29ca13a7c793.jpg)
![menuconfig-2](https://user-images.githubusercontent.com/6020549/89112942-76708780-d4a5-11ea-89d6-559b30ecd3a8.jpg)

---

# Operation

## View1
初期表示   
M5Stackの左ボタンを押すとこの表示になります。   
上：Gothicフォント　下：Mincyoフォント   

![Gothic-1](https://user-images.githubusercontent.com/6020549/89112965-b33c7e80-d4a5-11ea-9073-dc72e9c05fb3.JPG)
![Mincyo-1](https://user-images.githubusercontent.com/6020549/89112966-b59ed880-d4a5-11ea-8e0f-59491781d64d.JPG)

## View2
M5Stackの真ん中のボタンを押すとこの表示になります。   
上：Gothicフォント　下：Mincyoフォント   

![Gothic-2](https://user-images.githubusercontent.com/6020549/89112975-d0714d00-d4a5-11ea-9292-a207a8839244.JPG)
![Mincyo-2](https://user-images.githubusercontent.com/6020549/89112976-d1a27a00-d4a5-11ea-98f3-987cafd4a648.JPG)

## Update
M5Stackの右ボタンを押すとRSSからデータを再度取得します。

---

# XML Parser   
[こちら](https://libexpat.github.io/)のライブラリを使用しています。   
このライブラリはesp-idf標準ライブラリです。   

---

# Font File   
[こちら](http://ayati.cocolog-nifty.com/blog/2012/08/index.html)で公開されているFONTX形式のフォントファイル(ILFONT03.zip)を使っています。   
オリジナルのフォントはIPA(文字情報技術促進協議会)が公開している[IPAフォント](https://moji.or.jp/ipafont/)です。   
漢字コードはSJISです。   

---

# Convert from UTF8 to SJIS   
Yahoo JapanのRSSでは文字コードにUTF8が使われています。   
[こちら](https://www.mgo-tec.com/blog-entry-utf8sjis01.html)で公開されている変換テーブル(Utf8Sjis.tbl)を使って、
UTF8からSJISに変換しています。

