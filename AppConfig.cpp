// 
// 
// 
//
#include "AppConfig.h"
#include <EEPROM.h>
//
//
AppConfigClass::AppConfigClass() {
	bl_WiFiInitializing = false;
	configStep = -1;
	isStepMode = true;
	_isConfigMode = -1;
	psettings = &settings;
	//Serial.println("AppConfigClassInitializing");
	loadSettings();
	//Serial.println("AppConfigClass Init Done...");
}

AppConfigClass::~AppConfigClass() {}

IPAddress AppConfigClass::getRemoteAddress() {
	return	IPAddress(psettings->chId_intervalmin_intervalsec_alarmTh[0],
					  psettings->chId_intervalmin_intervalsec_alarmTh[1],
					  psettings->chId_intervalmin_intervalsec_alarmTh[2],
					  psettings->chId_intervalmin_intervalsec_alarmTh[3]);
}

void AppConfigClass::initWiFi(ZcSettings * config) {
	if (bl_WiFiInitializing) { return; }
	bl_WiFiInitializing = true;
	WiFi.mode(WIFI_STA);
	WiFi.begin(config->ssid, config->password);
	Serial.print("Connecting to ");
	Serial.println(config->ssid);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	WiFi.config(IPAddress(config->chId_intervalmin_intervalsec_alarmTh[0],
						  config->chId_intervalmin_intervalsec_alarmTh[1],
						  config->chId_intervalmin_intervalsec_alarmTh[2],
						  config->chId_intervalmin_intervalsec_alarmTh[3]),
				WiFi.gatewayIP(), WiFi.subnetMask());
	Serial.println("");
	Serial.println("WiFi connected");
	Serial.print("IP Address:");
	Serial.print(WiFi.localIP());
	Serial.println("");
}

bool AppConfigClass::isConfigMode() {//GPIO14がプルダウンされていればコンフィグモードいなければ通常モード
	if (_isConfigMode == -1) {//未判定の場合は判定処理を実行して結果を格納する
		pinMode(14, INPUT_PULLUP);
		delay(10);
		short val = digitalRead(14);
		if (val == LOW) { _isConfigMode = 1; }
		else { _isConfigMode = 0; }
	}
	return _isConfigMode == 1;//格納してある値を返す
}

void AppConfigClass::process() {
	//serialFlush();
	if (configStep == -1) {
		Serial.println("Start_Configration");
		configStep = 10;
	}
	else if (configStep == 10) {//my wKey
		Serial.print("WriteKey , current ");
		Serial.print(String(settings.wKey));
		Serial.println(" )");
		configStep = 15;
	}
	else if (configStep == 15) {
		//自分のIDを設定(入力受付)
		String s = Serial.readStringUntil('\n');
		s.trim();
		//serialFlush();
		if (s.length()!=0){ //(s != "") { ToDoここ直す
			strcpy(psettings->wKey, s.c_str());
			if (isStepMode) { configStep = 20; }
			else { configStep = 90; }
		}
		delay(200);
	}
	else if (configStep == 20) {//server ip address
		Serial.print("ChId_IntervalMin_InervalSec_AlarmTh  (current ");
		String ip_ad = getServerIpAddressString(psettings);
		Serial.print(ip_ad);
		Serial.print(") :");
		configStep = 25;
	}
	else if (configStep == 25) {
		String s = Serial.readStringUntil('\n');
		s.trim();
		//serialFlush();
		if (s != "") {
			setServerIpAddress(s, psettings);
			if (isStepMode) { configStep = 30; }
			else { configStep = 90; }
		}
		delay(200);
	}
	else if (configStep == 30) {//ssid
		Serial.print("SSID ( current ");
		Serial.print(String(settings.ssid));
		Serial.println(" )");
		configStep = 35;
	}
	else if (configStep == 35) {
		String s = Serial.readStringUntil('\n');
		s.trim();
		//serialFlush();
		if (s != "") {//ToDoここ直す
			strcpy(psettings->ssid, s.c_str());
			if (isStepMode) { configStep = 40; }
			else { configStep = 90; }
		}
		delay(200);
	}
	else if (configStep == 40) {//ssid
		Serial.print("password (current ");
		Serial.print(String(settings.password));
		Serial.println(" )");
		configStep = 45;
	}
	else if (configStep == 45) {
		String s = Serial.readStringUntil('\n');
		s.trim();
		//serialFlush();
		if (s != "") {//ToDoここ直す
			strcpy(psettings->password, s.c_str());
			if (isStepMode) { configStep = 90; }
			else { configStep = 90; }
		}
		delay(200);
	}
	else if (configStep == 90) {//確認
		Serial.println("Enter 0 (OK) or 1-4 (Change)");
		Serial.println("1 WriteKey: " + String(settings.wKey));
		Serial.println("2 ChId_IntervalMin_IntervalSec_AlarmTh: " + getServerIpAddressString(psettings));
		Serial.println("3 SSID: " + String(settings.ssid));
		Serial.println("4 Password: " + String(settings.password));
		configStep = 95;
	}
	else if (configStep == 95) {
		String s = Serial.readStringUntil('\n');
		s.trim();
		if (s != "") {
			Serial.println(s);
			if (s == "0") { configStep = 100; }
			else if (s == "1") { configStep = 10; isStepMode = false; }
			else if (s == "2") { configStep = 20; isStepMode = false; }
			else if (s == "3") { configStep = 30; isStepMode = false; }
			else if (s == "4") { configStep = 40; isStepMode = false; }
			else { configStep = 95; }
		}
		delay(200);
	}
	else if (configStep == 100) {
		configStep = 999;
		Serial.println("saving...");
		saveSettings();
	}
	else {
		initWiFi(psettings);
		Serial.println("Config Done!  Restarting...");
		while (true);
	}
}

String AppConfigClass::getServerIpAddressString(ZcSettings * src) {
	String ip_ad = String(src->chId_intervalmin_intervalsec_alarmTh[0]) + "."
		+ String(src->chId_intervalmin_intervalsec_alarmTh[1]) + "."
		+ String(src->chId_intervalmin_intervalsec_alarmTh[2]) + "."
		+ String(src->chId_intervalmin_intervalsec_alarmTh[3]);
	return ip_ad;
}

void AppConfigClass :: setServerIpAddress(String address,ZcSettings * dst) {
	address.trim();
	if (address == "") { return; }
	if (address.indexOf(address.length() - 1) != '.') { address = address + '.'; }
	int oldPos = 0;
	int pos = 1;
	int i = 0;
	while (pos > 0 && i < 4) {
		pos = address.indexOf('.', oldPos);
		String b = address.substring(oldPos, pos);
		dst->chId_intervalmin_intervalsec_alarmTh[i] = b.toInt();
		oldPos = pos + 1;
		i++;
	}
}

void AppConfigClass::serialFlush() {
	while (Serial.available() > 0) {
		char t = Serial.read();
	}
}
void AppConfigClass::loadSettings() {
	EEPROM.begin(sizeof(ZcSettings));
	EEPROM.get<ZcSettings>(0, settings);
	EEPROM.end();
}

void AppConfigClass::saveSettings() {
	EEPROM.begin(sizeof(ZcSettings));
	EEPROM.put<ZcSettings>(0, settings);
	EEPROM.end();
}

AppConfigClass AppConfig;

