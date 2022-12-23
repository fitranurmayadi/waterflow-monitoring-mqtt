#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <LoRa.h>

const char* ssid = "ap";
const char* password =  "1234567890";

String alamatServer = "http://127.0.0.1";
String alamatAplikasi = "/monitor_air/database/nodeToServer?";

//define the pins used by the transceiver module
#define ss 5
#define rst 2
#define dio0 32
#define wFsensor 27

String myId = "001", nodeId ="0";

String loradata = " ", nodeData = " ";


int time_counter = 0;
unsigned long debit_counter = 0;
unsigned long debit = 0;
unsigned long total = 0;
const long interval = 10000; //waktu untuk mengirim;

TaskHandle_t Task_00;
TaskHandle_t Task_01;

void IRAM_ATTR flowCounter() {
  debit_counter++;
}

void setup() {
  Serial.begin(115200);
  pinMode(wFsensor, INPUT_PULLUP);

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

  attachInterrupt(digitalPinToInterrupt(wFsensor), flowCounter, FALLING);

  xTaskCreatePinnedToCore( bacaSensor, "Baca Sensor", 10000, NULL, 1, &Task_00, 0);
  xTaskCreatePinnedToCore( komunikasi, "Komunikasi Wifi", 10000, NULL, 1, &Task_01, 1);

}

void loop() {
  //nothing to do
}

void bacaSensor( void * pvParameters ) {
  Serial.print("Task membaca sensor berada pada Core : ");
  Serial.println(xPortGetCoreID());
  delay(1000);
  while (1) {
    attachInterrupt(digitalPinToInterrupt(wFsensor), flowCounter, FALLING);
    delay(1000);
    detachInterrupt(digitalPinToInterrupt(wFsensor));
    debit = (debit_counter * 60 / 30);
    total += debit;
    Serial.print("nilai per detik= ");
    Serial.print(debit);
    Serial.print("   nilai total = ");
    Serial.println(total);
    debit_counter = 0;

    if (time_counter >= 60) {
      sendToServer(myId, total);
      time_counter = 0;
    }
    time_counter ++;
  }
}

void komunikasi( void * pvParameters ) {                    // penerima data dari node lora
  Serial.print("Task komunikasi penerima lora berada pada Core : ");
  Serial.println(xPortGetCoreID());
  delay(1000);
  while (1) {
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
    }
  }
  //end if
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
