//RELAY LAMPU INTERRUPT
// Hanya mengirimkan UID 1 kali

// Library ESP32
#include <soc/rtc.h>

//Library Wifi
#include <WebSocketClient.h>
#include <HTTPClient.h>
#include <WiFi.h>

//Library Tambahan
#include <string.h>
#include <ArduinoJson.h>

//Define LAMPU 
//NORMALLY CLOSE (Saklar low- Lampu low nyala)
#define Lamp 27
#define Saklar 14
int reading;
String flag_on;
String state;

//Define WiFi
#define UUID_LAMP "3b38fe3d-77a3-4c6f-b7be-9e08829d9a7e"
StaticJsonBuffer<200> jsonBuffer;
//const char* ssid = "Mi 5 Phone";
//const char* password =  "stefanuS";


const char* ssid = "AD1210";
const char* password =  "aaaaaaaaa";

//Define WebSocket
char path[] = "/hi";
char host[] = "pahala.xyz";
WebSocketClient webSocketClient;
WiFiClient client;

void connect_server(){
  WiFi.begin(ssid, password);

  //CONNECT TO WIFI
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to WiFi ^.^ ");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // MULAI HANDSHAKE KE SERVER
  if (client.connect(host, 8080)) {
    Serial.println("Connected");
  } 
  else {
    Serial.println("Connection failed.");
  }
 
  webSocketClient.path = path;
  webSocketClient.host = host;
  if (webSocketClient.handshake(client)) {
    Serial.println("Handshake successful");
    if (client.connected()) {
       digitalWrite(LED_BUILTIN, HIGH);delay(50);
       digitalWrite(LED_BUILTIN, LOW);delay(50);
       digitalWrite(LED_BUILTIN, HIGH);delay(50);
       digitalWrite(LED_BUILTIN, LOW);delay(50);
       digitalWrite(LED_BUILTIN, HIGH);delay(50);
       digitalWrite(LED_BUILTIN, LOW);delay(50);
       
       Serial.println("Connected First Time ^^");

      String sendItems;

      /** Json object for outgoing data */
      JsonObject& jsonOut = jsonBuffer.createObject();
      jsonOut["key"] = "uuid";
      jsonOut["uuid"] = UUID_LAMP;
      // Convert JSON object into a string
      jsonOut.printTo(sendItems);

      Serial.println(sendItems);
      
      webSocketClient.sendData(sendItems);
      
        

      //SEND DALAM BENTUK JSON KEY DAN UUID send data
    
    }
    else {
      connect_server();
    }
  } 
  else {
    Serial.println("Handshake failed.");
  }
}

void setup() {
  rtc_clk_cpu_freq_set(RTC_CPU_FREQ_80M); //our ESP32 runs with only 80 MHZ and the signal is now 3 times longer, much better for the HX711
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(Lamp, OUTPUT);
  pinMode(Saklar, OUTPUT);
  
  connect_server();
}
 
void loop() {
  String data;
  if (client.connected()) {
      Serial.println("Connected ^^");
      //SEND UUID
      
      //AMBIL DATA
      jsonBuffer.clear();
      
      webSocketClient.getData(data);

      //MINTA DATA ON/OFF
      flag_on="";state="";
          /** Json object for outgoing data */
          JsonObject& jsonIn = jsonBuffer.parseObject(data);
          if (jsonIn.success()) {
            if (jsonIn.containsKey("status") || jsonIn.containsKey("flag")) {
              //Kalo nyala
              if(jsonIn["flag"].as<String>()){
                flag_on = jsonIn["flag"].as<String>(); 
              }
              state = jsonIn["status"].as<String>();
            }
          }
          Serial.print("Perintah : ");Serial.println(flag_on);
          Serial.print("Status : ");Serial.println(state);
          Serial.println(data);

      if(flag_on=="true"){
        digitalWrite(Saklar, LOW);
        digitalWrite(Lamp, LOW);
        digitalWrite(LED_BUILTIN, HIGH);
        Serial.println("Lampu Menyala");
//        delay(3000);
      }
      else if(flag_on=="false"){
        digitalWrite(Saklar, LOW);
        digitalWrite(Lamp, HIGH);
        digitalWrite(LED_BUILTIN, LOW);
        Serial.println("Lampu Padam");
//        delay(3000);
      }

      String sendData;
      //KIRIM RESPONSE
      // Convert JSON object into a string
      jsonIn.printTo(sendData);
      Serial.print("Data dikirim : ");
      Serial.println(sendData);

      webSocketClient.sendData(sendData);
      delay(10);
  }
  else{
      jsonBuffer.clear();
      connect_server();
  }
}


