// ═══════════════════════════════════════════════════════════════════
//  Configuratie -Woonkamer Lamp Controller
//  Koen Verhallen, 2026
//
//  Alle instellingen op één plek. Pas hier aan wat je nodig hebt.
//  Gevoelige gegevens (wachtwoorden, keys) staan in secrets.h
// ═══════════════════════════════════════════════════════════════════

#ifndef CONFIG_H
#define CONFIG_H

// ── Versie ────────────────────────────────────────────────────────
#define FIRMWARE_VERSIE  "2.1.0"
#define FIRMWARE_NAAM    "Woonkamer Lamp Controller"
#define DEVICE_ID        "lamp-controller"

// ── Features aan/uit ──────────────────────────────────────────────
// Zet op false om een feature uit te schakelen
#define FEATURE_OTA          true
#define FEATURE_DEEP_SLEEP   true
#define FEATURE_BATTERIJ     true
#define FEATURE_MQTT         true

// ── Lampen ────────────────────────────────────────────────────────
#define MAX_LAMPEN  3     // maximaal aantal lampen (pas aan in secrets.h)

// ── Pin definities ────────────────────────────────────────────────
#define KNOP_PIN        4      // GPIO4  - drukknop (ext0 wakeup)
#define LED_1_PIN       16     // GPIO16 - status LED Hoofdlicht
#define LED_2_PIN       17     // GPIO17 - status LED Sfeerlamp
#define LED_3_PIN       18     // GPIO18 - status LED Leeslamp
#define BATTERIJ_PIN    35     // GPIO35 - ADC batterijspanning

// ── Knop timing ───────────────────────────────────────────────────
#define LANG_DRUK_MS    800    // langer dan 800ms = lang drukken
#define DUBBEL_WINDOW   400    // binnen 400ms nogmaals = dubbel
#define DEBOUNCE_MS     50

// ── Deep sleep ────────────────────────────────────────────────────
#define SLAAP_TIMEOUT_MS  60000   // na 60s zonder activiteit → slapen
#define SLAAP_NA_UIT_MS   5000    // 5s na "alles uit" → slapen

// ── Batterij ──────────────────────────────────────────────────────
#define BAT_LAAG_VOLT    3.3      // onder 3.3V = waarschuwing
#define BAT_KRITIEK_VOLT 3.0      // onder 3.0V = uitschakelen
#define BAT_CHECK_MS     30000    // elke 30s checken
// Spanningsdeler: 100kΩ + 100kΩ → factor 2
#define BAT_ADC_FACTOR   2.0
#define BAT_ADC_REF      3.3

// ── Tuya DataPoints ───────────────────────────────────────────────
// Specifiek voor de Action LSC Smart Connect lampen
#define DP_POWER  20   // aan/uit (bool)
#define DP_DIM    22   // helderheid (int, 10-1000)
#define DP_MODE   21   // modus (white/colour/scene)

// ── MQTT topics ───────────────────────────────────────────────────
#define MQTT_TOPIC_BASE  "woonkamer/lampcontroller"
//
// Afgeleide topics:
//   .../status    → lampstatus (JSON, retained)
//   .../cmd       → commando's ontvangen
//   .../batterij  → batterijspanning (retained)
//   .../online    → LWT: "online" of "offline" (retained)

// ── MQTT QoS niveaus ──────────────────────────────────────────────
// QoS 0 = fire and forget, QoS 1 = at least once
#define MQTT_QOS_STATUS  1    // status: belangrijk dat het aankomt
#define MQTT_QOS_CMD     1    // commando's: at least once
#define MQTT_QOS_LWT     1    // last will: moet betrouwbaar zijn

// ── Lamp struct ───────────────────────────────────────────────────
struct Lamp {
  const char* naam;
  const char* ip;
  const char* localKey;
  int         versie;
};

#endif // CONFIG_H
