#include <HTTPClient.h>
#include <HX711.h>
#include <WiFi.h>
#include <soc/rtc.h>
//LAMPU NORMALLY CLOSE (low-low nyala)
#define Lamp 27
#define Saklar 14
const char* ssid = "Sutedjo";
const char* password =  "11191996";
  
int reading;

void setup() {
  rtc_clk_cpu_freq_set(RTC_CPU_FREQ_80M); //our ESP32 runs with only 80 MHZ and the signal is now 3 times longer, much better for the HX711
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(Lamp, OUTPUT);
  pinMode(Saklar, OUTPUT);
  
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  
 if(WL_CONNECTED){
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    Serial.println("Connected to the WiFi network");
 }
  
 
}
 
void loop() {
  digitalWrite(Saklar, LOW);
  if(WiFi.status() == WL_CONNECTED){
    
    HTTPClient http;

    http.begin("http://pahala.xyz:8200/hello");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = http.POST("Status");//APA YANG MAU DIKIRIM ?
  //ERROR KARENA Port 8200 sudha dimatikan dari server
    if(httpCode>0){
      //KALO DAPET HASIL GET
      String request = http.getString();

      Serial.print("Code :");
      Serial.println(httpCode);
      Serial.println("Isi :"+request);
      
    
      //IF NYALA MAU MATIIN
      reading = digitalRead(Lamp);
      if(reading){
        digitalWrite(Lamp, LOW);
        digitalWrite(LED_BUILTIN, HIGH);
        Serial.println("Lampu Menyala");
        delay(3000);
      }
      else{
        digitalWrite(Lamp, HIGH);
        digitalWrite(LED_BUILTIN, LOW);
        Serial.println("Lampu Padam");
        delay(3000);
      }
    }
    else{
      Serial.println("Error on HTTP Request");
    }

    http.end();

    delay(5000);
  }
}
