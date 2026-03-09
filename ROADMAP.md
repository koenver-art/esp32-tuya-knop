# Roadmap — Woonkamer Lamp Controller

## Gedaan

- [x] **v3.0** Base port naar M5Stack Atom Lite
- [x] Knop: aan/uit, dubbel=alles uit, lang=dimmen, 3x=kleur cyclus
- [x] RGB LED status per lamp
- [x] BLE GATT telefoonbediening (nRF Connect / LightBlue)
- [x] BLE commando's: aan, uit, dim, kleuren, warm/koud, wit
- [x] BLE aanwezigheidsdetectie (AirPods UUID matching)
- [x] Zonberekening (burgerlijke schemering)
- [x] Tuya v3.5 protocol (AES-GCM, session key negotiation)
- [x] WiFi Manager (captive portal)
- [x] OTA updates
- [x] MQTT basis

## Fase 5 — Webinterface

- [ ] Ingebouwde webserver op ESP32 (`http://<ip>`)
- [ ] Knoppen aan/uit per lamp
- [ ] Dimmer slider
- [ ] Kleurkiezer (color wheel)
- [ ] Kleurtemperatuur slider (warm ↔ koud)
- [ ] Scene knoppen (film, lezen, feest, etc.)
- [ ] Status weergave (welke lamp aan, helderheid, kleur)
- [ ] Responsive design (werkt op telefoon en desktop)

## Fase 6 — Scenes & Automatisering

- [ ] Voorgedefinieerde scenes: film, lezen, feest, ontspannen, nachtlamp
- [ ] Scene via knop (4x drukken = volgende scene)
- [ ] Wake-up light (langzaam opkomen over 15-30 min)
- [ ] Slaap-timer ("uit over X minuten")
- [ ] Kleur cyclus modus (feestverlichting, langzaam roterend)

## Fase 7 — Home Assistant Integratie

- [ ] MQTT auto-discovery (lamp verschijnt automatisch in HA)
- [ ] HA entiteiten: light met brightness, color_temp, rgb_color
- [ ] Aanwezigheidssensor als binary_sensor in HA
- [ ] Donker/licht sensor als binary_sensor in HA
- [ ] HA automations triggers via MQTT

## Fase 8 — iOS / Siri Integratie

- [ ] iOS Shortcuts via BLE characteristic
- [ ] "Hé Siri, lamp aan/uit/rood/dim"
- [ ] Shortcut widget op iPhone homescreen

## Fase 9 — Multi-room & Mesh

- [ ] Meerdere ESP32's (per kamer)
- [ ] Synchronisatie via MQTT
- [ ] Groepen (bijv. "beneden alles uit")
- [ ] Centraal dashboard (webinterface of HA)

## Fase 10 — Extra Hardware

- [ ] IR blaster (G12) voor TV/airco bediening
- [ ] Grove sensoren (G26/G32): temperatuur, luchtvochtigheid
- [ ] PIR bewegingsdetectie via Grove
- [ ] Meerdere lampen per controller (tot 3 in config)

## Ideeen (ongesorteerd)

- Alarm functie (knipperende kleur bij melding)
- Spotify Now Playing kleur sync
- Energieverbruik tracking via Tuya DP's
- Firmware update via webinterface (OTA upload pagina)
- QR code op webpagina voor BLE verbinding
- Configuratie via webinterface (i.p.v. secrets.h flashen)
