#include <EEPROM.h>
#include <ST7032.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <Ambient.h>
#include <Wire.h>
#include <HDC.h>
#include "AppConfig.h"
#include <limits.h>

extern "C" {
#include <user_interface.h>
}

#define HDC1000_ADRS     0x40      // HDC1000��I2C�A�h���X
HDC hdc(HDC1000_ID, HDC1000_ADRS);// HDC1000���C�u�����𐶐�����

WiFiClient client;
Ambient ambient;
ST7032 lcd;
long lastSendmillis;
long intervalmillis;
int alarmThreshold;
bool showTmp;

//���x���x��ambient�̃T�[�r�X�֑��M����
//https://github.com/TakehikoShimojima/Ambient_ESP8266_lib

void ftoa(float val, char *buf) {
	itoa((int)val, buf, 10);
	int i = strlen(buf);
	buf[i++] = '.';
	val = (int)(val * 10) % 10;
	itoa(val, &buf[i], 10);
}

void setup() {
	// �V���A��
	Serial.begin(115200);
	//GPIO14���v���_�E��(10k)���Ă����Ɛݒ胂�[�h�ɓ���
	if (AppConfig.isConfigMode()) {
		Serial.println("ConfigMode");
	}
	else {
		Serial.println("NormalMode");
		AppConfig.loadSettings();
		wifi_set_sleep_type(LIGHT_SLEEP_T);
		int ans;

		// ambient
		//https://github.com/TakehikoShimojima/Ambient_ESP8266_lib
		ambient.begin(AppConfig.settings.chId_intervalmin_intervalsec_alarmTh[0], AppConfig.settings.wKey, &client);
		lastSendmillis = LONG_MAX;//�Ō�ɑ���������
		intervalmillis = (
			AppConfig.settings.chId_intervalmin_intervalsec_alarmTh[1] * 60
			+ AppConfig.settings.chId_intervalmin_intervalsec_alarmTh[2]
			) * 1000;
		Serial.print("IntervalMillis ");
		Serial.println(intervalmillis);
		// �h�Q�b
		Wire.begin();
		delay(3000);

		// HDC1000�̏�����
		//http://www.geocities.jp/zattouka/GarageHouse/micon/Arduino/Humidity/Humidity.htm
		ans = hdc.Init();
		if (ans != 0) {
			Serial.print("HDC Initialization abnormal ans=");
			Serial.println(ans);
		}
		else Serial.println("HDC Initialization normal !!");

		// LCD�̏�����
		//http://ore-kb.net/archives/195
		lcd.begin(8, 2);
		lcd.setContrast(30);
		lcd.print("Hello ");
		showTmp = true;

		//LED(�s���w����臒l�𒴂���Ɠ_��)
		pinMode(12, OUTPUT);
		digitalWrite(12, LOW);
		alarmThreshold = AppConfig.settings.chId_intervalmin_intervalsec_alarmTh[3];//臒l
	}
}
void loop() {
	if (!AppConfig.isConfigMode()) { loop1(); }//�ʏ탂�[�h
	else { AppConfig.process(); }//�ݒ胂�[�h
}

void loop1() {
	int ans;
	// HDC1000���玼�x�Ɖ��x��ǂݎ��
	// �ǂݎ�����l�́AHumi/Temp�ϐ��Ɋi�[�����
	ans = hdc.Read();
	if (ans == 0) {
		char tempbuf[12], humidbuf[12], dscbuf[12];
		Serial.print("HDC1000: ");
		Serial.print(Humi);
		Serial.print("%  ");
		Serial.print(Temp);
		Serial.println("'C");
		ftoa(Temp, tempbuf);
		ftoa(Humi, humidbuf);
		float dscftIdx = 0.81*Temp + 0.01*Humi*(0.99*Temp - 14.3) + 46.3;//����wiki�؃f�B�A���
		if (dscftIdx > alarmThreshold) {
			digitalWrite(12, 0);//LOW�œ_������z���Ȃ̂œ_��������Ƃ���0���o�͂���
		}
		else {
			digitalWrite(12, 1);
		}
		ftoa(dscftIdx, dscbuf);
		long m = millis();
		//�Ƃ��ǂ�����
		if ((m < lastSendmillis) || m > lastSendmillis + intervalmillis) {
			ambient.set(1, tempbuf);
			ambient.set(2, humidbuf);
			ambient.set(3, dscbuf);
			ambient.send();
			yield();
			lastSendmillis = m;
			Serial.print(AppConfig.settings.wKey);
			Serial.print(" ");
			Serial.print(AppConfig.settings.chId_intervalmin_intervalsec_alarmTh[0]);
			Serial.println("sent");
		}
		delay(100);
		lcd.clear();
		if (showTmp) {
			//���x�Ǝ��x
			lcd.setCursor(0, 0);
			lcd.print("TMP ");
			lcd.print(tempbuf);
			lcd.setCursor(0, 1);
			lcd.print("HUM ");
			lcd.print(humidbuf);
		}
		else {
			//�s���w��
			lcd.setCursor(0, 0);
			lcd.print("DscftIdx");
			lcd.setCursor(0, 1);
			lcd.print(dscftIdx);

		}
		showTmp = !showTmp;
	}
	else {
		Serial.print("Failed to read from HDC ans=");
		Serial.println(ans);
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("TMP ----");
		lcd.setCursor(0, 1);
		lcd.print("HUM ----");
	}
	delay(4900);                     
}