#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <RCSwitch.h>
#include "DHT.h"

const char* ssid = "TNCAPA11CF7";
const char* password = "004B7E3FE8";
const char* mqtt_server = "192.168.1.2";
WiFiClient espClient;
PubSubClient client(espClient);
RCSwitch mySwitch = RCSwitch();

long lastMsg = 0;
char msg[50];
int value = 0;
float last =0;
#define DHTPIN 12     // what digital pin we're connected to
float average = 20;
#define k 0.09
#define DHTTYPE DHT11 
DHT dht(DHTPIN, DHTTYPE);
static int LoopCount = 0;


void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  mySwitch.enableTransmit(0);
  mySwitch.setPulseLength(180);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  ArduinoOTA.setHostname("RF433-Lounge");
  delay(2000);
  ArduinoOTA.begin();
  dht.begin();
  digitalWrite(LED_BUILTIN, HIGH);
  average = dht.readTemperature();
  last = dht.readTemperature();
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
   String Topic = String((char *)topic);
   if (Topic == "/openHAB/out/RF433_OUT/command") {
    digitalWrite(LED_BUILTIN, LOW);
   Serial.print("Sending RF Command :");
   Serial.println((char *)payload);
   mySwitch.send((char *)payload);
   digitalWrite(LED_BUILTIN, HIGH);
   byte payload[0];
   }
   return;
   
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("RF433Controller")) {
      Serial.println("connected");
      client.subscribe("/openHAB/out/RF433_OUT/command/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  //ArduinoOTA.handle();
  LoopCount = LoopCount + 1;
  if (LoopCount == 200000) {
  delay(800);
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float reading = dht.computeHeatIndex(t, h, false); // whatever code reads the current value
  if (t <= last + 1) {
  average += k * (t - average) ;
  Serial.println(t);
  char buffer[10];
  dtostrf(roundtemp(average), 2, 2, buffer); // Convert Temp To Char for MQTT Buffer
  client.publish("/openHAB/in/Lounge_Temp/command", buffer);
  dtostrf(h, 2, 2, buffer); // Convert Temp To Char for MQTT Buffer
  client.publish("/openHAB/in/Lounge_Humid/command", buffer);
  }
  LoopCount=0;
  }
  }
float roundtemp (float x)
{
  int a = round(x * 10);
  float newTemp = a / 10.0;
  return newTemp ;
}
