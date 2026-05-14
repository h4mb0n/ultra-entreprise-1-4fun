#include <ESP8266WiFi.h>

#include "credentials.h"

#define TCP_PORT      23
#define SERIAL_BAUD   9600

// UART0 swap mode: RX=GPIO13(D7), TX=GPIO15(D8) — matches YL-97 wiring
#define TX_PIN        15

WiFiServer server(TCP_PORT);
WiFiClient client;

// Generate RS-232 BREAK: hold TX LOW for ~250 ms.
// Serial.end() releases the UART so we can drive the pin directly.
void sendBreak() {
  Serial.flush();
  Serial.end();
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  delay(250);
  Serial.begin(SERIAL_BAUD);
  Serial.swap();  // restore GPIO13/GPIO15
}

void setup() {
  // Debug on USB (GPIO1/GPIO3) until WiFi is up
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

  server.begin();
  server.setNoDelay(true);
}

// Escape protocol: 0x7E 0x42 ('~B') → BREAK
// 0x7E 0x7E                         → literal 0x7E
static bool escPending = false;

void loop() {
  if (!client || !client.connected()) {
    client = server.accept();
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
      } else {                // unknown → pass both bytes through
        Serial.write(0x7E);
        Serial.write(b);
      }
      continue;
    }

    if (b == 0x7E) {
      escPending = true;
    } else {
      Serial.write(b);
    }
  }

  // Serial → TCP
  while (Serial.available()) {
    client.write((uint8_t)Serial.read());
  }
}
