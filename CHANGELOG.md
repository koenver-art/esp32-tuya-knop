# Changelog

Alle noemenswaardige wijzigingen aan dit project staan hier.

Het formaat is gebaseerd op [Keep a Changelog](https://keepachangelog.com/nl/1.0.0/)
en dit project gebruikt [Semantic Versioning](https://semver.org/lang/v2.0.0/).

## [2.1.0] - 2026-02-18

### Toegevoegd
- Configuratie gescheiden van broncode (`config.h`)
- Gevoelige gegevens geïsoleerd in `secrets.h` (niet in git)
- `secrets.example.h` als template, kopiëren en invullen
- MQTT Last Will & Testament (LWT), Home Assistant ziet meteen of de controller offline is
- MQTT QoS niveaus per topic type
- Versienummer in firmware en serial output
- Architectuur documentatie in README
- MQTT documentatie met voorbeeld payloads
- CHANGELOG.md voor versiebeheer

### Gewijzigd
- Code beter georganiseerd: configuratie, secrets en logica gescheiden
- MQTT reconnect logt nu het versienummer mee

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
- Knop detectie: kort, lang en dubbel drukken

## [1.0.0] - 2026-01-20

### Eerste versie
- Basisbesturing van 3 Action lampen via drukknop
- Tuya LAN protocol communicatie
- Kort drukken om door lampen te schakelen
- Dubbel drukken voor alles uit
- Lang drukken om te dimmen

---

*Koen Verhallen, 2026*
