// ═══════════════════════════════════════════════════════════════════
//  Woonkamer Lamp Controller v1.2 -Koen Verhallen
//
//  ESP32 projectje om de Action lampen in de woonkamer te bedienen
//  met een drukknop. Nu met WiFi Manager zodat je het netwerk via
//  je telefoon kunt instellen in plaats van hardcoded.
//
//  Wat doet het:
//    Kort drukken  → wissel tussen lampen
//    Lang drukken  → dimmen (omlaag / omhoog afwisselend)
//    Dubbel druk   → alles uit
//
//  Features:
//    - WiFi Manager (configuratie via telefoon)
//
//  Hardware: ESP32 DevKit V1 + drukknop op GPIO4
// ═══════════════════════════════════════════════════════════════════

#include <WiFi.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include "EspTuya.h"

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

// WiFi
WiFiManager wifiManager;

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

// ── Setup ────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Woonkamer Lamp Controller v1.2 -Koen Verhallen ===");

  pinMode(KNOP_PIN, INPUT_PULLUP);

  // WiFi (via WiFi Manager portaal)
  verbindWiFi();

  Serial.println("Klaar!");
  Serial.println("  Kort drukken   = volgende lamp");
  Serial.println("  Lang drukken   = dimmen (omlaag/omhoog)");
  Serial.println("  Dubbel drukken = alles uit");
}

// ── Loop ─────────────────────────────────────────────────────────

void loop() {
  // WiFi check
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi weg, herverbinden...");
    verbindWiFi();
  }

  bool ingedrukt = (digitalRead(KNOP_PIN) == LOW);
  unsigned long nu = millis();

  // Knop ingedrukt
  if (ingedrukt && !wasIngedrukt) {
    delay(DEBOUNCE_MS);
    if (digitalRead(KNOP_PIN) == LOW) {
      knopNeerTijd = nu;
    }
  }

  // Knop losgelaten
  if (!ingedrukt && wasIngedrukt) {
    unsigned long duur = nu - knopNeerTijd;

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
//
//  Voeding: Micro-USB
// ═══════════════════════════════════════════════════════════════════
