#include <SPI.h>
#include <LoRa.h>

//definisikan pin LoRa
#define ss 5
#define rst 2
#define dio0 32
#define wFsensor 27  // pin sensor water flow
#define solenoid 26

String myId = "002", nodeId = "0";
String loradata = " ", nodeData = " ";
long lastMsg = 0;
int time_counter = 0;

unsigned long pulse_counter = 0;
unsigned long ppd = 0, ppm = 0;
float lpm = 0, total = 0;

const long interval = 10000; //waktu untuk mengirim

void IRAM_ATTR flowCounter() {
  pulse_counter++;
}

void setup() {
  Serial.begin(115200);

  pinMode(wFsensor, INPUT_PULLUP);
  pinMode(solenoid, OUTPUT);
  digitalWrite(solenoid, HIGH);
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
        digitalWrite(solenoid, HIGH);
      }
      else if (nodeData == "off") {
        digitalWrite(solenoid, LOW);
      }
      nodeId = "0";
    }
  }
  long now = millis();
  if (now - lastMsg > 1000) {
    lastMsg = now;
    detachInterrupt(digitalPinToInterrupt(wFsensor));
    ppd = pulse_counter;
    ppm += ppd;
    
    Serial.print("nilai per detik= ");
    Serial.print(ppd);
    Serial.print("   nilai per menit = ");
    Serial.println(ppm);
    pulse_counter = 0;

    if (time_counter >= 60) {
      lpm = float(ppm)/477;
      total += lpm;
      String paket = myId + ":" + String(lpm) + ":" + String(total);
      Serial.print("Sending packet: ");
      Serial.println(paket);

      LoRa.beginPacket();
      LoRa.print(paket);
      LoRa.endPacket();
      lpm = 0;
      ppm=0;
      time_counter = 0;
    }
    time_counter ++;
  }
}
