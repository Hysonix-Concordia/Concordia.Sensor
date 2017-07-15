#include <ESP8266HTTPClient.h>

#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#define DHTTYPE DHT11   // DHT 11

// WiFi parameters
const char* ssid = "Hysonix";
const char* password = "champster";

String subId;

ESP8266WebServer server(80);

// DHT Sensor
const int DHTPin = 5;

// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE, 11);

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

  delay(10);
  
  for (int i = 0; i < 36; ++i)
  {
    subId += char(EEPROM.read(i));
  }
  
  dht.begin();
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  
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

void loop() {
  server.handleClient(); //Handling of incoming requests
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
