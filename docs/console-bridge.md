# Console Bridge — WiFi RS-232 pour Sun Ultra Enterprise 1

Bridge ESP8266 → MAX232 → DB25 pour accéder à l'OBP et la console Solaris/NetBSD via TCP/WiFi.

---

## Matériel

| Composant | Référence |
|-----------|-----------|
| MCU WiFi | NodeMCU ESP8266 ESP-12E |
| Convertisseur RS-232 | YL-97 (MAX232, DB9 femelle) |
| Adaptateur | DB25 mâle ↔ DB9 femelle (liaison droite) |
| Câbles | Dupont F-F ×4 |

---

## Pinout YL-97 MAX232

### Côté TTL (header 4 broches)

```
┌─────────────────────┐
│  VCC  GND  RXD  TXD │  ← marquage sur le PCB
└──┬────┬────┬────┬───┘
   │    │    │    │
  3V3  GND  MCU  MCU
             TX   RX
```

| Broche YL-97 | Rôle | Connecte à |
|--------------|------|------------|
| VCC | Alimentation | NodeMCU **3V3** |
| GND | Masse | NodeMCU **GND** |
| RXD | Entrée TTL (vers RS-232 TX) | NodeMCU **D8 / GPIO15** (TX après swap) |
| TXD | Sortie TTL (depuis RS-232 RX) | NodeMCU **D7 / GPIO13** (RX après swap) |

> **Attention** : les labels RXD/TXD sont du point de vue du MAX232.
> RXD = ce que le MAX232 *reçoit* du MCU ; TXD = ce qu'il *envoie* au MCU.

### Côté RS-232 (DB9 femelle)

```
      DB9 femelle (vue de face, côté soudure)
         ___________
        /  1  2  3  \
       |  4  5  6   |
        \  7  8  9  /
         -----------
```

| Pin DB9 | Signal RS-232 | Utilisation |
|---------|--------------|-------------|
| 2 | RXD | Reçoit depuis le NodeMCU TX → vers Sun RX |
| 3 | TXD | Transmet depuis Sun TX → vers NodeMCU RX |
| 5 | GND | Masse commune |
| 1,4,6,7,8,9 | Handshake | Non connectés (no flow control) |

---

## Câblage complet

```
Sun UE1                   Adaptateur          YL-97 MAX232      NodeMCU ESP8266
DB25 mâle                 DB25↔DB9            DB9 → TTL         (UART0 swappé)
─────────                 ────────            ─────────         ───────────────

Pin 2  TX ──────────────── Pin 2 ─────────── Pin 3 ──── TXD ── D7 / GPIO13  RX
Pin 3  RX ──────────────── Pin 3 ─────────── Pin 2 ──── RXD ── D8 / GPIO15  TX
Pin 7  GND ─────────────── Pin 5 ─────────── Pin 5 ──── GND ── GND
                                                         VCC ── 3V3
```

> L'adaptateur DB25↔DB9 est une **liaison droite** (pin 2→2, 3→3, 7→5).

---

## Firmware ESP8266

**Fichier :** `ue1_bridge/ue1_bridge.ino`

- UART0 en mode swap → RX=GPIO13 (D7), TX=GPIO15 (D8)
- Serveur TCP port **23**, mono-client
- 9600 8N1, pas de flow control

### Séquence BREAK (→ prompt OBP `ok`)

| Octets envoyés | Effet |
|----------------|-------|
| `0x7E 0x42` (`~B`) | BREAK UART 250 ms |
| `0x7E 0x7E` (`~~`) | Tilde littéral `~` |

Le BREAK est généré en relâchant l'UART, tirant TX=GPIO15 à LOW pendant 250 ms, puis réinitialisant le Serial.

### Credentials WiFi

Créer `ue1_bridge/credentials.h` (gitignore) à partir du template :

```bash
cp ue1_bridge/credentials.h.example ue1_bridge/credentials.h
# éditer WIFI_SSID / WIFI_PASS
```

---

## Compilation et flash

```bash
# Installer arduino-cli (si absent)
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=~/.local/bin sh

# Ajouter le core ESP8266
arduino-cli config add board_manager.additional_urls https://arduino.esp8266.com/stable/package_esp8266com_index.json
arduino-cli core update-index
arduino-cli core install esp8266:esp8266

# Compiler
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 ue1_bridge/

# Flasher (NodeMCU sur /dev/ttyUSB0)
arduino-cli upload --fqbn esp8266:esp8266:nodemcuv2 --port /dev/ttyUSB0 ue1_bridge/
```

---

## Connexion depuis Debian

```bash
./connect.sh              # IP par défaut : 192.168.3.136
./connect.sh 192.168.3.136
UE1_IP=192.168.3.136 ./connect.sh
```

Puis pour déclencher l'OBP :

```
~B          ← taper tilde puis B
ok          ← prompt OpenBoot
```

---

## Dépannage

| Symptôme | Cause probable | Solution |
|----------|---------------|----------|
| ESP ne se connecte pas | Réseau 5 GHz | Utiliser un SSID 2.4 GHz |
| Pas d'output console | Câblage RX/TX inversé | Vérifier TXD→GPIO13, RXD→GPIO15 |
| Caractères corrompus | Mauvais baud rate | Confirmer 9600 8N1 côté Sun |
| BREAK sans effet | Durée trop courte | Augmenter le `delay(250)` dans `sendBreak()` |
