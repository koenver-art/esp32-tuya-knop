// ═══════════════════════════════════════════════════════════════════
//  Secrets -Woonkamer Lamp Controller
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
// ═══════════════════════════════════════════════════════════════════

#ifndef SECRETS_H
#define SECRETS_H

#include "config.h"  // voor struct Lamp

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

// ── MQTT (voor Home Assistant) ────────────────────────────────────
// Alleen nodig als FEATURE_MQTT aan staat in config.h
// Als je geen gebruikersnaam nodig hebt, laat dan leeg.

#define MQTT_SERVER      "homeassistant.local"
#define MQTT_PORT        1883
#define MQTT_USER        ""
#define MQTT_PASS        ""

#endif // SECRETS_H
