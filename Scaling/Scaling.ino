#include <soc/rtc.h>
#include <HX711.h>
#include <string.h>
#include <WiFi.h>
#include <HTTPClient.h>

HX711 scale(13, 12);
float tmp, ar[100], final_weight=0;
int i=0;

const char* ssid = "Sutedjo";
const char* password =  "11191996";

void calibrate(){
  scale.set_scale(25280);
//  scale.set_scale(10200);
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

void connect_server(){
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to WiFi ^.^ ");
}

void setup() {
  rtc_clk_cpu_freq_set(RTC_CPU_FREQ_80M);
  Serial.begin(115200);
  Serial.println("HX711 Demo");
  pinMode(LED_BUILTIN, OUTPUT);
  scale.power_down();
  delay(500);
  scale.power_up();
  Serial.println("Welcome");

  calibrate();

  connect_server(); 
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
//      Serial.println("Size : "+sizeof(ar));
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

void post(String endpoint, float datum){//Datum temennya data
  if(WiFi.status() == WL_CONNECTED && datum>=11){
      HTTPClient http;
      http.begin(endpoint);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
      int httpCode = http.POST("names="+(String)datum);
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
}

void loop() {
  // put your main code here, to run repeatedly:
  weighing(); 

//  if(final_weight>10){
//    post("http://pahala.xyz:8200/hello",final_weight);
//  }
}
