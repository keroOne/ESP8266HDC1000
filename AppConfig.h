// AppConfig.h

#ifndef _APPCONFIG_h
#define _APPCONFIG_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
#include <ESP8266WiFi.h>

struct ZcSettings {
	int chId_intervalmin_intervalsec_alarmTh[4];//���낢��Ȑ����l
	char ssid[32];//WiFi AP�̃A�h���X
	char password[32];//WiFi AP�̃p�X���[�h
	char id[16];//���g�p
	char wKey[20];//writeKey
};

class AppConfigClass
{
private:
	bool bl_WiFiInitializing;
	void initWiFi(ZcSettings * config);
	short _isConfigMode;
	int configStep;
	bool isStepMode;
	String getServerIpAddressString(ZcSettings * src);
	void setServerIpAddress(String address, ZcSettings * dst);
	void serialFlush();
	String id;
	String ssid;
	String password;
	String ipAddress;
	struct ZcSettings * psettings;
protected:


public:
	AppConfigClass();
	~AppConfigClass();
	IPAddress getRemoteAddress();
	bool isConfigMode();
	void loadSettings();
	void saveSettings();
	void process();
	struct ZcSettings settings;
};

extern AppConfigClass AppConfig;

#endif

