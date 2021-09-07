#include <M5StickC.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "AXP192.h"
#include "config.h"
#include "time.h"
#include <rom/rtc.h>

char* mqtt_topic = "iot/m5stick";


RTC_TimeTypeDef RTC_TimeStruct;
RTC_DateTypeDef RTC_DateStruct;
// ntp Server that is used for timesync
char* ntpServer =  "de.pool.ntp.org";

// connect wifi
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWD;
const char* mqtt_server = MQTT_SERVER;

#define TFT_GREY 0x5AEB // New colour
int brightness = 7;


//MQTT client
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");  
  }

  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {

    Serial.print("Attempting MQTT connection...");
    String clientId = "MQTT_Werkplaats";
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("hue/#");
     // client.subscribe("tkkrlab/spacestate");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}




void setup() {
    M5.begin();
    Serial.begin(115200);

    
    // we need to know why we booted up - deep sleep or actual power on?
    Serial.println("hello!");
    Serial.println("I was woken up for reason number (1 is reset, 5 is deep sleep..):");
    Serial.println(rtc_get_reset_reason(0));
    if ( rtc_get_reset_reason(0) == SW_CPU_RESET || rtc_get_reset_reason(0) == POWERON_RESET ) {  //if we actually reset

            Serial.println("looks like an actual reset, so I'll initialise some stuff");


             //RTC stuff
//              configTime(3600, 0, ntpServer);

            // Get local time
//            struct tm timeInfo;
//            if (getLocalTime(&timeInfo)) {
//              // Set RTC time
              RTC_TimeTypeDef TimeStruct;
//              TimeStruct.Hours   = timeInfo.tm_hour;
//              TimeStruct.Minutes = timeInfo.tm_min;
//              TimeStruct.Seconds = timeInfo.tm_sec;

                  //this code overrides the time above so we can use elapsed time instead. Comment out, whatever. 
                  TimeStruct.Hours   = 0;
                  TimeStruct.Minutes = 0;
                  TimeStruct.Seconds = 0;
              M5.Rtc.SetTime(&TimeStruct);
        
//              RTC_DateTypeDef DateStruct;
//              DateStruct.WeekDay = timeInfo.tm_wday;
//              DateStruct.Month = timeInfo.tm_mon + 1;
//              DateStruct.Date = timeInfo.tm_mday;
//              DateStruct.Year = timeInfo.tm_year + 1900;
//              M5.Rtc.SetData(&DateStruct);
//            }


            
   }


 
  M5.Lcd.setRotation(1);

  

  

   M5.Axp.EnableCoulombcounter();
   M5.Axp.ScreenBreath(brightness);  //screen brightness (values 7->12)
   M5.Axp.SetLDO2(false);
   setup_wifi();
  //Serial.begin(115200);
  client.setServer(mqtt_server, 1883);

  delay(1000);

  

    // M5.Axp.SetLDO3(false);





    
}

void loop() {
  String figuretodisplay = String(M5.Axp.GetBatVoltage());
  M5.Rtc.GetTime(&RTC_TimeStruct);
  String timestring = String(RTC_TimeStruct.Hours) +"h"+String(RTC_TimeStruct.Minutes)+"m"+String(RTC_TimeStruct.Seconds)+"s";
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
   
  Serial.print("sending mqtt message..");
  
  //String mqqtstring = "power: " + String((M5.Axp.GetPowerbatData()*1.1*0.5/1000),4);
  String mqqtstring = "battery voltage: " + figuretodisplay + " / " + timestring;
  int str_len = mqqtstring.length() + 1; 
  char mqqt_char_array[str_len];
  mqqtstring.toCharArray(mqqt_char_array, str_len);
  
  //client.publish("iot/m5stick",mqqt_char_array);
  client.publish(mqtt_topic,mqqt_char_array);
  
  delay(1000); //give it time to send
  WiFi.disconnect();

  esp_sleep_enable_timer_wakeup(SLEEP_MIN(30));  // 300 = 5 min, times 6 = 30 min OR we could always use the defined functions from the axp.h file! 
  //esp_sleep_enable_timer_wakeup(1000000*60);  // code test mode, every 1 min. Use only for testing 
  //M5.Axp.DeepSleep(SLEEP_MIN(1));
  M5.Axp.SetSleep();  // REMEMBER this will turn OFF the esp cpu power supply unless you change the code in axp.cpp! 
  esp_deep_sleep_start();
  //M5.Axp.DeepSleep(SLEEP_SEC(60));
  //delay(30000); //30s
}
