//------MPU-6050------
#include <Wire.h>

//-----Wifi/Telegram-----
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>

//------Push button--------
#define BUTTON_PIN 18


//------MPU-6050------
const int MPU_addr=0x68;
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
float ax=0, ay=0, az=0, gx=0, gy=0, gz=0;
boolean fall = false; //stores if a fall has occurred
boolean trigger1=false; //stores if first trigger (lower threshold) has occurred
boolean trigger2=false; //stores if second trigger (upper threshold) has occurred
boolean trigger3=false; //stores if third trigger (orientation change) has occurred
byte trigger1count=0; //stores the counts past since trigger 1 was set true
byte trigger2count=0; //stores the counts past since trigger 2 was set true
byte trigger3count=0; //stores the counts past since trigger 3 was set true
int angleChange=0;




//-----Wifi/Telegram-----
// Replace with your network credentials
const char* ssid = "aalto open";
const char* password = "";

// Initialize Telegram BOT
#define BOTtoken "2113591643:AAGXpRaxE9inyHKtZzhPvNKGDCs6l0amdsA"  // your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define CHAT_ID "711299774"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
//-----------------------------------------

void setup() {
  Serial.begin(115200);
  //-------MPU-6050------
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);



  
  //-----Wifi/Telegram-----
    // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  bot.sendMessage(CHAT_ID, "Bot started up", "");

  //--------------Push button-----------------
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  //---------------BUZZER-----------------
  ledcSetup(0,1E5, 12);
  ledcAttachPin(4  ,0);


}

void mpu_read(){
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr,14,true);  // request a total of 14 registers
  AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)    
  AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
  GyX=Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
  GyY=Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
  GyZ=Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
}

void loop() {
  mpu_read();
  ax = (AcX-2050)/16384.00;
  ay = (AcY-77)/16384.00;
  az = (AcZ-1947)/16384.00;
  gx = (GyX+270)/131.07;
  gy = (GyY-351)/131.07;
  gz = (GyZ+136)/131.07;

  float Raw_Amp = pow(pow(ax,2)+pow(ay,2)+pow(az,2),0.5);
  int Amp = Raw_Amp * 10;  // Mulitiplied by 10 bcz values are between 0 to 1

  if (Amp<=2 && trigger2==false){ //if AM breaks lower threshold (0.4g)
    trigger1=true;
    Serial.println("TRIGGER 1 ACTIVATED");
    }
    if (trigger1==true){
      trigger1count++;
      if (Amp>=12){ //if AM breaks upper threshold (3g)
        trigger2=true;
        Serial.println("TRIGGER 2 ACTIVATED");
        trigger1=false; trigger1count=0;
      }
    }
    if (trigger2==true){
      trigger2count++;
      angleChange = pow(pow(gx,2)+pow(gy,2)+pow(gz,2),0.5); Serial.println(angleChange);
      if (angleChange>=30 && angleChange<=400){ //if orientation changes by between 80-100 degrees
        trigger3=true; trigger2=false; trigger2count=0;
        Serial.println(angleChange);
        Serial.println("TRIGGER 3 ACTIVATED");
      }
   }
   if (trigger3==true){
    trigger3count++;
    if (trigger3count>=10){ 
      angleChange = pow(pow(gx,2)+pow(gy,2)+pow(gz,2),0.5);
      //delay(10);
      Serial.println(angleChange); 
      if ((angleChange>=0) && (angleChange<=10)){ //if orientation changes remains between 0-10 degrees
        fall=true; trigger3=false; trigger3count=0;
        Serial.println(angleChange);
      }
      else{ //user regained normal orientation
        trigger3=false; trigger3count=0;
        Serial.println("TRIGGER 3 DEACTIVATED");
      }
    }
  }
  if (fall==true){ //in event of a fall detection
    ledcWriteTone(0,800);
    delay (3000);
    uint8_t octave = 1;
    ledcWriteNote(0,NOTE_C,octave);
    delay(3000);
    
    
    int buttonState = digitalRead(BUTTON_PIN);
    
    if (buttonState == HIGH) {
      bot.sendMessage(CHAT_ID, "Fall detected!!", "");
      Serial.println("FALL DETECTED");
      fall=true;
    }
    else if (buttonState == LOW){
      Serial.println("Canceled");
      ledcWriteTone(0,0);
      delay (10000);
      fall = false;
    }
   
  }
  if (trigger2count>=6){ //allow 0.5s for orientation change
    trigger2=false; trigger2count=0;
    Serial.println("TRIGGER 2 DECACTIVATED");
  }
  if (trigger1count>=6){ //allow 0.5s for AM to break upper threshold
    trigger1=false; trigger1count=0;
    Serial.println("TRIGGER 1 DECACTIVATED");
  }
  delay(100);
}
