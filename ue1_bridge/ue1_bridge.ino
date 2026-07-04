#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>

#include "credentials.h"

#define TCP_PORT      23
#define SERIAL_BAUD   9600

// UART0 swap mode: RX=GPIO13(D7), TX=GPIO15(D8) — matches YL-97 wiring
#define TX_PIN        15

// After Serial.swap(), GPIO1/GPIO3 are free — CH340 USB chip still wired on GPIO1.
// SoftwareSerial here → debug output visible on /dev/ttyUSB0, no extra hardware.
SoftwareSerial dbg(3, 1); // RX=GPIO3, TX=GPIO1
#define DBG dbg

WiFiServer server(TCP_PORT);
WiFiClient client;

// Generate RS-232 BREAK: hold TX LOW for ~250 ms.
// Serial.end() releases the UART so we can drive the pin directly.
void sendBreak() {
  DBG.println("[dbg] sendBreak");
  Serial.flush();
  Serial.end();
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  delay(250);
  Serial.begin(SERIAL_BAUD);
  Serial.swap();  // restore GPIO13/GPIO15
}

void setup() {
  // Boot messages on USB UART0 until swap
  Serial.begin(115200);
  Serial.println("\n[ue1-bridge] booting...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to " WIFI_SSID);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - t0 > 15000) {
      Serial.println("\nFAILED — rebooting");
      ESP.restart();
    }
    delay(250);
    Serial.print(".");
  }
  Serial.print("\nIP: ");
  Serial.println(WiFi.localIP());
  Serial.end();

  // Switch UART0 to GPIO13(RX)/GPIO15(TX) for RS-232 bridge
  Serial.begin(SERIAL_BAUD);
  Serial.swap();

  // GPIO1/GPIO3 now free — init SoftwareSerial debug on USB CH340
  dbg.begin(9600);
  DBG.println("[dbg] ready, TCP listening on port 23");

  server.begin();
  server.setNoDelay(true);
}

// Escape protocol: 0x7E 0x42 ('~B') → BREAK
// 0x7E 0x7E                         → literal 0x7E
static bool escPending = false;
static bool wasConnected = false;

static uint32_t rxBytes = 0;   // UART → TCP
static uint32_t txBytes = 0;   // TCP  → UART
static unsigned long lastBeat = 0;

void loop() {
  // Heartbeat toutes les 5 s sur le debug serial
  unsigned long now = millis();
  if (now - lastBeat >= 5000) {
    lastBeat = now;
    bool connected = client && client.connected();
    DBG.print("[dbg] alive | client=");
    DBG.print(connected ? "yes" : "no");
    DBG.print(" | uart_rx=");
    DBG.print(rxBytes);
    DBG.print(" | uart_tx=");
    DBG.println(txBytes);
  }

  if (!client || !client.connected()) {
    if (wasConnected) {
      DBG.println("[dbg] client disconnected");
      wasConnected = false;
    }
    WiFiClient newClient = server.accept();
    if (newClient) {
      DBG.print("[dbg] client connected from ");
      DBG.println(newClient.remoteIP());
      client = newClient;
      wasConnected = true;
    }
    escPending = false;
    return;
  }

  // TCP → serial
  while (client.available()) {
    uint8_t b = client.read();

    if (escPending) {
      escPending = false;
      if (b == 0x42) {        // ~B  → BREAK
        sendBreak();
      } else if (b == 0x7E) { // ~~ → literal ~
        Serial.write(0x7E);
        txBytes++;
      } else {                // unknown → pass both bytes through
        Serial.write(0x7E);
        Serial.write(b);
        txBytes += 2;
      }
      continue;
    }

    if (b == 0x7E) {
      escPending = true;
    } else {
      Serial.write(b);
      txBytes++;
    }
  }

  // Serial → TCP
  while (Serial.available()) {
    client.write((uint8_t)Serial.read());
    rxBytes++;
  }
}
