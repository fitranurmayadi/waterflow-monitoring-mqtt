#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <LoRa.h>

const char* ssid = "-----";      // your wifi ssid
const char* password =  "-----"; // your wifi password

String alamatServer = "http://127.0.0.1";
String alamatAplikasi = "/monitor_air/database/nodeToServer?";

//define the pins used by the transceiver module
#define ss 5
#define rst 2
#define dio0 32
String nodeId = "0", nodeData = "0";

String loradata = " ";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("........");
  }

  Serial.print("Terkoneksi dengan WLAN dengan Akses point : ");
  Serial.println(WiFi.localIP());

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

void sendToServer(String id, unsigned long data_sended) {
  if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
    HTTPClient http;
    
    String httpdata = alamatServer + alamatAplikasi + "id=" + id + "&lpm=" + data_sended;
    
    Serial.println(httpdata);
    
    http.begin(httpdata); //Specify the URL
    
    int httpCode = http.GET();                                        //Make the request
    Serial.println(httpCode);
    if (httpCode == 200) { //Check for the returning code
      String payload = http.getString();
      //Serial.println(httpCode);
      Serial.println(payload);
    }
    else {
      Serial.println("Error on HTTP request");
    }
    http.end(); //Free the resources
  }
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
