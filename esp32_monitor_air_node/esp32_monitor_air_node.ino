#include <SPI.h>
#include <LoRa.h>
#include <Servo.h>
Servo myservo;
//definisikan pin LoRa
#define ss 5
#define rst 2
#define dio0 32
#define wFsensor 27  // pin sensor water flow
//#define aktuator 26

String myId = "001", nodeId = "0";
String loradata = " ", nodeData = " ";
long lastMsg = 0;
int time_counter = 0;
unsigned long debit_counter = 0;
unsigned long debit = 0;
unsigned long total = 0;
const long interval = 10000; //waktu untuk mengirim

void IRAM_ATTR flowCounter() {
  debit_counter++;
}

void setup() {
  Serial.begin(115200);

  pinMode(wFsensor, INPUT_PULLUP);
  //pinMode(aktuator, OUTPUT);
  //digitalWrite(aktuator, HIGH);
  myservo.attach(26);
  Serial.println("LoRa Node ....");
  LoRa.setPins(ss, rst, dio0);
  while (!LoRa.begin(923200000)) {
    Serial.println(".");
    delay(500);
  }

  LoRa.setSyncWord(0xEE);                         //set syncword
  Serial.println("LoRa Initializing OK!");
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
  attachInterrupt(digitalPinToInterrupt(wFsensor), flowCounter, FALLING);

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

    if (nodeId == myId) {
      if (nodeData == "on") {
        myservo.write(5);
      }
      else if (nodeData == "off") {
        myservo.write(75);
      }
      nodeId = "0";
    }
  }
  long now = millis();
  if (now - lastMsg > 1000) {
    lastMsg = now;
    detachInterrupt(digitalPinToInterrupt(wFsensor));
    debit = (debit_counter * 60 / 30);
    total += debit;
    Serial.print("nilai per detik= ");
    Serial.print(debit);
    Serial.print("   nilai total = ");
    Serial.println(total);
    debit_counter = 0;

    if (time_counter >= 60) {
      String paket = myId + ":" + String(total);
      Serial.print("Sending packet: ");
      Serial.println(paket);

      LoRa.beginPacket();
      LoRa.print(paket);
      LoRa.endPacket();
      time_counter = 0;
    }
    time_counter ++;
  }
}
