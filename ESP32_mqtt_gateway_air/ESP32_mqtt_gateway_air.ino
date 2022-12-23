#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <LoRa.h>

WiFiClient espClient;
PubSubClient client(espClient);

//define the pins used by the transceiver module
#define ss 5
#define rst 2
#define dio0 32

const char* ssid = "---------";    //your wifi ssid
const char* password =  "-------"; //your wifi password

const char* mqttServer = "127.0.0.1";//your mqtt server ip
const int mqttPort = 1883;
const char* mqttUser = "your username"; //your mqtt username
const char* mqttPassword = "your password"; //your mqtt pasword

float temperature = 0;
float humidity = 0;

String nodeId = "0", nodeData0 = "0", nodeData1 = "0";
String loradata = " ";

void setup() {
  Serial.begin(115200);
  wifi_reconnect();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  mqtt_reconnect();
  LoRa.setPins(ss, rst, dio0);
  while (!LoRa.begin(923200000)) {
    Serial.println(".");
    delay(500);
  }
  LoRa.setSyncWord(0xEE);
  Serial.println("LoRa Initializing OK!");
}

void wifi_reconnect() {
  delay(10);
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
}

void mqtt_reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("esp32water", mqttUser, mqttPassword )) {
      Serial.println("connected");
      client.subscribe("esp32/servo");
      client.subscribe("esp32/solenoid");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if (String(topic) == "esp32/servo") {
    if (messageTemp == "on") {
      lora_to_node("001", "on");
    }
    else if (messageTemp == "off") {
      lora_to_node("001", "off");
    }
  }
  else if (String(topic) == "esp32/solenoid") {
    if (messageTemp == "on") {
      lora_to_node("002", "on");
    }
    else if (messageTemp == "off") {
      lora_to_node("002", "off");
    }
  }
}


void mqtt_send(String id, float data_to_send0, float data_to_send1 ) {
  if (WiFi.status() != WL_CONNECTED) {
    wifi_reconnect();
  }
  if (!client.connected()) {
    mqtt_reconnect();
  }
  StaticJsonBuffer<300> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();

  JSONencoder["id"] = id;
  JSONencoder["lpm"] = data_to_send0;
  JSONencoder["total"] = data_to_send1;
  char JSONmessageBuffer[100];

  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println("Sending message to MQTT topic..");
  Serial.println(JSONmessageBuffer);

  if (client.publish("esp32/water", JSONmessageBuffer) == true) {
    Serial.println("Success sending message");
  } else {
    Serial.println("Error sending message");
  }
  Serial.println(client.loop());
  Serial.println(client.state());
  Serial.println("-------------");
}

void lora_to_node(String id, String callback_state) {
  String packet = id + ":" + callback_state;
  Serial.print("Send to node  ");
  Serial.println(packet);

  LoRa.beginPacket();
  LoRa.print(packet);
  LoRa.endPacket();
}

String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
void loop() {
  client.loop();
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    Serial.print("Received packet ");

    // read packet
    while (LoRa.available()) {
      loradata = LoRa.readString();
      nodeId = getValue(loradata, ':', 0);
      nodeData0 = getValue(loradata, ':', 1);
      nodeData1 = getValue(loradata, ':', 2);
    }
    // print RSSI of packet
    Serial.println(loradata);
    Serial.print(" with RSSI ");
    Serial.println(LoRa.packetRssi());

    if (nodeId != "0") {
      mqtt_send(nodeId, nodeData0.toFloat(), nodeData1.toFloat());
      nodeId = "0";
    }
  }
}
