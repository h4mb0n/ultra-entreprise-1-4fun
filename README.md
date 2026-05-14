![Status](https://img.shields.io/badge/Status-Restoration_In_Progress-orange)
![OS](https://img.shields.io/badge/OS-NetBSD-blue)
![CPU](https://img.shields.io/badge/CPU-SPARC_v9-purple)

# Dot-Com Survivor — « The Dot is back. »

> "We're the dot in .com" — Sun Microsystems, 1997.

![](docs/tweet.png)

---

## Description

Projet de réhabilitation d'un serveur **Sun Ultra Enterprise 1** (SPARC). La machine quitte le placard pour reprendre du service sous un noyau moderne (NetBSD / OpenBSD).

---

## Objectifs

- [x] Accès console série via WiFi (bridge ESP8266 ↔ RS-232 DB25)
- [ ] Bypass NVRAM (puce M48T59 HS)
- [ ] Boot NetBSD / OpenBSD SPARC
- [ ] Inventaire et validation du matériel SBus

---

## Console WiFi (ESP8266 bridge)

Accès OBP et console série depuis Debian via TCP, sans câble USB-série permanent.

**Hardware :** NodeMCU ESP8266 + YL-97 MAX232 + adaptateur DB25↔DB9
**IP ESP :** `192.168.3.136` — port **23**

```bash
# Se connecter
./connect.sh

# Déclencher le prompt OBP (OpenBoot)
~B          # → ok
```

Documentation complète et pinout : [docs/console-bridge.md](docs/console-bridge.md)

---

## Structure

```
.
├── ue1_bridge/
│   ├── ue1_bridge.ino        # Firmware ESP8266
│   ├── credentials.h         # WiFi SSID/PASS (gitignore)
│   └── credentials.h.example # Template
├── connect.sh                # Client socat Debian
└── docs/
    └── console-bridge.md     # Schéma, pinout, procédures
```
