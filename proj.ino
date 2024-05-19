#include "MQ135.h"

#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <ESP_Mail_Client.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ESP8266WiFi.h>
#include "ThingSpeak.h"
const char* ssid = "myesp";   // your network SSID (name) 
const char* password = "12345678";
float thresholdTemp = 40;


#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

#define AUTHOR_EMAIL "espiot26@gmail.com"
#define AUTHOR_PASSWORD "qifvblazlncxmdqh"

#define RECIPIENT_EMAIL "210701280@rajalakshmi.edu.in"

SMTPSession smtp;
SMTP_Message message;
Session_Config config;
void smtpCallback(SMTP_Status status);

unsigned long myChannelNumber = 3;
const char * myWriteAPIKey = "79CJ5HANUMRM7YUP";


#define DHTPIN 4    
unsigned long lastTime = 0;
unsigned long timerDelay = 10000;
unsigned long lastTime1 = 0;
unsigned long timerDelay1 = 100;
#define DHTTYPE    DHT11    
DHT_Unified dht(DHTPIN, DHTTYPE);





uint32_t delayMS;
float temperatureC;
float humidity;
float air_quality;
WiFiClient  client;
void setup() {
  Serial.begin(9600);

  dht.begin();
  Serial.println(F("DHTxx Unified Sensor Example"));
 
   WiFi.mode(WIFI_STA);   
  ThingSpeak.begin(client);
  sensor_t sensor;
 if(WiFi.status() != WL_CONNECTED){
  Serial.print("Attempting to connect");
  while(WiFi.status() != WL_CONNECTED){
    WiFi.begin(ssid, password); 
    delay(5000);     
  } 
  Serial.println("\nConnected.");
 }
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));





  
  delayMS = sensor.min_delay / 1000;

  MailClient.networkReconnect(true);

 
  smtp.debug(1);


  smtp.callback(smtpCallback);


  


  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;
  config.login.user_domain = "";
  

  
}

void loop() {

 

  if ((millis() - lastTime) > timerDelay) {
   MQ135 gasSensor = MQ135(A0);
    air_quality = gasSensor.getPPM();
    Serial.print("Air Quality: ");  
    Serial.print(air_quality);
    Serial.println("  PPM");   
    Serial.println();
    
  // Get temperature event and print its value.
  sensors_event_t event;


   dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    temperatureC = event.temperature;
    Serial.println(F("°C"));
    delay(50);
    if(temperatureC>thresholdTemp){
      sendMail();
    }

  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }

     else {
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    humidity= event.relative_humidity;
    Serial.println(F("%"));
  }
   lastTime = millis();
    ThingSpeak.setField(1, temperatureC);
    ThingSpeak.setField(2, humidity);
    ThingSpeak.setField(3, air_quality);
    
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    
    if(x == 200){
      Serial.println(" Channel update successful.");
    }
    else{
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }
  }


}

void sendMail(){
  message.sender.name = F("COLD STORAGE MONITOR SENSOR");
  message.sender.email = AUTHOR_EMAIL;
  message.subject = F("Alert! Anomaly detected");
  message.addRecipient(F("Taejashwar"), RECIPIENT_EMAIL);
  

   
  //Send raw text message
  String textMsg = "Warning! Food parameters compromised. Check before consuming";
  message.text.content = textMsg.c_str();
  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
  
  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;


  /* Connect to the server */
  if (!smtp.connect(&config)){
    ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return;
  }

  if (!smtp.isLoggedIn()){
    Serial.println("\nNot yet logged in.");
  }
  else{
    if (smtp.isAuthenticated())
      Serial.println("\nSuccessfully logged in.");
    else
      Serial.println("\nConnected with no Auth.");
  }

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    ESP_MAIL_PRINTF("Error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());

}





void smtpCallback(SMTP_Status status){
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success()){
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
    Serial.println("----------------\n");

    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
    {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);

      // In case, ESP32, ESP8266 and SAMD device, the timestamp get from result.timestamp should be valid if
      // your device time was synched with NTP server.
      // Other devices may show invalid timestamp as the device time was not set i.e. it will show Jan 1, 1970.
      // You can call smtp.setSystemTime(xxx) to set device time manually. Where xxx is timestamp (seconds since Jan 1, 1970)
      
      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");

    // You need to clear sending result as the memory usage will grow up.
    smtp.sendingResult.clear();
  }
}