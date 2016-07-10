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

#define HDC1000_ADRS     0x40      // HDC1000のI2Cアドレス
HDC hdc(HDC1000_ID, HDC1000_ADRS);// HDC1000ライブラリを生成する

WiFiClient client;
Ambient ambient;
ST7032 lcd;
long lastSendmillis;
long intervalmillis;
int alarmThreshold;
bool showTmp;

//温度湿度をambientのサービスへ送信する
//https://github.com/TakehikoShimojima/Ambient_ESP8266_lib

void ftoa(float val, char *buf) {
	itoa((int)val, buf, 10);
	int i = strlen(buf);
	buf[i++] = '.';
	val = (int)(val * 10) % 10;
	itoa(val, &buf[i], 10);
}

void setup() {
	// シリアル
	Serial.begin(115200);
	//GPIO14をプルダウン(10k)しておくと設定モードに入る
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
		lastSendmillis = LONG_MAX;//最後に送った時刻
		intervalmillis = (
			AppConfig.settings.chId_intervalmin_intervalsec_alarmTh[1] * 60
			+ AppConfig.settings.chId_intervalmin_intervalsec_alarmTh[2]
			) * 1000;
		Serial.print("IntervalMillis ");
		Serial.println(intervalmillis);
		// Ｉ２Ｃ
		Wire.begin();
		delay(3000);

		// HDC1000の初期化
		//http://www.geocities.jp/zattouka/GarageHouse/micon/Arduino/Humidity/Humidity.htm
		ans = hdc.Init();
		if (ans != 0) {
			Serial.print("HDC Initialization abnormal ans=");
			Serial.println(ans);
		}
		else Serial.println("HDC Initialization normal !!");

		// LCDの初期化
		//http://ore-kb.net/archives/195
		lcd.begin(8, 2);
		lcd.setContrast(30);
		lcd.print("Hello ");
		showTmp = true;

		//LED(不快指数が閾値を超えると点灯)
		pinMode(12, OUTPUT);
		digitalWrite(12, LOW);
		alarmThreshold = AppConfig.settings.chId_intervalmin_intervalsec_alarmTh[3];//閾値
	}
}
void loop() {
	if (!AppConfig.isConfigMode()) { loop1(); }//通常モード
	else { AppConfig.process(); }//設定モード
}

void loop1() {
	int ans;
	// HDC1000から湿度と温度を読み取る
	// 読み取った値は、Humi/Temp変数に格納される
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
		float dscftIdx = 0.81*Temp + 0.01*Humi*(0.99*Temp - 14.3) + 46.3;//式はwikiぺディアより
		if (dscftIdx > alarmThreshold) {
			digitalWrite(12, 0);//LOWで点灯する配線なので点灯させるときは0を出力する
		}
		else {
			digitalWrite(12, 1);
		}
		ftoa(dscftIdx, dscbuf);
		long m = millis();
		//ときどき送る
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
			//温度と湿度
			lcd.setCursor(0, 0);
			lcd.print("TMP ");
			lcd.print(tempbuf);
			lcd.setCursor(0, 1);
			lcd.print("HUM ");
			lcd.print(humidbuf);
		}
		else {
			//不快指数
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