# Roadmap — Woonkamer Lamp Controller

## Gedaan

- [x] **v1.0** Basisbesturing: kort/lang/dubbel drukken, Tuya LAN protocol
- [x] **v2.0** WiFi Manager, OTA updates, deep sleep, batterij monitoring, MQTT, status LEDs
- [x] **v2.1** Config/secrets scheiding, MQTT LWT, architectuur documentatie
- [x] **v3.0** M5Stack Atom Lite port: BLE GATT, aanwezigheidsdetectie, zonberekening, webinterface
- [x] **v3.1** Atomic Battery Base: deep sleep, ext0 wakeup, power management

## Fase 6 — Scenes & Automatisering

- [ ] Voorgedefinieerde scenes via knop (4x drukken = volgende scene)
- [ ] Wake-up light (langzaam opkomen over 15-30 min)
- [ ] Kleur cyclus modus (feestverlichting, langzaam roterend)

## Fase 7 — Home Assistant Integratie

- [ ] MQTT auto-discovery (lamp verschijnt automatisch in HA)
- [ ] HA entiteiten: light met brightness, color_temp, rgb_color
- [ ] Aanwezigheidssensor als binary_sensor in HA
- [ ] Donker/licht sensor als binary_sensor in HA

## Fase 8 — iOS / Siri Integratie

- [ ] iOS Shortcuts via BLE characteristic
- [ ] "Hé Siri, lamp aan/uit/rood/dim"
- [ ] Shortcut widget op iPhone homescreen

## Fase 9 — Multi-room & Mesh

- [ ] Meerdere ESP32's (per kamer)
- [ ] Synchronisatie via MQTT
- [ ] Groepen (bijv. "beneden alles uit")

## Fase 10 — Extra Hardware

- [ ] IR blaster (G12) voor TV/airco bediening
- [ ] Grove sensoren (G26/G32): temperatuur, luchtvochtigheid
- [ ] PIR bewegingsdetectie via Grove

## Ideeen (ongesorteerd)

- Alarm functie (knipperende kleur bij melding)
- Spotify Now Playing kleur sync
- Energieverbruik tracking via Tuya DP's
- Firmware update via webinterface (OTA upload pagina)
- QR code op webpagina voor BLE verbinding
- Configuratie via webinterface (i.p.v. secrets.h flashen)
- Power optimalisatie: light sleep tussen BLE scans
