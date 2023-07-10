# esp-idf-japan-weather
Display the Japanese weather on M5Stack.   
M5Stackに週間天気予報を表示します。   

![screen](https://user-images.githubusercontent.com/6020549/204073294-3b156449-15d1-4385-9a6a-83b627e464c8.JPG)


Yahoo Japanが提供する天気予報サイトは2022年3月末で提供を終了しました。   
そこで、[こちら](https://api.aoikujira.com/index.php?tenki)のサイトから週間天気予報を取り出しています。   
このサイトもいつまで天気予報を提供してくれるか分かりません。   
気象庁が天気予報をJSON形式で提供していますが、3日分しかありません。   
精度は高くなくていいので、週間天気予報が欲しいです。   
土日に洗濯できるかどうか、週間天気予報を見て決めています。   

# Hardware requirements
M5Stack   


# Installation

```
git clone https://github.com/nopnop2002/esp-idf-japan-weather
cd esp-idf-japan-weather
chmod 777 getpem.sh
./getpem.sh
idf.py set-target esp32
idf.py menuconfig
idf.py flash monitor
```


# Configuration
menuconfigを使用してこれらを設定する必要があります。   

- CONFIG_ESP_WIFI_SSID   
WiFIのSSID
- CONFIG_ESP_WIFI_PASSWORD   
Wifiのパスワード
- CONFIG_ESP_MAXIMUM_RETRY   
Wi-Fiに接続するときの最大再試行回数
- CONFIG_ESP_LOCATION   
対象の地域
- CONFIG_ESP_FONT   
使用するフォント

![config-1](https://user-images.githubusercontent.com/6020549/204073332-f5ce6734-1a55-4abc-84d5-007c4e62b177.jpg)
![config-2](https://user-images.githubusercontent.com/6020549/204073334-c0f52ba4-3756-470f-a90e-714f33f39362.jpg)
![config-3](https://user-images.githubusercontent.com/6020549/204073342-2a154bd6-16c4-4f35-a826-17197c1ac5a6.jpg)
![config-4](https://user-images.githubusercontent.com/6020549/204073343-d4885877-812c-4bfa-974c-1dbe6790b39b.jpg)


# Operation
左ボタンを押すと再起動します。   
毎時０分に再起動して情報を更新します。   

# JSON Library   
ESP-IDE標準のSONライブラリを使用しています。   


# Font File   
[こちら](http://ayati.cocolog-nifty.com/blog/2012/08/index.html)で公開されているFONTX形式のフォントファイル(ILFONT03.zip)を使っています。   
オリジナルのフォントはIPA(文字情報技術促進協議会)が公開している[IPAフォント](https://moji.or.jp/ipafont/)です。   
ライセンスはIPAフォントのライセンスに従います。   
漢字コードはSJISです。   


# Convert from UTF8 to SJIS   
天気予報サイトの文字コードにはUTF8が使われています。   
フォントファイルにはSJISのコードでフォントが格納されています。   
そこで、[こちら](https://www.mgo-tec.com/blog-entry-utf8sjis01.html)で公開されている変換テーブル(Utf8Sjis.tbl)を使って、
UTF8からSJISにコードを変換し、変換結果のSJISコードを使ってフォントを取り出しています。   

