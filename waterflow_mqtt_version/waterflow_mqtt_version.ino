#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <LoRa.h>

const char* ssid = "---------";    //your wifi ssid
const char* password =  "-------"; //your wifi password

const char* mqttServer = "127.0.0.1";//your mqtt server ip
const int mqttPort = 1883;
const char* mqttUser = "your username"; //your mqtt username
const char* mqttPassword = "your password"; //your mqtt pasword

WiFiClient espClient;
PubSubClient client(espClient);

//define the pins used by the transceiver module
#define ss 5
#define rst 2
#define dio0 32

String nodeId = "0", nodeData = "0";
String loradata = " ";

void cek_jaringan() {
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
}

void cek_hubungan() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32Client", mqttUser, mqttPassword )) {
      Serial.println("connected");
      client.subscribe("esp32/servo");
      client.subscribe("esp32/solenoid");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  cek_jaringan();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  cek_hubungan();

  LoRa.setPins(ss, rst, dio0);
  while (!LoRa.begin(923200000)) {
    Serial.println(".");
    delay(500);
  }
  LoRa.setSyncWord(0xEE);
  Serial.println("LoRa Initializing OK!");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    Serial.print("Received packet ");

    // read packet
    while (LoRa.available()) {
      loradata = LoRa.readString();
      nodeId = getValue(loradata, ':', 0);
      nodeData = getValue(loradata, ':', 1);
    }
    // print RSSI of packet
    Serial.print(" with RSSI ");
    Serial.println(LoRa.packetRssi());

    if (nodeId != "0") {
      sendToServer(nodeId, nodeData.toInt());
      nodeId = "0";
    }
  }//nothing to do
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

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off".
  // Changes the output state according to the message
  if (String(topic) == "esp32/servo") {
    Serial.print("Changing output to ");
    if (messageTemp == "1") {
      Serial.println("on");
    }
    else if (messageTemp == "0") {
      Serial.println("off");
    }
  }
  else if (String(topic) == "esp32/solenoid") {
    Serial.print("Changing output to ");
    if (messageTemp == "1") {
      Serial.println("on");
    }
    else if (messageTemp == "0") {
      Serial.println("off");
    }
  }
}

void sendToServer(String id, unsigned long data_sended) {
  cek_jaringan();
  cek_hubungan();

  StaticJsonBuffer<300> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();

  JSONencoder["id"] = id;
  JSONencoder["lpm"] = data_sended;
  char JSONmessageBuffer[60];

  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println("Sending message to MQTT topic..");
  Serial.println(JSONmessageBuffer);

  if (client.publish("esp32/waterflow", JSONmessageBuffer) == true) {
    Serial.println("Success sending message");

  } else {
    Serial.println("Error sending message");
  }
  client.loop();
  Serial.println(client.loop());
  Serial.println(client.state());
  Serial.println("-------------");
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
