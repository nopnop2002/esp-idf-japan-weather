# esp-idf-japan-weather
Display the Japanese weather on M5Stack.   
M5Stacに[Yahoo Japan](https://weather.yahoo.co.jp/weather/rss/)が提供する天気予報を表示します。   

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
- CONFIG_ESP_FONT   
使用するフォント

![menuconfig-1](https://user-images.githubusercontent.com/6020549/89112941-753f5a80-d4a5-11ea-804b-29ca13a7c793.jpg)
![menuconfig-2](https://user-images.githubusercontent.com/6020549/89112942-76708780-d4a5-11ea-89d6-559b30ecd3a8.jpg)

---

# Operation

## View1
初期表示   
M5Stackの左ボタンを押すとこの表示になります。   

![Gothic-1](https://user-images.githubusercontent.com/6020549/89112965-b33c7e80-d4a5-11ea-9073-dc72e9c05fb3.JPG)
![Mincyo-1](https://user-images.githubusercontent.com/6020549/89112966-b59ed880-d4a5-11ea-8e0f-59491781d64d.JPG)

## View2
M5Stackの真ん中のボタンを押すとこの表示にしなります。   

![Gothic-2](https://user-images.githubusercontent.com/6020549/89112975-d0714d00-d4a5-11ea-9292-a207a8839244.JPG)
![Mincyo-2](https://user-images.githubusercontent.com/6020549/89112976-d1a27a00-d4a5-11ea-98f3-987cafd4a648.JPG)

## Update
M5Stackの右ボタンを押すとRSSからデータを再度取得します。

---

# Font File   
[こちら](https://ipafont.ipa.go.jp/old/ipafont/download.html)で公開されているFONTX形式のフォントファイルを使っています。   
漢字コードはSJISです。   

---

# Convert from UTF8 to SJIS   
Yahoo JapanのRSSでは文字コードにUTF8が使われています。   
[こちら](https://www.mgo-tec.com/blog-entry-utf8sjis01.html)で公開されている変換テーブル(Utf8Sjis.tbl)を使って、
UTF8からSJISに変換しています。

