#include <ESP8266HTTPClient.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#define DHTTYPE DHT11   // DHT 11

String state = "WAITING";
String ssid = "";
String password = "";

unsigned long previousMillis = 0; // last time update
long interval = 2000; // interval at which to do something (milliseconds)

ESP8266WebServer server(80);

const int DHTPin = 5;
DHT dht(DHTPin, DHTTYPE, 11);

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
  
  server.send(200, "text/html", "<div style=\"font-size: 18pt\">Subscription successful</div>");
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

  /*Use to clear EEPROM*/
  for (int i = 0; i < 9; ++i)
  {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();

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
    startSensor();
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
    server.handleClient(); //Handling of incoming requests    
  }
}


void sendData() {
  
  String host = "http://concordia.hysonix.com";
  String uri = "/sensor-data";
  
  HTTPClient http;    //Declare object of class HTTPClient
 
  http.begin(host + uri);      //Specify request destination
  http.addHeader("Content-Type", "text/json");  //Specify content-type header
  
  int httpCode = http.POST("{\"SubscriptionId\": \"" + subId + "\"}");   //Send the request
  String payload = http.getString();                  //Get the response payload
  
  Serial.println(httpCode);   //Print HTTP return code
  Serial.println(payload);    //Print request response payload
  
  http.end();  //Close connection
}
