#include <ESP8266HTTPClient.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#define DHTTYPE DHT11   // DHT 11

String state = "WAITING";
String ssid = "";
String password = "";
String deviceId = "";
String host = "http://concordia.hysonix.com";

unsigned long previousMillis = 0; // last time update
long interval = 2000; // interval at which to do something (milliseconds)
long sensorinterval = 900000; 

ESP8266WebServer server(80);

const int DHTPin = 5;
DHT dht(DHTPin, DHTTYPE, 11);
float temperature = 0;
float humidity = 0;

void startWebserver(){
  Serial.println(ssid);
  Serial.println(password);
  // Connect to WiFi  
  char __ssid[sizeof(ssid)];
  ssid.toCharArray(__ssid, sizeof(__ssid));
  char __password[sizeof(password)];
  password.toCharArray(__password, sizeof(__password));
  
  WiFi.begin(__ssid, __password);  
  
  Serial.println("Attempting connection");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
  }
  
  Serial.println("WiFi connected");
  
  // Print the IP address
  Serial.println(WiFi.localIP());

  server.on("/", handleRootPath); 
  server.on("/concordia/join", handleJoinConcordia);
  server.on("/concordia/sensor/data", handleReadSensor);
  
  server.begin();
  
  Serial.println("Server listening");
  state = "READY";
}

void startSensor() {  
  dht.begin();
}

void handleRootPath() {       
  String html = "<form action=\"/concordia/join\" method=\"get\"><input type=\"text\" name=\"devicename\" placeholder=\"DEVICE NAME\"><br><input type=\"text\" name=\"subid\" placeholder=\"SUBSCRIPTION ID\"><br><input type=\"submit\" value=\"Submit\"></form>";  
  server.send(200, "text/html", html);
}

void handleJoinConcordia(){
  String deviceName = server.arg("devicename");
  String subId = server.arg("subid");
  String uri = "/device/register";
  
  HTTPClient http;    //Declare object of class HTTPClient
 
  http.begin(host + uri);      //Specify request destination
  http.addHeader("Content-Type", "application/json");  //Specify content-type header

  String json = "{\"SubscriptionId\": \"" + subId + "\", \"DeviceName\": \"" + deviceName + "\"}";
  Serial.println(json);
  int httpCode = http.POST(json);
  String payload = http.getString();
  
  Serial.println(httpCode);   //Print HTTP return code
  Serial.println(payload);    //Print request response payload
  
  bool success = payload.substring(0, 10) == "SUCCESSFUL";
  String html = "";
  if(success) {
    deviceId = payload.substring(11);
    Serial.println(deviceId); 
    eeprom_write_string(87, (char *)deviceId.c_str());
    EEPROM.commit();  
    html = "<div style=\"font-size: 18pt\">Device registration successful</div>";
  }
  else {
    html = payload;
  }  
  
  http.end();  //Close connection

  startSensor();
  state = "CONNECTED";
  
  server.send(200, "text/html", html);
}

void handleReadSensor(){  
  char strHumidity[6];
  char strTemperature[6];
  dtostrf(humidity,5,2,strHumidity);
  dtostrf(temperature,5,2,strTemperature);  
  String html = String("<div style=\"font-size: 18pt\">Temperature: ") + strTemperature + String("</div><div style=\"font-size: 18pt\">Humidity: ") + strHumidity + String("</div>");
  server.send(200, "text/html", html);
}

bool readConfiguration(){
  bool isConfigured = false;
  Serial.println("Reading from EEPROM");
  String configCheck = "";
  for (int i = 0; i < 9; ++i)
  {
    configCheck += char(EEPROM.read(i));
  }
  
  if(configCheck == "concordia") {
    isConfigured = true;
  }

  ssid = "";
  for (int i = 10; i < 47; ++i)
  {
    ssid += char(EEPROM.read(i));
  }
  ssid.replace(" ", "");
  ssid.trim();
  ssid.remove(ssid.length() - 1);

  password = "";
  for (int i = 47; i < 87; ++i)
  {
    password += char(EEPROM.read(i));
  }
  password.replace(" ", "");
  password.trim();
  password.remove(password.length() - 1);

  deviceId = "";
  for (int i = 87; i < 125; ++i)
  {
    deviceId += char(EEPROM.read(i));
  }
  deviceId.replace(" ", "");
  deviceId.trim();
  deviceId.remove(deviceId.length() - 1);
  Serial.println(deviceId); 
  
  return isConfigured;
}

boolean eeprom_write_bytes(int startAddr, const byte* array, int numBytes) {
  int i;
  for (i = 0; i < numBytes; i++) {
    EEPROM.write(startAddr + i, array[i]);
  }
  return true;
}

boolean eeprom_write_string(int addr, char* string) {
  int numBytes;
  numBytes = strlen(string) + 1;
  return eeprom_write_bytes(addr, (const byte*)string, numBytes);
}

void saveConfiguration(String newSSID, String newPassword) {  
  eeprom_write_string(0, "concordia");
  eeprom_write_string(10, (char *)newSSID.c_str());
  eeprom_write_string(47, (char *)newPassword.c_str());
  EEPROM.commit();  
}

void setup(void)
{   
  Serial.begin(115200);
  
  EEPROM.begin(512);

  /*Use to clear EEPROM
  for (int i = 0; i < 9; ++i)
  {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();*/

  bool isConfigured = readConfiguration();

  if(isConfigured) {
    startWebserver();
    startSensor();
  }  
}

void saveConfiguration(String data) {
  String newSSID = data.substring(0,32);
  String newPassword = data.substring(32, 64);
  saveConfiguration(newSSID, newPassword);
  delay(200);
  bool isConfigured = readConfiguration();

  if(isConfigured) {
    startWebserver();
  }  
}

void loop() {
  bool configChanged = false;
  String data = "";
  while(Serial.available()) {
    data += Serial.readString();// read the incoming data as string    
    Serial.println(data); 
    configChanged = true;
  }

  if (configChanged && data.length() == 64) {
    saveConfiguration(data);
  }  
  
  unsigned long currentMillis = millis();
  if (state == "WAITING") {
    if(currentMillis - previousMillis > interval) {
      previousMillis = currentMillis;  
  
      Serial.println("WAITING");
    }
  }
  else if (state == "READY" || state == "CONNECTED"){
    if (state == "READY") {
      if(currentMillis - previousMillis > interval) {
        previousMillis = currentMillis; 
      
        Serial.print("READY:");       
        Serial.println(WiFi.localIP()); 
      }     
    }
    else {
      if(currentMillis - previousMillis > sensorinterval) {
        previousMillis = currentMillis; 
        
        humidity = dht.readHumidity();
        temperature = dht.readTemperature(true);
        if (isnan(humidity) || isnan(temperature)) {
          Serial.println("Failed to read from DHT sensor!");     
        }
        else{
          //Post to concordia
          /*String uri = "/device/register";
          
          HTTPClient http;    //Declare object of class HTTPClient
         
          http.begin(host + uri);      //Specify request destination
          http.addHeader("Content-Type", "application/json");  //Specify content-type header
        
          String json = "{\"SubscriptionId\": \"" + subId + "\", \"DeviceId\": \"" + deviceId + "\", \"Temperature\": \"" + temperature + "\", \"Humidity\": \"" + humidity + "\"}";
          Serial.println(json);
          int httpCode = http.POST(json);
          String payload = http.getString();*/
        }
        
      } 
     
    }
    server.handleClient(); //Handling of incoming requests    
  }
}
