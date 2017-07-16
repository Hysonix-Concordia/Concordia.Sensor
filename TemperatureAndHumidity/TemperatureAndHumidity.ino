#include <ESP8266HTTPClient.h>

#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#define DHTTYPE DHT11   // DHT 11

bool isConfigured = false;
String ssid = "";
String password = "";
String state = "WAITING";

String subId;

unsigned long previousMillis = 0; // last time update
long interval = 2000; // interval at which to do something (milliseconds)

ESP8266WebServer server(80);

const int DHTPin = 5;

// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE, 11);

void startWebserver(){
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
  
  Serial.println("");
  Serial.println(subId);
  Serial.println("WiFi connected");
  
  // Print the IP address
  Serial.println(WiFi.localIP());

  server.on("/", handleRootPath);  //Associate the handler function to the path

  server.on("/concordia/join", handleJoinConcordia);
  
  server.begin(); //Start the server
  
  Serial.println("Server listening");
}

void startSensor() {  
  dht.begin();
}

void handleRootPath() {       
  String html = "<form action=\"/concordia/join\" method=\"get\"><input type=\"text\" name=\"subid\" placeholder=\"SUBSCRIPTION ID\"><br><input type=\"submit\" value=\"Submit\"></form>";  
  server.send(200, "text/html", html);
}

void handleJoinConcordia(){
  subId = server.arg("subid");
  Serial.println(subId);
  for (int i = 0; i < subId.length(); ++i)
  {
    EEPROM.write(i, subId[i]);
    Serial.print("Wrote: ");
    Serial.println(subId[i]); 
  }
  
  EEPROM.commit();
  server.send(200, "text/html", "<div style=\"font-size: 18pt\">Subscription successful</div>");
}

void setup(void)
{   
  Serial.begin(115200);
  
  EEPROM.begin(512);

  int(10);

  Serial.println("Reading from EEPROM");
  String configCheck = "";
  for (int i = 0; i < 9; ++i)
  {
    configCheck += char(EEPROM.read(i));
  }
  
  if(configCheck == "concordia") {
    isConfigured = true;
  }
  Serial.println("configCheck: ");
  Serial.println(configCheck);
  Serial.println(isConfigured);

  for (int i = 9; i < 45; ++i)
  {
    ssid += char(EEPROM.read(i));
  }
  
  for (int i = 45; i < 81; ++i)
  {
    password += char(EEPROM.read(i));
  }
  
  for (int i = 81; i < 117; ++i)
  {
    subId += char(EEPROM.read(i));
  }

  
  Serial.println("ssid: ");
  Serial.println(ssid);
  Serial.println("password: ");
  Serial.println(password);
  Serial.println("subId: ");
  Serial.println(subId);

  Serial.println(state);
  if(isConfigured) {
    startWebserver();
    startSensor();
  }  
}

void loop() {
  while(Serial.available()) {
    String data = Serial.readString();// read the incoming data as string    
    Serial.println(data); 

    ssid = data.substring(0,32);
    password = data.substring(32, 64);
    subId = data.substring(64, 100);
    
    state = "CONNECTED";
    Serial.println(state);
    Serial.print("ssid: ");
    Serial.println(ssid);
    Serial.print("password: ");
    Serial.println(password);
    Serial.print("subId: ");
    Serial.println(subId);
  }
  if (state == "WAITING") {
    unsigned long currentMillis = millis();

    if(currentMillis - previousMillis > interval) {
      previousMillis = currentMillis;  
  
      Serial.println("WAITING");
    }
  }
  else if (state == "CONNECTED"){
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
