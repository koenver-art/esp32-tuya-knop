// ═══════════════════════════════════════════════════════════════════
//  Configuratie -Woonkamer Lamp Controller (M5Stack Atom Lite)
//  Frank Geujen, 2026
//
//  Alle instellingen op één plek. Pas hier aan wat je nodig hebt.
//  Gevoelige gegevens (wachtwoorden, keys) staan in secrets.h
// ═══════════════════════════════════════════════════════════════════

#ifndef CONFIG_H
#define CONFIG_H

// ── Versie ────────────────────────────────────────────────────────
#define FIRMWARE_VERSIE  "3.0.0"
#define FIRMWARE_NAAM    "Woonkamer Lamp Controller"
#define DEVICE_ID        "lamp-controller"

// ── Features aan/uit ──────────────────────────────────────────────
// Zet op false om een feature uit te schakelen
#define FEATURE_OTA          true
#define FEATURE_MQTT         true
#define FEATURE_BLE          true     // BLE telefoonbediening (GATT server)
#define FEATURE_PRESENCE     true     // BLE aanwezigheidsdetectie (via service UUID voor Apple apparaten)
#define FEATURE_ZONBEREKENING true    // Zonsopkomst/-ondergang berekening
#define FEATURE_WEB          true     // Webinterface met API

// ── Lampen ────────────────────────────────────────────────────────
#define MAX_LAMPEN  3     // maximaal aantal lampen (pas aan in secrets.h)

// ── Pin definities (M5Stack Atom Lite) ──────────────────────────
#define KNOP_PIN        39     // G39  - ingebouwde drukknop (actief laag)
#define RGB_LED_PIN     27     // G27  - ingebouwde SK6812 RGB LED
// G12 = IR LED (gereserveerd voor toekomstig gebruik)
// G26/G32 = Grove poort (gereserveerd voor toekomstig gebruik)

// ── RGB LED kleuren per lamp ────────────────────────────────────
// CRGB formaat (rood, groen, blauw) - pas aan naar smaak
#define KLEUR_LAMP_1    CRGB(255, 0, 0)      // rood = Hoofdlicht
#define KLEUR_LAMP_2    CRGB(0, 255, 0)       // groen = Sfeerlamp
#define KLEUR_LAMP_3    CRGB(0, 0, 255)       // blauw = Leeslamp
#define KLEUR_ALLES_UIT CRGB(255, 255, 255)   // wit flash bij dubbel druk
#define KLEUR_FOUT      CRGB(255, 80, 0)      // oranje bij fout
#define KLEUR_WIFI      CRGB(0, 255, 255)     // cyaan bij WiFi verbinden
#define KLEUR_BLE       CRGB(128, 0, 255)     // paars bij BLE activiteit
#define RGB_HELDERHEID  40                     // globale helderheid (0-255)

// ── Knop timing ───────────────────────────────────────────────────
#define LANG_DRUK_MS    800    // langer dan 800ms = lang drukken
#define DUBBEL_WINDOW   400    // binnen 400ms nogmaals = dubbel
#define DEBOUNCE_MS     50

// ── Tuya DataPoints ───────────────────────────────────────────────
// Specifiek voor de Action LSC Smart Connect lampen
#define DP_POWER    20   // aan/uit (bool)
#define DP_MODE     21   // modus: "white", "colour", "scene" (string)
#define DP_DIM      22   // helderheid (int, 10-1000)
#define DP_TEMP     23   // kleurtemperatuur (int, 0=warm - 1000=koud)
#define DP_COLOUR   24   // kleurdata (string, HSV: "HHHHSSSSVVVV" hex)
#define DP_SCENE    25   // scene data (string)

// ── BLE configuratie ────────────────────────────────────────────
#define BLE_DEVICE_NAAM      "LampController"   // naam zichtbaar op telefoon
#define MAX_BLE_APPARATEN    4                   // max. bekende apparaten

// BLE GATT UUIDs (gegenereerd, uniek voor dit project)
#define SERVICE_UUID         "a1b2c3d4-e5f6-7890-abcd-ef1234567890"
#define CHAR_LAMP_UUID       "a1b2c3d4-e5f6-7890-abcd-ef1234567891"  // lamp selectie (uint8: 0=uit, 1-3=lamp)
#define CHAR_HELDERHEID_UUID "a1b2c3d4-e5f6-7890-abcd-ef1234567892"  // helderheid (uint8: 0-100)
#define CHAR_POWER_UUID      "a1b2c3d4-e5f6-7890-abcd-ef1234567893"  // aan/uit (uint8: 0=uit, 1=aan)
#define CHAR_COMMANDO_UUID   "a1b2c3d4-e5f6-7890-abcd-ef1234567894"  // tekst commando (string)
#define CHAR_STATUS_UUID     "a1b2c3d4-e5f6-7890-abcd-ef1234567895"  // status uitlezen (read/notify)

// ── Aanwezigheidsdetectie ───────────────────────────────────────
#define SCAN_DUUR_MS         5000     // BLE scan duur per burst (5s)
#define SCAN_INTERVAL_MS     30000    // tijd tussen scans (30s)
#define PRESENCE_TIMEOUT_MS  300000   // 5 minuten geen signaal = afwezig
#define OVERRIDE_TIMEOUT_MS  1800000  // 30 min geen auto-on na handmatige uit

// BLE scan parameters (voor WiFi coëxistentie)
// Scan window moet kleiner zijn dan scan interval
#define BLE_SCAN_INTERVAL    0x80     // 80ms scan interval
#define BLE_SCAN_WINDOW      0x10     // 10ms scan window (~87% WiFi tijd)

// ── Zonberekening ───────────────────────────────────────────────
#define ZON_CHECK_INTERVAL_MS 3600000  // herbereken elke uur (1u)
#define NTP_SYNC_INTERVAL_MS  3600000  // NTP sync elke uur
#define NTP_SERVER_1         "pool.ntp.org"
#define NTP_SERVER_2         "time.nist.gov"

// ── MQTT topics ───────────────────────────────────────────────────
#define MQTT_TOPIC_BASE  "woonkamer/lampcontroller"
//
// Afgeleide topics:
//   .../status       → lampstatus (JSON, retained)
//   .../cmd          → commando's ontvangen
//   .../online       → LWT: "online" of "offline" (retained)
//   .../aanwezigheid → wie is thuis (JSON, retained)
//   .../donker       → is het donker buiten (bool, retained)

// ── MQTT QoS niveaus ──────────────────────────────────────────────
#define MQTT_QOS_STATUS  1
#define MQTT_QOS_CMD     1
#define MQTT_QOS_LWT     1

// ── Lamp struct ───────────────────────────────────────────────────
struct Lamp {
  const char* naam;
  const char* ip;
  const char* localKey;
  int         versie;
};

// ── BLE apparaat struct ─────────────────────────────────────────
// Matching op service UUID (voor Apple apparaten die MAC roteren)
// OF op MAC-adres (voor Android apparaten met vast MAC)
struct BleApparaat {
  const char* naam;            // bijv. "Frank"
  const char* serviceUUID;     // service UUID (voor Apple), of "" als MAC gebruikt wordt
  const char* macAdres;        // MAC-adres (voor Android), of "" als UUID gebruikt wordt
};

#endif // CONFIG_H
