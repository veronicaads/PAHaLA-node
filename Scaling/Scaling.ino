//Library ESP32
#include <soc/rtc.h>

//Library Loadcell
#include <HX711.h>

//Library Koneksi (WiFi)
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebSocketClient.h>

//Library Time NTP
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SPI.h>

//Library Tambahan
#include <ArduinoJson.h>

//Define WiFi
const char* ssid = "Mi 5 Phone";
const char* password =  "stefanuS";

//Define Koneksi
#define UUID_SCALE "52f41354-d94e-49e6-952e-16744cfbe467"
StaticJsonBuffer<150> jsonBuffer;

//HOST UNTUK WEBSOCKETge
char path[] = "/hi";
char host[] = "pahala.xyz";
WebSocketClient webSocketClient;
WiFiClient client;

//Define NTP
#define NTP_OFFSET  25200 // In seconds 
#define NTP_INTERVAL 60 * 1000    // In miliseconds
#define NTP_ADDRESS  "id.pool.ntp.org"
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, 0, NTP_INTERVAL);

//Define Scale
#define BUZZER 4
HX711 scale(13, 14);
float tmp, ar[100], final_weight=0;
int i=0;

//Define Alarm
String statu;
String alarm_time;
String now;


void calibrate(){
  scale.set_scale(25280);
  //Kalo set scale makin besar, average getunit makin besar = Hasil makin kecil
  //Kalo set scale makin kecil, average getunit makin besar = Hasil makin besar
  
  scale.tare(); 
}

float average(){
  float ave=0, count=0;
  for(int y=5;y<i-3;y++){
    if(ar[y]!=0){
        ave+=ar[y];
        count++;  
    }
    else break;
  }
  if (count==0){
    return 0;
  }
  else{
    final_weight=(float)ave/count;
    return final_weight;
  }
}

void alarm_connect_server(int flag){
  //Kalo flag 1 : konek pertama, flag 2 : konek karena putus
  if(flag==1){
    digitalWrite(BUZZER, HIGH);delay(50);
    digitalWrite(BUZZER, LOW); delay(50);
    digitalWrite(BUZZER, HIGH);delay(50);
    digitalWrite(BUZZER, LOW); 
  }
  else{
    digitalWrite(BUZZER, HIGH);delay(50);
    digitalWrite(BUZZER, LOW); delay(50);
    digitalWrite(BUZZER, HIGH);delay(500);
    digitalWrite(BUZZER, LOW); 
  }
}

void connect_server(){//SEKALIAN NTP
  timeClient.setTimeOffset(NTP_OFFSET);
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
  if (client.connect(host, 8081)) {
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
      Serial.println("Connected Websocket^^");
      alarm_connect_server(1);
      String sendItems;

      /** Json object for outgoing data */
      JsonObject& jsonOut = jsonBuffer.createObject();
      jsonOut["key"] = "uuid";
      jsonOut["uuid"] = UUID_SCALE;
      // Convert JSON object into a string
      jsonOut.printTo(sendItems);

      Serial.println(sendItems);
      
      webSocketClient.sendData(sendItems);
      
      // jsonBuffer.clear();
        

      //SEND DALAM BENTUK JSON KEY DAN UUID send data
    
    }
    else {
      connect_server();
    }
  } else {
    Serial.println("Handshake failed.");
  }
  timeClient.begin();
}

void weighing(){
  Serial.print("Hasil : ");
  tmp=scale.get_units();
  delay(30);
  Serial.println(tmp);
  if(tmp>=11){
    ar[i++]=tmp;
  }
  else {
    if(ar[3]!=0){
      //Serial.println("Size : "+sizeof(ar));
      for(int i=1;i<sizeof(ar);i++){
        Serial.print(ar[i]);Serial.print("-");
      }
      Serial.println();
      Serial.print("Berat kamu: ");
      Serial.println(average());
      memset (ar, 0, sizeof(ar));
      i=0;

      scale.power_down();
      delay(500);
      scale.power_up();
  
      calibrate();
    }
  }
  
}

void post(String api_endpoint, String datum, float weight){//Datum temennya data
  if(WiFi.status() == WL_CONNECTED && weight>=11){
      HTTPClient http;
      http.begin(api_endpoint);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
//      int httpCode = http.POST(datum);
      int httpCode = http.POST("weight="+(String)weight+"&uuid="+(String)UUID_SCALE);
      if(httpCode>0){
        String response = http.getString();
        
        Serial.print("Code :");
        Serial.println(httpCode);
        Serial.println("Isi :"+response);
        final_weight=0;
      }
      else{
        Serial.println("Error on HTTP Request");
      }
      http.end();
      
      delay(1000);
   }
   else{
    Serial.println("Gagal Koneksi ke API Scale");
    connect_server(); 
   }
}

void setup() {
  rtc_clk_cpu_freq_set(RTC_CPU_FREQ_80M);
  Serial.begin(115200);
  Serial.println("Welcome, This is PAHALA Scale");
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  scale.power_down();
  delay(500);
  scale.power_up();

  calibrate();

  connect_server(); 
}

void loop() {
  String data;

  // KALO PAS JAMNYA, FORMAT HH:MM:SS BUNYI ALARAM digitalWrite(BUZZER, HIGH);
  // CEK BERAT BADAN, KALO ADA BERAT NYA MATTIN BUZZER KIRIM KE SERVER DATANYA
  // KALO ADA PERUBAHAN DATA ALARM, CEK JAM (websocket) heartbeat
  //WEBSOCKET : selalu nyala dan khsus minta jam, API ENDPOINT : Ngirim berat

//  weighing(); 
  
  if (client.connected()) {
      Serial.println("Connected ^^");

    //MINTA DATA ALARM
      String sendData;
      /** Json object for outgoing data */
//      JsonObject& jsonOut2 = jsonBuffer.createObject();
//      jsonOut2["key"] = "flag";
//      jsonOut2["flag"] = 1;
//      // Convert JSON object into a string
//      jsonOut2.printTo(sendData);
//      Serial.print("Data dikirim : ");
//      Serial.println(sendData);
//      webSocketClient.sendData(sendData);
//      jsonBuffer.clear();
      String sendItems="";

      /** Json object for outgoing data */
      JsonObject& jsonOut = jsonBuffer.createObject();
      jsonOut["key"] = "uuid";
      jsonOut["uuid"] = UUID_SCALE;
      // Convert JSON object into a string
      jsonOut.printTo(sendItems);

      Serial.println(sendItems);
      
      webSocketClient.sendData(sendItems);
      jsonBuffer.clear();
      
      statu="";alarm_time="";
      webSocketClient.getData(data);
      JsonObject& jsonIn = jsonBuffer.parseObject(data);
      if (jsonIn.success()) {
        if (jsonIn.containsKey("status")) { //JAM ALARM
            if(jsonIn.containsKey("text")){
                alarm_time = jsonIn["text"].as<String>();    
            }
          statu = jsonIn["status"].as<String>();
        }
      }
      Serial.print("Perintah : ");Serial.println(alarm_time);
      Serial.print("Status : ");Serial.println(statu);
      Serial.print("Penerimaan Data : ");Serial.println(data);

    //CEK ALARM
      timeClient.update();
      Serial.println(timeClient.getFormattedTime());
      Serial.print("Epoch : ");Serial.println(timeClient.getEpochTime());
      if(alarm_time!="" && statu=="200"){
        Serial.print("Cek Status"); Serial.println(statu);
//        if(timeClient.getFormattedTime()==alarm_time)//KALO UDH LEWAT JAMNYA
        if(alarm_time=="13:45:00")
        {
          delay(100);
          final_weight=0;
          while(final_weight<10){
            digitalWrite(LED_BUILTIN, HIGH);
            digitalWrite(BUZZER, HIGH);
            weighing();
          }
          digitalWrite(BUZZER, LOW);
          digitalWrite(LED_BUILTIN, LOW);
          //Kalo ada beratnya, kirim ke server;
          
          jsonBuffer.clear();
          JsonObject& jsonOut = jsonBuffer.createObject();
          jsonOut["weight"] = final_weight;
          jsonOut["uuid"] = UUID_SCALE;
          // Convert JSON object into a string
          jsonOut.printTo(sendItems);
          post("https://pahala.xyz/user/wakeup",sendItems, final_weight);
          
          Serial.print("Berat anda : ");
          Serial.println(final_weight);
        }
      }
  }
  else {
    alarm_connect_server(2);
    
    connect_server();
  }

}
