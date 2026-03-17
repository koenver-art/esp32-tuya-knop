# Changelog

Alle noemenswaardige wijzigingen aan dit project staan hier.

Het formaat is gebaseerd op [Keep a Changelog](https://keepachangelog.com/nl/1.0.0/)
en dit project gebruikt [Semantic Versioning](https://semver.org/lang/v2.0.0/).

## [3.1.0] - 2026-03-15

### Toegevoegd
- Deep sleep na 60 seconden inactiviteit (batterij besparing)
- Snelle sleep: 5 seconden na "alles uit" → deep sleep
- Wake-up via ingebouwde knop (ext0 wakeup op G39)
- Nette shutdown: BLE deinit + MQTT LWT offline voor sleep
- Ondersteuning voor Atomic Battery Base (200mAh)

### Gewijzigd
- Hardware beschrijving bijgewerkt voor Battery Base
- Firmware versie naar 3.1.0

## [3.0.0] - 2026-03-08

### Toegevoegd
- Complete herschrijving voor **M5Stack Atom Lite** hardware
- BLE GATT server (NimBLE) met 5 characteristics voor telefoonbediening
- BLE aanwezigheidsdetectie: UUID matching (Apple) + MAC matching (Android)
- Automatische lampen bij thuiskomst als het donker is
- Zonberekening via SolarCalculator (burgerlijke schemering)
- Webinterface op poort 8080 met scenes, timer, kleurkiezer
- HTTP API endpoints voor Siri Shortcuts integratie
- RGB LED feedback via ingebouwde SK6812 (kleur per lamp)
- Tuya v3.5 protocol ondersteuning (AES-GCM, session key negotiation)
- EspTuya.h: header-only Tuya LAN protocol library
- Kleurmodus cyclus via triple-press (8 kleuren)
- BLE commando queue (voorkomt stack overflow op NimBLE task)

### Verwijderd
- Externe drukknop, 3 losse LEDs, batterij ADC (nu ingebouwd in Atom Lite)
- ESP32 DevKit V1 pin definities

### Gewijzigd
- Hardware platform: ESP32 DevKit V1 → M5Stack Atom Lite (ESP32-PICO-D4)
- LED feedback: 3 aparte LEDs → 1 RGB LED met kleurcodering
- Knop: externe GPIO4 → ingebouwde G39

## [2.1.0] - 2026-02-18

### Toegevoegd
- Configuratie gescheiden van broncode (`config.h`)
- Gevoelige gegevens geïsoleerd in `secrets.h` (niet in git)
- `secrets.example.h` als template, kopiëren en invullen
- MQTT Last Will & Testament (LWT)
- MQTT QoS niveaus per topic type
- Versienummer in firmware en serial output
- Architectuur documentatie in README
- CHANGELOG.md voor versiebeheer

### Gewijzigd
- Code beter georganiseerd: configuratie, secrets en logica gescheiden

## [2.0.0] - 2026-02-15

### Toegevoegd
- WiFi Manager, bij eerste boot WiFi configureren via telefoon
- OTA firmware updates via WiFi
- Deep sleep met wake-up via drukknop (ext0 op GPIO4)
- Batterij monitoring met LED waarschuwing
- MQTT integratie voor Home Assistant
- Bidirectioneel dimmen (omlaag en omhoog, afwisselend)
- 3 status LEDs voor lampstatus
- Foutafhandeling bij onbereikbare lampen (LED knippert 5x)
- Feature flags om onderdelen aan/uit te zetten via `#define`

### Gewijzigd
- Complete herschrijving van v1.0

## [1.0.0] - 2026-01-20

### Eerste versie
- Basisbesturing van 3 Action lampen via drukknop
- Tuya LAN protocol communicatie
- Kort drukken om door lampen te schakelen
- Dubbel drukken voor alles uit
- Lang drukken om te dimmen

---

*Koen Verhallen, 2026*
