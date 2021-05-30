# esp-idf-japan-weather
Display the Japanese weather on M5Stack.   
M5Stack��[Yahoo Japan](https://weather.yahoo.co.jp/weather/rss/)���񋟂���V�C�\���\�����܂��B   

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
menuconfig���g�p���Ă�����ݒ肷��K�v������܂��B   

- CONFIG_ESP_WIFI_SSID   
WiFI��SSID
- CONFIG_ESP_WIFI_PASSWORD   
Wifi�̃p�X���[�h
- CONFIG_ESP_MAXIMUM_RETRY   
Wi-Fi�ɐڑ�����Ƃ��̍ő�Ď��s��
- CONFIG_ESP_LOCATION   
[������](https://weather.yahoo.co.jp/weather/rss/)�̃y�[�W����n���I�сA�\�������URL�̖����̔ԍ����w�肵�܂��B   
�Ⴆ�Έ��m�����É��s�̏ꍇ   
https://rss-weather.yahoo.co.jp/rss/days/5110.xml
�Ȃ̂ŁA5110��ݒ肵�܂��B   
- CONFIG_ESP_FONT   
�g�p����t�H���g

![menuconfig-1](https://user-images.githubusercontent.com/6020549/89112941-753f5a80-d4a5-11ea-804b-29ca13a7c793.jpg)
![menuconfig-2](https://user-images.githubusercontent.com/6020549/89112942-76708780-d4a5-11ea-89d6-559b30ecd3a8.jpg)

---

# Operation

## View1
�����\��   
M5Stack�̍��{�^���������Ƃ��̕\���ɂȂ�܂��B   
��FGothic�t�H���g�@���FMincyo�t�H���g   

![Gothic-1](https://user-images.githubusercontent.com/6020549/89112965-b33c7e80-d4a5-11ea-9073-dc72e9c05fb3.JPG)
![Mincyo-1](https://user-images.githubusercontent.com/6020549/89112966-b59ed880-d4a5-11ea-8e0f-59491781d64d.JPG)

## View2
M5Stack�̐^�񒆂̃{�^���������Ƃ��̕\���ɂȂ�܂��B   
��FGothic�t�H���g�@���FMincyo�t�H���g   

![Gothic-2](https://user-images.githubusercontent.com/6020549/89112975-d0714d00-d4a5-11ea-9292-a207a8839244.JPG)
![Mincyo-2](https://user-images.githubusercontent.com/6020549/89112976-d1a27a00-d4a5-11ea-98f3-987cafd4a648.JPG)

## Update
M5Stack�̉E�{�^����������RSS����f�[�^���ēx�擾���܂��B

---

# XML Parser   
[������](https://libexpat.github.io/)�̃��C�u�������g�p���Ă��܂��B   
���̃��C�u������esp-idf�W�����C�u�����ł��B   

---

# Font File   
[������](http://ayati.cocolog-nifty.com/blog/2012/08/index.html)�Ō��J����Ă���FONTX�`���̃t�H���g�t�@�C��(ILFONT03.zip)���g���Ă��܂��B   
�I���W�i���̃t�H���g��IPA(�������Z�p���i���c��)�����J���Ă���[IPA�t�H���g](https://moji.or.jp/ipafont/)�ł��B   
�����R�[�h��SJIS�ł��B   

---

# Convert from UTF8 to SJIS   
Yahoo Japan��RSS�ł͕����R�[�h��UTF8���g���Ă��܂��B   
[������](https://www.mgo-tec.com/blog-entry-utf8sjis01.html)�Ō��J����Ă���ϊ��e�[�u��(Utf8Sjis.tbl)���g���āA
UTF8����SJIS�ɕϊ����Ă��܂��B

