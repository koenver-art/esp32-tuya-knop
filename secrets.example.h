// ═══════════════════════════════════════════════════════════════════
//  Secrets -Woonkamer Lamp Controller (M5Stack Atom Lite)
//  Koen Verhallen, 2026
//
//  STAP 1: Kopieer dit bestand naar secrets.h
//  STAP 2: Vul je eigen gegevens in
//  STAP 3: secrets.h staat in .gitignore → wordt niet meegecommit
//
//  Local keys ophalen:
//    pip install tinytuya
//    python -m tinytuya wizard
//    (gratis iot.tuya.com account nodig)
//
//  BLE MAC-adressen ophalen:
//    Gebruik nRF Connect app op je telefoon → kijk bij "Advertiser Address"
//    Of kijk in je telefoon instellingen bij "Over dit apparaat"
// ═══════════════════════════════════════════════════════════════════

#ifndef SECRETS_H
#define SECRETS_H

#include "config.h"  // voor struct Lamp en struct BleApparaat

// ── Lampen in de woonkamer ────────────────────────────────────────
// Vul het IP-adres en de local key in van elke lamp.
// Versie is bijna altijd 4 voor de Action LSC lampen.
//
// Tip: geef je lampen een vast IP in je router (DHCP reservering),
//      dan verandert het IP niet na een herstart.

Lamp lampen[] = {
  { "Hoofdlicht", "192.168.1.10", "vervang_met_jouw_key_1", 4 },
  { "Sfeerlamp",  "192.168.1.11", "vervang_met_jouw_key_2", 4 },
  { "Leeslamp",   "192.168.1.12", "vervang_met_jouw_key_3", 4 },
};

// ── BLE apparaten (aanwezigheidsdetectie) ─────────────────────────
// Voeg het MAC-adres toe van elke telefoon die je wilt tracken.
// Let op: sommige telefoons roteren hun MAC-adres (privacy).
//   - iPhone: Instellingen → Wi-Fi → (i) → gebruik privé-adres UIT
//   - Android: verschilt per merk, zoek "Bluetooth MAC adres"
//
// Tip: gebruik nRF Connect om het juiste BLE MAC-adres te vinden.

BleApparaat bleApparaten[] = {
  { "Koen",  "AA:BB:CC:DD:EE:01" },
  { "Lotte", "AA:BB:CC:DD:EE:02" },
};

// ── Locatie (voor zonsberekening) ────────────────────────────────
// Breedtegraad en lengtegraad van je woning.
// Opzoeken via Google Maps: rechtermuisklik → coördinaten kopiëren.
#define LOCATIE_LAT   51.4416    // breedtegraad (bijv. Eindhoven)
#define LOCATIE_LON    5.4697    // lengtegraad

// Tijdzone (POSIX formaat voor automatische zomer/wintertijd)
// Nederland: CET-1CEST,M3.5.0/02,M10.5.0/03
#define TIJDZONE  "CET-1CEST,M3.5.0/02,M10.5.0/03"

// ── MQTT (voor Home Assistant) ────────────────────────────────────
// Alleen nodig als FEATURE_MQTT aan staat in config.h
// Als je geen gebruikersnaam nodig hebt, laat dan leeg.

#define MQTT_SERVER      "homeassistant.local"
#define MQTT_PORT        1883
#define MQTT_USER        ""
#define MQTT_PASS        ""

#endif // SECRETS_H
