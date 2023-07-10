# esp-idf-japan-weather
Display the Japanese weather on M5Stack.   
M5Stack�ɏT�ԓV�C�\���\�����܂��B   

![screen](https://user-images.githubusercontent.com/6020549/204073294-3b156449-15d1-4385-9a6a-83b627e464c8.JPG)


Yahoo Japan���񋟂���V�C�\��T�C�g��2022�N3�����Œ񋟂��I�����܂����B   
�����ŁA[������](https://api.aoikujira.com/index.php?tenki)�̃T�C�g����T�ԓV�C�\������o���Ă��܂��B   
���̃T�C�g�����܂œV�C�\���񋟂��Ă���邩������܂���B   
�C�ے����V�C�\���JSON�`���Œ񋟂��Ă��܂����A3������������܂���B   
���x�͍����Ȃ��Ă����̂ŁA�T�ԓV�C�\�񂪗~�����ł��B   
�y���ɐ���ł��邩�ǂ����A�T�ԓV�C�\������Č��߂Ă��܂��B   

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
menuconfig���g�p���Ă�����ݒ肷��K�v������܂��B   

- CONFIG_ESP_WIFI_SSID   
WiFI��SSID
- CONFIG_ESP_WIFI_PASSWORD   
Wifi�̃p�X���[�h
- CONFIG_ESP_MAXIMUM_RETRY   
Wi-Fi�ɐڑ�����Ƃ��̍ő�Ď��s��
- CONFIG_ESP_LOCATION   
�Ώۂ̒n��
- CONFIG_ESP_FONT   
�g�p����t�H���g

![config-1](https://user-images.githubusercontent.com/6020549/204073332-f5ce6734-1a55-4abc-84d5-007c4e62b177.jpg)
![config-2](https://user-images.githubusercontent.com/6020549/204073334-c0f52ba4-3756-470f-a90e-714f33f39362.jpg)
![config-3](https://user-images.githubusercontent.com/6020549/204073342-2a154bd6-16c4-4f35-a826-17197c1ac5a6.jpg)
![config-4](https://user-images.githubusercontent.com/6020549/204073343-d4885877-812c-4bfa-974c-1dbe6790b39b.jpg)


# Operation
���{�^���������ƍċN�����܂��B   
�����O���ɍċN�����ď����X�V���܂��B   

# JSON Library   
ESP-IDE�W����SON���C�u�������g�p���Ă��܂��B   


# Font File   
[������](http://ayati.cocolog-nifty.com/blog/2012/08/index.html)�Ō��J����Ă���FONTX�`���̃t�H���g�t�@�C��(ILFONT03.zip)���g���Ă��܂��B   
�I���W�i���̃t�H���g��IPA(�������Z�p���i���c��)�����J���Ă���[IPA�t�H���g](https://moji.or.jp/ipafont/)�ł��B   
���C�Z���X��IPA�t�H���g�̃��C�Z���X�ɏ]���܂��B   
�����R�[�h��SJIS�ł��B   


# Convert from UTF8 to SJIS   
�V�C�\��T�C�g�̕����R�[�h�ɂ�UTF8���g���Ă��܂��B   
�t�H���g�t�@�C���ɂ�SJIS�̃R�[�h�Ńt�H���g���i�[����Ă��܂��B   
�����ŁA[������](https://www.mgo-tec.com/blog-entry-utf8sjis01.html)�Ō��J����Ă���ϊ��e�[�u��(Utf8Sjis.tbl)���g���āA
UTF8����SJIS�ɃR�[�h��ϊ����A�ϊ����ʂ�SJIS�R�[�h���g���ăt�H���g�����o���Ă��܂��B   

