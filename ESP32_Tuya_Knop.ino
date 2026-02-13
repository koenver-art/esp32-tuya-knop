// ═══════════════════════════════════════════════════════════════════
//  Woonkamer Lamp Controller v1.4 -Koen Verhallen
//
//  Mijn ESP32 projectje om de Action lampen in de woonkamer te
//  bedienen met een enkele drukknop. Geen cloud, geen app, gewoon
//  een knopje op de muur en klaar.
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
//    - Batterij monitoring met LED waarschuwing
//    - Status LEDs
//    - Foutafhandeling bij onbereikbare lampen
//
//  Hardware: ESP32 DevKit V1 + drukknop op GPIO4
// ═══════════════════════════════════════════════════════════════════

#include <WiFi.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "EspTuya.h"

// ── Features aan/uit ──────────────────────────────────────────────
#define FEATURE_OTA          true
#define FEATURE_DEEP_SLEEP   true
#define FEATURE_BATTERIJ     true

// ── Tuya DataPoints ──────────────────────────────────────────────
#define DP_POWER  20   // aan/uit (bool)
#define DP_DIM    22   // helderheid (int, 10-1000)

// ── Lampen ───────────────────────────────────────────────────────
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

// ── Pin definities ──────────────────────────────────────────────
#define KNOP_PIN        4      // GPIO4  - drukknop (ext0 wakeup)
#define LED_1_PIN       16     // GPIO16 - status LED Hoofdlicht
#define LED_2_PIN       17     // GPIO17 - status LED Sfeerlamp
#define LED_3_PIN       18     // GPIO18 - status LED Leeslamp
#define BATTERIJ_PIN    35     // GPIO35 - ADC batterijspanning

const int ledPinnen[] = { LED_1_PIN, LED_2_PIN, LED_3_PIN };

// ── Knop timing ─────────────────────────────────────────────────
#define LANG_DRUK_MS    800
#define DUBBEL_WINDOW   400
#define DEBOUNCE_MS     50

#define SLAAP_TIMEOUT_MS  60000   // deep sleep na 60s inactiviteit

// ── Batterij ────────────────────────────────────────────────────
#define BAT_LAAG_VOLT    3.3
#define BAT_KRITIEK_VOLT 3.0
#define BAT_CHECK_MS     30000
#define BAT_ADC_FACTOR   2.0      // spanningsdeler 100kΩ + 100kΩ
#define BAT_ADC_REF      3.3

// ── Huidige status ──────────────────────────────────────────────
int  activeLamp      = -1;     // -1 = alles uit
int  helderheid      = 100;    // 0-100 %
bool lampAan[3]      = {false};
bool dimRichting     = false;  // false = omlaag, true = omhoog

unsigned long knopNeerTijd   = 0;
unsigned long knopLosTijd    = 0;
bool          wasIngedrukt   = false;
bool          wachtOpDubbel  = false;

unsigned long laatsteActiviteit = 0;
unsigned long laatsteBatCheck   = 0;
float         batterijVolt      = 4.2;

WiFiManager   wifiManager;
Preferences   prefs;

// ── Status LEDs ─────────────────────────────────────────────────
void updateLEDs() {
  for (int i = 0; i < AANTAL_LAMPEN && i < 3; i++) {
    digitalWrite(ledPinnen[i], lampAan[i] ? HIGH : LOW);
  }
}

void knipperLED(int pin, int keer, int ms) {
  for (int i = 0; i < keer; i++) {
    digitalWrite(pin, HIGH);
    delay(ms);
    digitalWrite(pin, LOW);
    if (i < keer - 1) delay(ms);
  }
}

void knipperAlleLEDs(int keer, int ms) {
  for (int i = 0; i < keer; i++) {
    for (int j = 0; j < 3; j++) digitalWrite(ledPinnen[j], HIGH);
    delay(ms);
    for (int j = 0; j < 3; j++) digitalWrite(ledPinnen[j], LOW);
    if (i < keer - 1) delay(ms);
  }
}

// ── Lamp aansturen via Tuya ─────────────────────────────────────
bool stuurAanUit(int idx, bool aan) {
  if (idx < 0 || idx >= AANTAL_LAMPEN) return false;
  Serial.printf("[%s] → %s\n", lampen[idx].naam, aan ? "AAN" : "UIT");

  EspTuya tuya;
  if (!tuya.begin(lampen[idx].ip, lampen[idx].localKey, lampen[idx].versie)) {
    Serial.printf("[%s] Fout: kan niet verbinden!\n", lampen[idx].naam);
    knipperLED(ledPinnen[idx < 3 ? idx : 0], 5, 80);
    return false;
  }

  if (!tuya.setBool(DP_POWER, aan)) {
    Serial.printf("[%s] Fout: commando mislukt!\n", lampen[idx].naam);
    knipperLED(ledPinnen[idx < 3 ? idx : 0], 5, 80);
    return false;
  }

  lampAan[idx] = aan;
  updateLEDs();
  return true;
}

bool stuurHelderheid(int idx, int pct) {
  if (idx < 0 || idx >= AANTAL_LAMPEN) return false;
  int tuyaWaarde = map(pct, 0, 100, 10, 1000);
  Serial.printf("[%s] dim → %d%% (tuya: %d)\n", lampen[idx].naam, pct, tuyaWaarde);

  EspTuya tuya;
  if (!tuya.begin(lampen[idx].ip, lampen[idx].localKey, lampen[idx].versie)) {
    Serial.printf("[%s] Fout: kan niet verbinden!\n", lampen[idx].naam);
    knipperLED(ledPinnen[idx < 3 ? idx : 0], 5, 80);
    return false;
  }

  if (!lampAan[idx]) {
    tuya.setBool(DP_POWER, true);
    lampAan[idx] = true;
    delay(200);
  }

  if (!tuya.setInt(DP_DIM, tuyaWaarde)) {
    Serial.printf("[%s] Fout: dim commando mislukt!\n", lampen[idx].naam);
    knipperLED(ledPinnen[idx < 3 ? idx : 0], 5, 80);
    return false;
  }

  updateLEDs();
  return true;
}

void allesUit() {
  for (int i = 0; i < AANTAL_LAMPEN; i++) {
    if (lampAan[i]) stuurAanUit(i, false);
  }
  activeLamp = -1;
  helderheid = 100;
  dimRichting = false;
  updateLEDs();
  Serial.println("Alles uit.");
}

// ── Wat de knop doet ────────────────────────────────────────────
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

void actieDubbel() {
  allesUit();
  Serial.println("Dubbel gedrukt, alles uit.");
}

// ── Batterij monitoring ─────────────────────────────────────────
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
    knipperAlleLEDs(10, 50);
    gaSlapen();
  } else if (batterijVolt < BAT_LAAG_VOLT) {
    Serial.println("Batterij laag!");
    knipperAlleLEDs(3, 150);
    updateLEDs();
  }
}

// ── Deep sleep ──────────────────────────────────────────────────
void gaSlapen() {
  if (!FEATURE_DEEP_SLEEP) return;
  Serial.println("Ga slapen... druk op de knop om wakker te worden.");
  Serial.flush();

  for (int i = 0; i < 3; i++) digitalWrite(ledPinnen[i], LOW);

  esp_sleep_enable_ext0_wakeup((gpio_num_t)KNOP_PIN, LOW);
  esp_deep_sleep_start();
}

// ── WiFi verbinding (via WiFi Manager) ──────────────────────────
void verbindWiFi() {
  wifiManager.setConfigPortalTimeout(120);
  wifiManager.setAPCallback([](WiFiManager* mgr) {
    Serial.println("WiFi niet geconfigureerd.");
    Serial.println("Verbind met 'LampController-Setup' op je telefoon.");
    knipperAlleLEDs(2, 300);
  });

  if (!wifiManager.autoConnect("LampController-Setup")) {
    Serial.println("WiFi configuratie mislukt, herstart...");
    delay(3000);
    ESP.restart();
  }

  Serial.printf("Verbonden! IP: %s\n", WiFi.localIP().toString().c_str());
}

// ── OTA updates ─────────────────────────────────────────────────
void setupOTA() {
  if (!FEATURE_OTA) return;
  ArduinoOTA.setHostname("lamp-controller");
  ArduinoOTA.onStart([]() {
    Serial.println("OTA update gestart...");
    knipperAlleLEDs(3, 100);
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

// ── Setup ───────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Woonkamer Lamp Controller v1.4 -Koen Verhallen ===");
  // Check of we wakker worden uit deep sleep
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("Wakker geworden door knop!");
  }
  // Pinnen instellen
  pinMode(KNOP_PIN, INPUT_PULLUP);
  for (int i = 0; i < 3; i++) {
    pinMode(ledPinnen[i], OUTPUT);
    digitalWrite(ledPinnen[i], LOW);
  }
  // Opstart animatie
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPinnen[i], HIGH);
    delay(150);
  }
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPinnen[i], LOW);
    delay(150);
  }

  verbindWiFi();
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

// ── Loop ────────────────────────────────────────────────────────
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
//  GPIO16 ── 220Ω ── LED 1 ── GND       (status Hoofdlicht)
//  GPIO17 ── 220Ω ── LED 2 ── GND       (status Sfeerlamp)
//  GPIO18 ── 220Ω ── LED 3 ── GND       (status Leeslamp)
//  GPIO35 ── spanningsdeler ── batterij  (100kΩ + 100kΩ)
//
//  Voeding: Micro-USB of 18650 via TP4056
// ═══════════════════════════════════════════════════════════════════
