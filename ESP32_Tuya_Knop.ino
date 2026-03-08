// ═══════════════════════════════════════════════════════════════════
//  Woonkamer Lamp Controller -Frank Geujen
//  v3.0.0 — M5Stack Atom Lite editie
//
//  ESP32 projectje om de Action lampen in de woonkamer te bedienen
//  met de ingebouwde drukknop, via BLE vanaf je telefoon, en met
//  automatische aanwezigheidsdetectie.
//
//  Knop (ingebouwd):
//    Kort drukken  → wissel tussen lampen
//    Lang drukken  → dimmen (omlaag / omhoog afwisselend)
//    Dubbel druk   → alles uit
//
//  BLE telefoonbediening:
//    Verbind met "LampController" via nRF Connect of andere BLE app
//    Schrijf naar characteristics om lampen te bedienen
//
//  Aanwezigheidsdetectie:
//    Scant voor bekende apparaten (bijv. AirPods)
//    Lampen automatisch aan als het donker is en iemand thuiskomt
//    Lampen automatisch uit als iedereen weg is
//
//  Hardware: M5Stack Atom Lite (ESP32-PICO-D4)
//    - Knop op G39, RGB LED op G27, IR op G12 (toekomstig)
//
//  Instellingen: zie config.h en secrets.h
// ═══════════════════════════════════════════════════════════════════

#include "config.h"
#include "secrets.h"

#include <WiFi.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <FastLED.h>
#include <time.h>
#include "EspTuya.h"

#if FEATURE_ZONBEREKENING
  #include <SolarCalculator.h>
#endif

#if FEATURE_BLE || FEATURE_PRESENCE
  #include <NimBLEDevice.h>
#endif

// ── Forward declaraties ───────────────────────────────────────────
void publiceerStatus();
void publiceerAanwezigheid();
void publiceerDonker();
void verwerkCommando(String bericht);
void updateLED();

// ── Afgeleide constanten ──────────────────────────────────────────
const int AANTAL_LAMPEN     = sizeof(lampen) / sizeof(lampen[0]);
const int AANTAL_BLE_APPARATEN = sizeof(bleApparaten) / sizeof(bleApparaten[0]);

// ── RGB LED ─────────────────────────────────────────────────────
CRGB led[1];
const CRGB lampKleuren[] = { KLEUR_LAMP_1, KLEUR_LAMP_2, KLEUR_LAMP_3 };

// ── Huidige status ────────────────────────────────────────────────
int  activeLamp      = -1;     // -1 = alles uit
int  helderheid      = 100;    // 0-100 %
bool lampAan[MAX_LAMPEN] = {false};
bool dimRichting     = false;  // false = omlaag, true = omhoog

// Knop timing
unsigned long knopNeerTijd   = 0;
unsigned long knopLosTijd    = 0;
bool          wasIngedrukt   = false;
bool          wachtOpDubbel  = false;

// Timers
unsigned long laatsteActiviteit   = 0;
unsigned long laatsteZonCheck     = 0;
unsigned long laatsteScan         = 0;
unsigned long laatsteNtpSync      = 0;
unsigned long handmatigeOverride  = 0;  // tijdstip van laatste handmatige "uit"

// ── Zonberekening ───────────────────────────────────────────────
double zonTransit          = 0.0;  // uur UTC
double burgerlijkeDageraad = 0.0;  // uur UTC
double burgerlijkeSchemer  = 0.0;  // uur UTC
bool   ntpGesynchroniseerd = false;
bool   hetIsDonker         = false;

// ── Aanwezigheid ────────────────────────────────────────────────
struct ApparaatStatus {
  unsigned long laatstGezien;
  int           rssi;
  bool          aanwezig;
};
ApparaatStatus apparaatStatus[MAX_BLE_APPARATEN] = {};
bool iemandWasThuis = false;  // vorige status voor edge-detectie
bool scanActief     = false;

// ── WiFi & MQTT ─────────────────────────────────────────────────
WiFiManager   wifiManager;
WiFiClient    espClient;
PubSubClient  mqtt(espClient);

// ── BLE ─────────────────────────────────────────────────────────
#if FEATURE_BLE
  NimBLEServer* bleServer = nullptr;
  bool bleVerbonden = false;
#endif

#if FEATURE_PRESENCE
  NimBLEScan* bleScan = nullptr;
#endif

// ═══════════════════════════════════════════════════════════════════
//  RGB LED functies
// ═══════════════════════════════════════════════════════════════════

void toonKleur(CRGB kleur) {
  led[0] = kleur;
  FastLED.show();
}

void toonUit() {
  led[0] = CRGB::Black;
  FastLED.show();
}

void knipperLED(CRGB kleur, int keer, int ms) {
  for (int i = 0; i < keer; i++) {
    toonKleur(kleur);
    delay(ms);
    toonUit();
    if (i < keer - 1) delay(ms);
  }
}

void updateLED() {
  if (activeLamp >= 0 && activeLamp < AANTAL_LAMPEN) {
    // Toon kleur van actieve lamp, helderheid geeft dimniveau aan
    CRGB kleur = lampKleuren[activeLamp];
    kleur.nscale8(map(helderheid, 0, 100, 20, 255));
    toonKleur(kleur);
  } else {
    toonUit();
  }
}

// Ademend effect (non-blocking, aanroepen vanuit loop)
uint8_t ademFase = 0;
void ademLED(CRGB kleur) {
  uint8_t val = sin8(ademFase);
  ademFase += 2;
  CRGB c = kleur;
  c.nscale8(val);
  toonKleur(c);
}

// ═══════════════════════════════════════════════════════════════════
//  Lamp aansturen via Tuya
// ═══════════════════════════════════════════════════════════════════

// Pauzeer BLE scan tijdens Tuya commando (WiFi prioriteit)
void pauzeerScan() {
  #if FEATURE_PRESENCE
    if (bleScan && bleScan->isScanning()) {
      bleScan->stop();
      scanActief = false;
    }
  #endif
}

bool stuurAanUit(int idx, bool aan) {
  if (idx < 0 || idx >= AANTAL_LAMPEN) return false;
  Serial.printf("[%s] → %s\n", lampen[idx].naam, aan ? "AAN" : "UIT");

  pauzeerScan();

  EspTuya tuya;
  if (!tuya.begin(lampen[idx].ip, lampen[idx].localKey, lampen[idx].versie)) {
    Serial.printf("[%s] Fout: kan niet verbinden!\n", lampen[idx].naam);
    knipperLED(KLEUR_FOUT, 5, 80);
    updateLED();
    return false;
  }

  if (!tuya.setBool(DP_POWER, aan)) {
    Serial.printf("[%s] Fout: commando mislukt!\n", lampen[idx].naam);
    knipperLED(KLEUR_FOUT, 5, 80);
    updateLED();
    return false;
  }

  lampAan[idx] = aan;
  updateLED();
  publiceerStatus();
  return true;
}

bool stuurHelderheid(int idx, int pct) {
  if (idx < 0 || idx >= AANTAL_LAMPEN) return false;
  int tuyaWaarde = map(pct, 0, 100, 10, 1000);
  Serial.printf("[%s] dim → %d%% (tuya: %d)\n", lampen[idx].naam, pct, tuyaWaarde);

  pauzeerScan();

  EspTuya tuya;
  if (!tuya.begin(lampen[idx].ip, lampen[idx].localKey, lampen[idx].versie)) {
    Serial.printf("[%s] Fout: kan niet verbinden!\n", lampen[idx].naam);
    knipperLED(KLEUR_FOUT, 5, 80);
    updateLED();
    return false;
  }

  if (!lampAan[idx]) {
    tuya.setBool(DP_POWER, true);
    lampAan[idx] = true;
    delay(200);
  }

  if (!tuya.setInt(DP_DIM, tuyaWaarde)) {
    Serial.printf("[%s] Fout: dim commando mislukt!\n", lampen[idx].naam);
    knipperLED(KLEUR_FOUT, 5, 80);
    updateLED();
    return false;
  }

  updateLED();
  publiceerStatus();
  return true;
}

void allesUit() {
  for (int i = 0; i < AANTAL_LAMPEN; i++) {
    if (lampAan[i]) stuurAanUit(i, false);
  }
  activeLamp = -1;
  helderheid = 100;
  dimRichting = false;
  updateLED();
  Serial.println("Alles uit.");
}

// ═══════════════════════════════════════════════════════════════════
//  Wat de knop doet
// ═══════════════════════════════════════════════════════════════════

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
  knipperLED(KLEUR_ALLES_UIT, 1, 100);
  allesUit();
  handmatigeOverride = millis();  // voorkom auto-on na handmatige uit
  Serial.println("Dubbel gedrukt, alles uit (override actief).");
}

// ═══════════════════════════════════════════════════════════════════
//  Zonberekening (burgerlijke schemering)
// ═══════════════════════════════════════════════════════════════════

void berekenZonTijden() {
  #if FEATURE_ZONBEREKENING
    if (!ntpGesynchroniseerd) return;

    struct tm ti;
    if (!getLocalTime(&ti)) return;

    // SolarCalculator werkt met UTC — gebruik gmtime
    time_t nu = time(nullptr);
    struct tm utc;
    gmtime_r(&nu, &utc);

    calcCivilDawnDusk(utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday,
                      LOCATIE_LAT, LOCATIE_LON,
                      zonTransit, burgerlijkeDageraad, burgerlijkeSchemer);

    Serial.printf("Zon: dageraad %.2f UTC, schemer %.2f UTC\n",
                  burgerlijkeDageraad, burgerlijkeSchemer);
  #endif
}

bool isDonker() {
  #if FEATURE_ZONBEREKENING
    if (!ntpGesynchroniseerd) return false;  // D-006: veilige default

    time_t nu = time(nullptr);
    struct tm utc;
    gmtime_r(&nu, &utc);

    double uurUTC = utc.tm_hour + utc.tm_min / 60.0 + utc.tm_sec / 3600.0;
    hetIsDonker = (uurUTC < burgerlijkeDageraad) || (uurUTC > burgerlijkeSchemer);
    return hetIsDonker;
  #else
    return false;
  #endif
}

// ═══════════════════════════════════════════════════════════════════
//  NTP tijdsynchronisatie
// ═══════════════════════════════════════════════════════════════════

void syncNTP() {
  Serial.println("NTP synchroniseren...");
  configTime(0, 0, NTP_SERVER_1, NTP_SERVER_2);
  setenv("TZ", TIJDZONE, 1);
  tzset();

  // Wacht max 5 seconden op sync
  struct tm ti;
  int poging = 0;
  while (!getLocalTime(&ti) && poging < 10) {
    delay(500);
    poging++;
  }

  if (poging < 10) {
    ntpGesynchroniseerd = true;
    Serial.printf("NTP OK! Lokale tijd: %02d:%02d:%02d\n", ti.tm_hour, ti.tm_min, ti.tm_sec);
    berekenZonTijden();
  } else {
    Serial.println("NTP sync mislukt, probeer later opnieuw.");
  }
}

// ═══════════════════════════════════════════════════════════════════
//  BLE GATT Server (telefoonbediening)
// ═══════════════════════════════════════════════════════════════════

#if FEATURE_BLE

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    bleVerbonden = true;
    Serial.println("BLE: telefoon verbonden");
    toonKleur(KLEUR_BLE);
    delay(200);
    updateLED();
  }

  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
    bleVerbonden = false;
    Serial.println("BLE: telefoon losgekoppeld");
    NimBLEDevice::startAdvertising();
  }
};

// Lamp selectie: schrijf 0 (uit) of 1-3 (lamp nummer)
class LampCallback : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
    uint8_t waarde = pChar->getValue<uint8_t>();
    Serial.printf("BLE: lamp → %d\n", waarde);
    laatsteActiviteit = millis();

    if (waarde == 0) {
      allesUit();
    } else if (waarde >= 1 && waarde <= AANTAL_LAMPEN) {
      int nieuweLamp = waarde - 1;
      if (activeLamp >= 0 && activeLamp != nieuweLamp) {
        stuurAanUit(activeLamp, false);
      }
      activeLamp = nieuweLamp;
      helderheid = 100;
      stuurHelderheid(activeLamp, helderheid);
    }
  }
};

// Helderheid: schrijf 0-100
class HelderheidCallback : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
    uint8_t waarde = pChar->getValue<uint8_t>();
    Serial.printf("BLE: helderheid → %d%%\n", waarde);
    laatsteActiviteit = millis();

    if (activeLamp >= 0) {
      helderheid = constrain(waarde, 0, 100);
      stuurHelderheid(activeLamp, helderheid);
    }
  }
};

// Aan/uit: schrijf 0 of 1
class PowerCallback : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
    uint8_t waarde = pChar->getValue<uint8_t>();
    Serial.printf("BLE: power → %d\n", waarde);
    laatsteActiviteit = millis();

    if (waarde == 0) {
      allesUit();
      handmatigeOverride = millis();
    } else {
      if (activeLamp < 0) activeLamp = 0;
      stuurAanUit(activeLamp, true);
    }
  }
};

// Tekst commando: "lamp1_on", "lamp2_dim_50", "alles_uit", etc.
class CommandoCallback : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
    std::string raw = pChar->getValue();
    String bericht = String(raw.c_str());
    Serial.printf("BLE: commando → %s\n", bericht.c_str());
    laatsteActiviteit = millis();
    verwerkCommando(bericht);
  }
};

NimBLECharacteristic* statusChar = nullptr;

void setupBLE() {
  Serial.println("BLE GATT server starten...");

  NimBLEDevice::init(BLE_DEVICE_NAAM);
  NimBLEDevice::setPower(ESP_PWR_LVL_P6);

  bleServer = NimBLEDevice::createServer();
  bleServer->setCallbacks(new ServerCallbacks());

  NimBLEService* service = bleServer->createService(SERVICE_UUID);

  // Lamp selectie (write)
  NimBLECharacteristic* lampChar = service->createCharacteristic(
    CHAR_LAMP_UUID, NIMBLE_PROPERTY::WRITE);
  lampChar->setCallbacks(new LampCallback());

  // Helderheid (write)
  NimBLECharacteristic* heldChar = service->createCharacteristic(
    CHAR_HELDERHEID_UUID, NIMBLE_PROPERTY::WRITE);
  heldChar->setCallbacks(new HelderheidCallback());

  // Power (write)
  NimBLECharacteristic* powerChar = service->createCharacteristic(
    CHAR_POWER_UUID, NIMBLE_PROPERTY::WRITE);
  powerChar->setCallbacks(new PowerCallback());

  // Commando (write)
  NimBLECharacteristic* cmdChar = service->createCharacteristic(
    CHAR_COMMANDO_UUID, NIMBLE_PROPERTY::WRITE);
  cmdChar->setCallbacks(new CommandoCallback());

  // Status (read + notify)
  statusChar = service->createCharacteristic(
    CHAR_STATUS_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

  service->start();

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->start();

  Serial.printf("BLE klaar: \"%s\"\n", BLE_DEVICE_NAAM);
}

void updateBLEStatus() {
  if (!statusChar || !bleVerbonden) return;

  StaticJsonDocument<128> doc;
  doc["lamp"] = activeLamp + 1;  // 0=uit, 1-3=lamp
  doc["helderheid"] = helderheid;
  doc["aan"] = (activeLamp >= 0);
  doc["donker"] = hetIsDonker;

  char buf[128];
  serializeJson(doc, buf, sizeof(buf));
  statusChar->setValue(buf);
  statusChar->notify();
}

#endif // FEATURE_BLE

// ═══════════════════════════════════════════════════════════════════
//  BLE Aanwezigheidsdetectie
// ═══════════════════════════════════════════════════════════════════

#if FEATURE_PRESENCE

// Vergelijk strings (case-insensitive)
bool stringGelijk(const char* a, const char* b) {
  if (!a || !b) return false;
  if (strlen(a) == 0 || strlen(b) == 0) return false;
  while (*a && *b) {
    if (tolower(*a) != tolower(*b)) return false;
    a++; b++;
  }
  return *a == *b;
}

// Check of apparaat matcht op service UUID of MAC-adres
bool apparaatMatcht(const NimBLEAdvertisedDevice* apparaat, int idx) {
  // Methode 1: Match op service UUID (voor Apple apparaten)
  if (strlen(bleApparaten[idx].serviceUUID) > 0) {
    NimBLEUUID zoekUUID(bleApparaten[idx].serviceUUID);
    if (apparaat->isAdvertisingService(zoekUUID)) {
      return true;
    }
  }

  // Methode 2: Match op MAC-adres (voor Android apparaten)
  if (strlen(bleApparaten[idx].macAdres) > 0) {
    std::string macStr = apparaat->getAddress().toString();
    if (stringGelijk(macStr.c_str(), bleApparaten[idx].macAdres)) {
      return true;
    }
  }

  return false;
}

class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* apparaat) override {
    for (int i = 0; i < AANTAL_BLE_APPARATEN; i++) {
      if (apparaatMatcht(apparaat, i)) {
        apparaatStatus[i].laatstGezien = millis();
        apparaatStatus[i].rssi = apparaat->getRSSI();
        if (!apparaatStatus[i].aanwezig) {
          apparaatStatus[i].aanwezig = true;
          Serial.printf("Aanwezigheid: %s is thuisgekomen (RSSI: %d)\n",
                        bleApparaten[i].naam, apparaat->getRSSI());
          publiceerAanwezigheid();
        }
      }
    }
  }

  void onScanEnd(const NimBLEScanResults& results, int reason) override {
    scanActief = false;
  }
};

void setupPresence() {
  Serial.println("Aanwezigheidsdetectie starten...");

  bleScan = NimBLEDevice::getScan();
  bleScan->setScanCallbacks(new ScanCallbacks(), false);
  bleScan->setInterval(BLE_SCAN_INTERVAL);
  bleScan->setWindow(BLE_SCAN_WINDOW);
  bleScan->setActiveScan(true);

  for (int i = 0; i < AANTAL_BLE_APPARATEN; i++) {
    apparaatStatus[i].laatstGezien = 0;
    apparaatStatus[i].rssi = 0;
    apparaatStatus[i].aanwezig = false;
  }

  Serial.printf("Tracking: %d apparaten\n", AANTAL_BLE_APPARATEN);
  for (int i = 0; i < AANTAL_BLE_APPARATEN; i++) {
    if (strlen(bleApparaten[i].serviceUUID) > 0) {
      Serial.printf("  - %s (UUID: %s)\n", bleApparaten[i].naam, bleApparaten[i].serviceUUID);
    } else {
      Serial.printf("  - %s (MAC: %s)\n", bleApparaten[i].naam, bleApparaten[i].macAdres);
    }
  }
}

void startScan() {
  if (scanActief) return;
  if (WiFi.status() != WL_CONNECTED) return;  // WiFi prioriteit

  scanActief = true;
  bleScan->start(SCAN_DUUR_MS / 1000, false);  // niet-blokkerend
}

bool iemandThuis() {
  unsigned long nu = millis();
  for (int i = 0; i < AANTAL_BLE_APPARATEN; i++) {
    if (apparaatStatus[i].aanwezig) {
      // Check timeout
      if (apparaatStatus[i].laatstGezien > 0 &&
          (nu - apparaatStatus[i].laatstGezien) > PRESENCE_TIMEOUT_MS) {
        apparaatStatus[i].aanwezig = false;
        Serial.printf("Aanwezigheid: %s is vertrokken (timeout)\n", bleApparaten[i].naam);
        publiceerAanwezigheid();
      }
    }
    if (apparaatStatus[i].aanwezig) return true;
  }
  return false;
}

void checkAanwezigheid() {
  unsigned long nu = millis();
  bool nuIemandThuis = iemandThuis();
  bool nuDonker = isDonker();

  // Edge: iemand komt thuis + het is donker → lampen aan
  if (nuIemandThuis && !iemandWasThuis && nuDonker) {
    // Check handmatige override
    if (handmatigeOverride == 0 || (nu - handmatigeOverride) > OVERRIDE_TIMEOUT_MS) {
      Serial.println("Auto-aan: iemand thuisgekomen en het is donker!");
      if (activeLamp < 0) {
        activeLamp = 0;
        helderheid = 100;
        stuurHelderheid(activeLamp, helderheid);
      }
    } else {
      Serial.println("Auto-aan overgeslagen (handmatige override actief).");
    }
  }

  // Edge: het wordt donker terwijl iemand thuis is → lampen aan
  if (nuIemandThuis && nuDonker && activeLamp < 0) {
    if (handmatigeOverride == 0 || (nu - handmatigeOverride) > OVERRIDE_TIMEOUT_MS) {
      Serial.println("Auto-aan: het wordt donker en iemand is thuis!");
      activeLamp = 0;
      helderheid = 100;
      stuurHelderheid(activeLamp, helderheid);
    }
  }

  // Edge: iedereen weg → lampen uit (ongeacht licht/donker)
  if (!nuIemandThuis && iemandWasThuis) {
    Serial.println("Auto-uit: iedereen is vertrokken.");
    allesUit();
    handmatigeOverride = 0;  // reset override bij vertrek
  }

  iemandWasThuis = nuIemandThuis;
}

#endif // FEATURE_PRESENCE

// ═══════════════════════════════════════════════════════════════════
//  Gedeelde commando-parser (MQTT + BLE)
// ═══════════════════════════════════════════════════════════════════

void verwerkCommando(String bericht) {
  laatsteActiviteit = millis();

  if (bericht == "alles_uit") {
    allesUit();
    handmatigeOverride = millis();
  }
  else if (bericht.startsWith("lamp") && bericht.length() >= 6) {
    int idx = bericht.charAt(4) - '1';
    if (idx >= 0 && idx < AANTAL_LAMPEN) {
      if (bericht.indexOf("_on") > 0) {
        activeLamp = idx;
        stuurAanUit(idx, true);
      } else if (bericht.indexOf("_off") > 0) {
        stuurAanUit(idx, false);
        if (activeLamp == idx) activeLamp = -1;
      } else if (bericht.indexOf("_dim_") > 0) {
        int pct = bericht.substring(bericht.lastIndexOf('_') + 1).toInt();
        if (pct >= 0 && pct <= 100) {
          activeLamp = idx;
          helderheid = pct;
          stuurHelderheid(idx, pct);
        }
      }
    }
  }
  #if FEATURE_PRESENCE
  else if (bericht == "presence_on") {
    Serial.println("Aanwezigheidsdetectie ingeschakeld via commando.");
  }
  else if (bericht == "presence_off") {
    Serial.println("Aanwezigheidsdetectie uitgeschakeld via commando.");
  }
  #endif
}

// ═══════════════════════════════════════════════════════════════════
//  MQTT (Home Assistant)
// ═══════════════════════════════════════════════════════════════════

void mqttVerbind() {
  if (!FEATURE_MQTT) return;
  if (mqtt.connected()) return;
  if (strlen(MQTT_SERVER) == 0) return;

  Serial.printf("MQTT verbinden met %s...\n", MQTT_SERVER);
  String clientId = "lamp-ctrl-" + String(WiFi.macAddress());

  bool ok;
  if (strlen(MQTT_USER) > 0) {
    ok = mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS,
                      MQTT_TOPIC_BASE "/online", MQTT_QOS_LWT, true, "offline");
  } else {
    ok = mqtt.connect(clientId.c_str(), "", "",
                      MQTT_TOPIC_BASE "/online", MQTT_QOS_LWT, true, "offline");
  }

  if (ok) {
    Serial.println("MQTT verbonden!");
    mqtt.publish(MQTT_TOPIC_BASE "/online", "online", true);
    mqtt.subscribe(MQTT_TOPIC_BASE "/cmd", MQTT_QOS_CMD);
    publiceerStatus();
    publiceerAanwezigheid();
    publiceerDonker();
  } else {
    Serial.printf("MQTT mislukt, rc=%d\n", mqtt.state());
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String bericht = "";
  for (unsigned int i = 0; i < length; i++) bericht += (char)payload[i];

  Serial.printf("MQTT ontvangen [%s]: %s\n", topic, bericht.c_str());
  verwerkCommando(bericht);
}

void publiceerStatus() {
  if (!FEATURE_MQTT || !mqtt.connected()) return;

  StaticJsonDocument<256> doc;
  doc["actief"] = activeLamp;
  doc["helderheid"] = helderheid;

  JsonArray arr = doc.createNestedArray("lampen");
  for (int i = 0; i < AANTAL_LAMPEN; i++) {
    JsonObject lamp = arr.createNestedObject();
    lamp["naam"] = lampen[i].naam;
    lamp["aan"] = lampAan[i];
  }

  char buf[256];
  serializeJson(doc, buf, sizeof(buf));
  mqtt.publish(MQTT_TOPIC_BASE "/status", buf, true);

  #if FEATURE_BLE
    updateBLEStatus();
  #endif
}

void publiceerAanwezigheid() {
  #if FEATURE_PRESENCE
    if (!FEATURE_MQTT || !mqtt.connected()) return;

    StaticJsonDocument<256> doc;
    doc["iemand_thuis"] = iemandThuis();

    JsonArray arr = doc.createNestedArray("apparaten");
    for (int i = 0; i < AANTAL_BLE_APPARATEN; i++) {
      JsonObject a = arr.createNestedObject();
      a["naam"] = bleApparaten[i].naam;
      a["aanwezig"] = apparaatStatus[i].aanwezig;
      a["rssi"] = apparaatStatus[i].rssi;
    }

    char buf[256];
    serializeJson(doc, buf, sizeof(buf));
    mqtt.publish(MQTT_TOPIC_BASE "/aanwezigheid", buf, true);
  #endif
}

void publiceerDonker() {
  #if FEATURE_ZONBEREKENING
    if (!FEATURE_MQTT || !mqtt.connected()) return;
    mqtt.publish(MQTT_TOPIC_BASE "/donker", hetIsDonker ? "true" : "false", true);
  #endif
}

// ═══════════════════════════════════════════════════════════════════
//  WiFi verbinding (via WiFi Manager)
// ═══════════════════════════════════════════════════════════════════

void verbindWiFi() {
  wifiManager.setConfigPortalTimeout(120);
  wifiManager.setAPCallback([](WiFiManager* mgr) {
    Serial.println("WiFi niet geconfigureerd.");
    Serial.println("Verbind met 'LampController-Setup' op je telefoon.");
    knipperLED(KLEUR_WIFI, 2, 300);
  });

  toonKleur(KLEUR_WIFI);

  if (!wifiManager.autoConnect("LampController-Setup")) {
    Serial.println("WiFi configuratie mislukt, herstart...");
    delay(3000);
    ESP.restart();
  }

  Serial.printf("Verbonden! IP: %s\n", WiFi.localIP().toString().c_str());
  toonUit();
}

// ═══════════════════════════════════════════════════════════════════
//  OTA updates
// ═══════════════════════════════════════════════════════════════════

void setupOTA() {
  if (!FEATURE_OTA) return;

  ArduinoOTA.setHostname(DEVICE_ID);
  ArduinoOTA.onStart([]() {
    Serial.println("OTA update gestart...");
    #if FEATURE_PRESENCE
      if (bleScan && bleScan->isScanning()) bleScan->stop();
    #endif
    knipperLED(KLEUR_WIFI, 3, 100);
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

// ═══════════════════════════════════════════════════════════════════
//  Setup
// ═══════════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.printf("\n=== %s v%s -Frank Geujen ===\n", FIRMWARE_NAAM, FIRMWARE_VERSIE);
  Serial.println("Hardware: M5Stack Atom Lite");

  // RGB LED initialiseren
  FastLED.addLeds<SK6812, RGB_LED_PIN, GRB>(led, 1);
  FastLED.setBrightness(RGB_HELDERHEID);
  toonUit();

  // Knop instellen (G39 = input only, Atom Lite heeft externe pull-up)
  pinMode(KNOP_PIN, INPUT);

  // Opstart animatie: regenboog sweep
  for (int hue = 0; hue < 256; hue += 8) {
    led[0] = CHSV(hue, 255, 200);
    FastLED.show();
    delay(15);
  }
  toonUit();

  // WiFi (via WiFi Manager portaal) — VOOR BLE! (D-005)
  verbindWiFi();

  // NTP tijdsynchronisatie
  syncNTP();

  // OTA updates
  setupOTA();

  // MQTT
  if (FEATURE_MQTT) {
    mqtt.setServer(MQTT_SERVER, MQTT_PORT);
    mqtt.setCallback(mqttCallback);
    mqttVerbind();
  }

  // BLE starten (NA WiFi verbinding — D-005)
  #if FEATURE_BLE
    setupBLE();
  #endif

  #if FEATURE_PRESENCE
    setupPresence();
  #endif

  laatsteActiviteit = millis();

  // Geheugen check
  Serial.printf("Vrij geheugen: %d bytes\n", ESP.getFreeHeap());

  Serial.println("Klaar!");
  Serial.println("  Kort drukken   = volgende lamp");
  Serial.println("  Lang drukken   = dimmen (omlaag/omhoog)");
  Serial.println("  Dubbel drukken = alles uit");
  if (FEATURE_OTA)     Serial.println("  OTA updates    = aan");
  if (FEATURE_MQTT)    Serial.printf( "  MQTT           = %s\n", MQTT_SERVER);
  if (FEATURE_BLE)     Serial.printf( "  BLE            = %s\n", BLE_DEVICE_NAAM);
  if (FEATURE_PRESENCE) Serial.printf("  Aanwezigheid   = %d apparaten\n", AANTAL_BLE_APPARATEN);
  if (FEATURE_ZONBEREKENING) {
    Serial.printf("  Zon            = lat %.2f, lon %.2f\n", LOCATIE_LAT, LOCATIE_LON);
    Serial.printf("  Donker         = %s\n", hetIsDonker ? "ja" : "nee");
  }

  // Groene flash = klaar
  knipperLED(CRGB(0, 255, 0), 2, 150);
  updateLED();
}

// ═══════════════════════════════════════════════════════════════════
//  Loop
// ═══════════════════════════════════════════════════════════════════

void loop() {
  unsigned long nu = millis();

  // OTA afhandelen
  if (FEATURE_OTA) ArduinoOTA.handle();

  // MQTT afhandelen
  if (FEATURE_MQTT) {
    if (!mqtt.connected()) mqttVerbind();
    mqtt.loop();
  }

  // WiFi check
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi weg, herverbinden...");
    toonKleur(KLEUR_WIFI);
    WiFi.reconnect();
    delay(5000);
    if (WiFi.status() != WL_CONNECTED) {
      verbindWiFi();
    }
    toonUit();
    updateLED();
  }

  // NTP hersynch (elke uur)
  if (nu - laatsteNtpSync >= NTP_SYNC_INTERVAL_MS) {
    syncNTP();
    laatsteNtpSync = nu;
  }

  // Zonberekening herberekenen (elke uur)
  #if FEATURE_ZONBEREKENING
    if (nu - laatsteZonCheck >= ZON_CHECK_INTERVAL_MS) {
      bool wasDonker = hetIsDonker;
      berekenZonTijden();
      isDonker();
      laatsteZonCheck = nu;

      if (hetIsDonker != wasDonker) {
        Serial.printf("Donker status gewijzigd: %s\n", hetIsDonker ? "donker" : "licht");
        publiceerDonker();
      }
    }
  #endif

  // BLE aanwezigheid scan (burst elke 30s)
  #if FEATURE_PRESENCE
    if (nu - laatsteScan >= SCAN_INTERVAL_MS) {
      startScan();
      laatsteScan = nu;
    }
    checkAanwezigheid();
  #endif

  // ── Knop afhandeling ──────────────────────────────────────────
  bool ingedrukt = (digitalRead(KNOP_PIN) == LOW);

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

  // Geen tweede druk gekomen → korte druk
  if (wachtOpDubbel && (nu - knopLosTijd) >= DUBBEL_WINDOW) {
    actieKort();
    wachtOpDubbel = false;
  }

  wasIngedrukt = ingedrukt;
  delay(10);
}

// ═══════════════════════════════════════════════════════════════════
//  Hardware: M5Stack Atom Lite (ESP32-PICO-D4)
//  ──────────────────────────────────────────────
//  G39  ── ingebouwde drukknop (actief laag, externe pull-up)
//  G27  ── ingebouwde SK6812 RGB LED (NeoPixel-compatible)
//  G12  ── ingebouwde IR LED (gereserveerd)
//  G26  ── Grove SDA (gereserveerd)
//  G32  ── Grove SCL (gereserveerd)
//
//  Voeding: USB-C (5V)
// ═══════════════════════════════════════════════════════════════════
