// ═══════════════════════════════════════════════════════════════════
//  Woonkamer Lamp Controller v1.3 -Koen Verhallen
//
//  ESP32 projectje om de Action lampen in de woonkamer te bedienen
//  met een drukknop. Nu met OTA updates, deep sleep en batterij
//  monitoring.
//
//  Wat doet het:
//    Kort drukken  → wissel tussen lampen
//    Lang drukken  → dimmen (omlaag / omhoog afwisselend)
//    Dubbel druk   → alles uit
//
//  Features:
//    - WiFi Manager (configuratie via telefoon)
//    - OTA firmware updates via WiFi
//    - Deep sleep met wake-up via knop
//    - Batterij monitoring
//
//  Hardware: ESP32 DevKit V1 + drukknop op GPIO4
// ═══════════════════════════════════════════════════════════════════

#include <WiFi.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "EspTuya.h"

// ── Feature flags ────────────────────────────────────────────────
#define FEATURE_OTA          true
#define FEATURE_DEEP_SLEEP   true
#define FEATURE_BATTERIJ     true

// ── Tuya DataPoints ──────────────────────────────────────────────
#define DP_POWER  20   // aan/uit (bool)
#define DP_DIM    22   // helderheid (int, 10-1000)

// ── Lampen in de woonkamer ───────────────────────────────────────
struct Lamp {
  const char* naam;
  const char* ip;
  const char* localKey;
  int         versie;
};

Lamp lampen[] = {
  { "Hoofdlicht", "192.168.1.10", "vervang_met_jouw_key_1", 4 },
  { "Sfeerlamp",  "192.168.1.11", "vervang_met_jouw_key_2", 4 },
  { "Leeslamp",   "192.168.1.12", "vervang_met_jouw_key_3", 4 },
};

const int AANTAL_LAMPEN = sizeof(lampen) / sizeof(lampen[0]);

// ── Knop instellingen ────────────────────────────────────────────
#define KNOP_PIN        4
#define DEBOUNCE_MS     50
#define LANG_DRUK_MS    800    // langer dan 800ms = lang drukken
#define DUBBEL_WINDOW   400    // binnen 400ms nogmaals = dubbel

// ── Batterij instellingen ────────────────────────────────────────
#define BATTERIJ_PIN     35
#define BAT_LAAG_VOLT    3.3      // onder 3.3V = waarschuwing
#define BAT_KRITIEK_VOLT 3.0      // onder 3.0V = uitschakelen
#define BAT_CHECK_MS     30000    // elke 30s checken
#define BAT_ADC_FACTOR   2.0      // spanningsdeler: 100kΩ + 100kΩ
#define BAT_ADC_REF      3.3

// ── Deep sleep instellingen ──────────────────────────────────────
#define SLAAP_TIMEOUT_MS  60000   // na 60s zonder activiteit → slapen
#define SLAAP_NA_UIT_MS   5000    // 5s na "alles uit" → slapen

// ── Huidige status ───────────────────────────────────────────────
int  activeLamp      = -1;     // -1 = alles uit
int  helderheid      = 100;    // 0-100 %
bool lampAan[3]      = {false};
bool dimRichting     = false;  // false = omlaag, true = omhoog

// Knop timing
unsigned long knopNeerTijd   = 0;
unsigned long knopLosTijd    = 0;
bool          wasIngedrukt   = false;
bool          wachtOpDubbel  = false;

// Timers
unsigned long laatsteActiviteit = 0;
unsigned long laatsteBatCheck   = 0;
float         batterijVolt      = 4.2;

// WiFi & opslag
WiFiManager   wifiManager;
Preferences   prefs;

// ── Lamp aansturen via Tuya ──────────────────────────────────────

void stuurAanUit(int idx, bool aan) {
  Serial.printf("[%s] → %s\n", lampen[idx].naam, aan ? "AAN" : "UIT");
  EspTuya tuya;
  tuya.begin(lampen[idx].ip, lampen[idx].localKey, lampen[idx].versie);
  tuya.setBool(DP_POWER, aan);
  lampAan[idx] = aan;
}

void stuurHelderheid(int idx, int pct) {
  int tuyaWaarde = map(pct, 0, 100, 10, 1000);
  Serial.printf("[%s] dim → %d%% (tuya: %d)\n", lampen[idx].naam, pct, tuyaWaarde);
  EspTuya tuya;
  tuya.begin(lampen[idx].ip, lampen[idx].localKey, lampen[idx].versie);
  if (!lampAan[idx]) {
    tuya.setBool(DP_POWER, true);
    lampAan[idx] = true;
    delay(200);
  }
  tuya.setInt(DP_DIM, tuyaWaarde);
}

void allesUit() {
  for (int i = 0; i < AANTAL_LAMPEN; i++) {
    if (lampAan[i]) stuurAanUit(i, false);
  }
  activeLamp = -1;
  helderheid = 100;
  dimRichting = false;
  Serial.println("Alles uit.");
}

// ── Wat de knop doet ─────────────────────────────────────────────

// Kort drukken: wissel naar de volgende lamp (of alles uit)
void actieKort() {
  int vorigeActief = activeLamp;
  activeLamp++;
  if (activeLamp >= AANTAL_LAMPEN) activeLamp = -1;

  if (activeLamp == -1) {
    allesUit();
  } else {
    if (vorigeActief >= 0 && vorigeActief != activeLamp) {
      stuurAanUit(vorigeActief, false);
    }
    helderheid = 100;
    dimRichting = false;
    stuurHelderheid(activeLamp, helderheid);
    Serial.printf("Nu actief: %s\n", lampen[activeLamp].naam);
  }
}

// Lang drukken: dim de actieve lamp (afwisselend omlaag/omhoog)
void actieLang() {
  if (activeLamp < 0) {
    Serial.println("Niks aan, kan niet dimmen.");
    return;
  }
  if (dimRichting) {
    helderheid = min(100, helderheid + 20);
    if (helderheid >= 100) dimRichting = false;
  } else {
    helderheid = max(10, helderheid - 20);
    if (helderheid <= 10) dimRichting = true;
  }
  stuurHelderheid(activeLamp, helderheid);
}

// Dubbel drukken: alles uit
void actieDubbel() {
  allesUit();
  Serial.println("Dubbel gedrukt, alles uit.");
}

// ── Batterij monitoring ──────────────────────────────────────────

float leesBatterij() {
  if (!FEATURE_BATTERIJ) return 4.2;
  int raw = analogRead(BATTERIJ_PIN);
  float volt = (raw / 4095.0) * BAT_ADC_REF * BAT_ADC_FACTOR;
  return volt;
}

void checkBatterij() {
  if (!FEATURE_BATTERIJ) return;
  batterijVolt = leesBatterij();
  Serial.printf("Batterij: %.2fV\n", batterijVolt);

  if (batterijVolt < BAT_KRITIEK_VOLT) {
    Serial.println("Batterij kritiek! Ga slapen...");
    gaSlapen();
  } else if (batterijVolt < BAT_LAAG_VOLT) {
    Serial.println("Batterij laag!");
  }
}

// ── Deep sleep ───────────────────────────────────────────────────

void gaSlapen() {
  if (!FEATURE_DEEP_SLEEP) return;
  Serial.println("Ga slapen... druk op de knop om wakker te worden.");
  Serial.flush();
  esp_sleep_enable_ext0_wakeup((gpio_num_t)KNOP_PIN, LOW);
  esp_deep_sleep_start();
}

// ── WiFi verbinding (via WiFi Manager) ───────────────────────────

void verbindWiFi() {
  wifiManager.setConfigPortalTimeout(120);
  wifiManager.setAPCallback([](WiFiManager* mgr) {
    Serial.println("WiFi niet geconfigureerd.");
    Serial.println("Verbind met 'LampController-Setup' op je telefoon.");
  });

  // Probeert opgeslagen WiFi, anders opent configuratie portaal
  if (!wifiManager.autoConnect("LampController-Setup")) {
    Serial.println("WiFi configuratie mislukt, herstart...");
    delay(3000);
    ESP.restart();
  }
  Serial.printf("Verbonden! IP: %s\n", WiFi.localIP().toString().c_str());
}

// ── OTA updates ──────────────────────────────────────────────────

void setupOTA() {
  if (!FEATURE_OTA) return;
  ArduinoOTA.setHostname("lamp-controller");
  ArduinoOTA.onStart([]() {
    Serial.println("OTA update gestart...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("OTA update klaar! Herstart...");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA: %u%%\r", progress / (total / 100));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA fout [%u]\n", error);
  });
  ArduinoOTA.begin();
  Serial.println("OTA klaar (upload via Arduino IDE of PlatformIO)");
}

// ── Setup ────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Woonkamer Lamp Controller v1.3 -Koen Verhallen ===");

  // Check of we wakker worden uit deep sleep
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("Wakker geworden door knop!");
  }

  pinMode(KNOP_PIN, INPUT_PULLUP);

  // WiFi (via WiFi Manager portaal)
  verbindWiFi();

  // OTA updates
  setupOTA();

  // Batterij eerste meting
  if (FEATURE_BATTERIJ) {
    analogReadResolution(12);
    checkBatterij();
  }

  laatsteActiviteit = millis();

  Serial.println("Klaar!");
  Serial.println("  Kort drukken   = volgende lamp");
  Serial.println("  Lang drukken   = dimmen (omlaag/omhoog)");
  Serial.println("  Dubbel drukken = alles uit");
  if (FEATURE_OTA)        Serial.println("  OTA updates    = aan");
  if (FEATURE_DEEP_SLEEP) Serial.printf( "  Deep sleep     = na %ds inactiviteit\n", SLAAP_TIMEOUT_MS / 1000);
}

// ── Loop ─────────────────────────────────────────────────────────

void loop() {
  // OTA afhandelen
  if (FEATURE_OTA) ArduinoOTA.handle();

  // WiFi check
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi weg, herverbinden...");
    verbindWiFi();
  }

  // Batterij check (elke 30s)
  if (FEATURE_BATTERIJ && (millis() - laatsteBatCheck >= BAT_CHECK_MS)) {
    checkBatterij();
    laatsteBatCheck = millis();
  }

  // Deep sleep check (na timeout zonder activiteit)
  if (FEATURE_DEEP_SLEEP && activeLamp == -1) {
    if (millis() - laatsteActiviteit >= SLAAP_TIMEOUT_MS) {
      gaSlapen();
    }
  }

  bool ingedrukt = (digitalRead(KNOP_PIN) == LOW);
  unsigned long nu = millis();

  // Knop ingedrukt
  if (ingedrukt && !wasIngedrukt) {
    delay(DEBOUNCE_MS);
    if (digitalRead(KNOP_PIN) == LOW) {
      knopNeerTijd = nu;
      laatsteActiviteit = nu;
    }
  }

  // Knop losgelaten
  if (!ingedrukt && wasIngedrukt) {
    unsigned long duur = nu - knopNeerTijd;
    laatsteActiviteit = nu;

    if (duur >= LANG_DRUK_MS) {
      actieLang();
      wachtOpDubbel = false;
    } else {
      if (wachtOpDubbel && (nu - knopLosTijd) < DUBBEL_WINDOW) {
        actieDubbel();
        wachtOpDubbel = false;
      } else {
        wachtOpDubbel = true;
        knopLosTijd   = nu;
      }
    }
  }

  // Geen tweede druk gekomen, dan was het een korte druk
  if (wachtOpDubbel && (nu - knopLosTijd) >= DUBBEL_WINDOW) {
    actieKort();
    wachtOpDubbel = false;
  }

  wasIngedrukt = ingedrukt;
  delay(10);
}

// ═══════════════════════════════════════════════════════════════════
//  Bedrading
//  ─────────
//  GPIO4  ── knop ── 10kΩ ── 3V3        (drukknop + pull-up)
//  GND    ── knop (andere pin)
//  GPIO35 ── spanningsdeler ── batterij  (100kΩ + 100kΩ)
//
//  Voeding: Micro-USB of 18650 via TP4056
// ═══════════════════════════════════════════════════════════════════
