#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <IRremote.hpp>
#include "Secret.h"
#define RECV_PIN  2
#define openwindow 3
#define closewindow 4

//define maximum time the window allow to open
#define openClose_Time 2000

// Instances
WiFiSSLClient wifiClient;
PubSubClient mqttClient(wifiClient);

//priority
bool Manual= false;


uint32_t command;//command from remote


//timer for Manual control
unsigned long Last_control;

bool state = false; //state of the window

void setup() {
  Serial.begin(115200);
  while(!Serial);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting to WiFi...");
  }
  mqttClient.setCallback(callback);
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  connectMQTT();
  IrReceiver.begin(RECV_PIN);
  pinMode(openwindow, OUTPUT);
  pinMode(closewindow, OUTPUT);
  pinMode(12, OUTPUT);
}

void loop() {
  mqttClient.loop();
  if(Manual == true && millis() - Last_control > 14400000)
  {
    Manual = false;
  }
  if (IrReceiver.decode()) {
    command = IrReceiver.decodedIRData.decodedRawData;
    handleIRCommand(command);
  }
}

void handleIRCommand(uint32_t command)
{
  if(command == IR_opensignal && !state)
  {
    digitalWrite(openwindow, HIGH);
    delay(openClose_Time);
    digitalWrite(openwindow, LOW);
    state = true;
    Manual = true;
    Last_control = millis();
  }
  else if(command == IR_closesignal && state)
  {
    digitalWrite(closewindow,HIGH);
    delay(openClose_Time);
    digitalWrite(closewindow, LOW);
    state = false;
    Manual = true;
    Last_control = millis();
  }
  else if(command == IR_Manualsignal)
  {
    Manual = !Manual;
    Last_control = millis();
    if(Manual)
    {
      digitalWrite(12, HIGH);
      delay(100);
      digitalWrite(12, LOW);
      delay(50);
      digitalWrite(12,HIGH);
      delay(100);
      digitalWrite(12,LOW);
      delay(50);
    }
    else if(!Manual)
    {
      digitalWrite(12, HIGH);
      delay(100);
      digitalWrite(12, LOW);
    }
  }
  IrReceiver.resume();
}

void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.println("Connecting to MQTT...");
    if (mqttClient.connect("Arduino", MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("Connected to MQTT Broker!");
      mqttClient.subscribe(MQTT_SUBTOPIC);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) 
  {
    message += (char)payload[i];
  }
  Serial.println(message);
  if (message == "openwindow" && !Manual)
  {
    if(!state)
    {
      digitalWrite(openwindow, HIGH);
      delay(openClose_Time);
      digitalWrite(openwindow, LOW);
    }
  }
  else if(message == "closewindow" && !Manual){
    if(state)
    {
      Serial.println("Closeing window");
      digitalWrite(closewindow,HIGH);
      delay(openClose_Time);
      digitalWrite(closewindow, LOW);
    }
  }
  else if(message == "closewindowbycommand")
  {
    if(state)
    {
      digitalWrite(closewindow, 100);
      delay(openClose_Time);
      digitalWrite(closewindow, 0);
      Manual = true;
      state = false;
      Last_control = millis();
    }
  }
  else if (message == "openwindowbycommand")
  {
    if(!state)
    {
      digitalWrite(openwindow, HIGH);
      delay(openClose_Time);
      digitalWrite(openwindow, LOW);
      Manual = true;
      state = true;
      Last_control = millis();
    }
  }
  else if (message == "setautomode")
  {
    Manual = !Manual;
    if(Manual)
    {
      digitalWrite(12, HIGH);
      delay(100);
      digitalWrite(12, LOW);
      delay(50);
      digitalWrite(12,HIGH);
      delay(100);
      digitalWrite(12,LOW);
      delay(50);
    }
    else if(!Manual)
    {
      digitalWrite(12, HIGH);
      delay(100);
      digitalWrite(12, LOW);
    }
  }
}
