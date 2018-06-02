#include "ESP8266WiFi.h"



// WiFi parameters to be configured
const char* ssid = "Krehlikovi";
const char* password = "axago240";
const char* host = "192.168.1.34";

void setup(void)
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  delay(100);
 
  
}

void loop() {

    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect(host, httpPort)) {
      return;
    }

    while(Serial.available()>0){
      
      String url = Serial.readString();
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");
      String line = client.readStringUntil(0xFF);
      Serial.print(line);
    }
    client.stop();
    delay(700);

}


