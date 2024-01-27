#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>

#include <GyverPortal.h>
#include <EEPROM.h>

//----------LED----------
const int LED_green = 33;
const int LED_red = 32;

//------------ 


struct LoginPass {
  char ssid[20];
  char pass[20];

};

LoginPass lp;

void build() {
  String s;
  BUILD_BEGIN(s);
  add.THEME(GP_DARK);
  add.TITLE ("Welcome to the world of the DSD project!");

  

  add.FORM_BEGIN("/login");
  add.TEXT("lg", "Login", lp.ssid);      add.BREAK();
  add.TEXT("ps", "Password", lp.pass);   add.BREAK();

  add.SUBMIT("Submit");
  add.FORM_END();

  BUILD_END();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#define BOTtoken "xxxxxx"  // Initialize Telegram BOT, your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define CHAT_ID "xxxxxxx"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loginPortal() {
  Serial.println("Portal start");

  // запускаем точку доступа
  WiFi.mode(WIFI_AP);
  WiFi.softAP("DSD project local network");

  // запускаем портал
  GyverPortal portal;
  portal.attachBuild(build);
  portal.start(WIFI_AP);
  bool val = false;
  uint32_t tmr_t = millis();
  // работа портала
  while (portal.tick()) {
    if (portal.form("/login")) {      // кнопка нажата
      portal.copyStr("lg", lp.ssid);  // копируем себе
      portal.copyStr("ps", lp.pass);

      EEPROM.put(0, lp);              // сохраняем
      EEPROM.commit();                // записываем
      WiFi.softAPdisconnect();        // отключаем AP
      break;                          // выходим из цикла
    }
    if (millis() - tmr_t >= 200) {
      digitalWrite(LED_green, val);
      digitalWrite(LED_red, !val);
      val = !val;
      tmr_t = millis();  
    }
    
  }
  char read_ssid[20];
  Serial.print("Read lg = "); 
  EEPROM.get(0,read_ssid);
  Serial.println(read_ssid);
  


  char read_pass[20];
  Serial.print("Read lp.pass = ");
  EEPROM.get(0, read_pass); //readParam=EEPROM.readFloat(address);
  Serial.println(read_pass);  
  
  EEPROM.end();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void telegram_setup(){
  delay(2000);
  Serial.println();

  // читаем логин пароль из памяти
  EEPROM.begin(100);
  EEPROM.get(0, lp);

  
  loginPortal();

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(lp.ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(lp.ssid, lp.pass);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

bool telegram_send_message(String message){
  bot.sendMessage(CHAT_ID, message, "");
}
