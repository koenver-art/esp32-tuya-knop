# Handleiding — ESP32 Atom Lite Lamp Controller

Stap-voor-stap handleiding om je M5Stack Atom Lite te configureren als lamp controller voor Action LSC Smart Connect lampen.

**Wat je nodig hebt:**
- M5Stack Atom Lite + Atomic Battery Base (200mAh) + USB-C kabel
- Computer (Mac of Windows)
- Smartphone (iPhone of Android)
- Action LSC Smart Connect lamp(en), aangesloten en werkend
- WiFi netwerk (lamp en Atom Lite moeten op hetzelfde netwerk)

---

## Stap 1 — Smart Life app installeren en lampen toevoegen

> **Belangrijk:** Gebruik de **Smart Life** app, NIET de LSC Smart Connect app. De LSC app kan niet gekoppeld worden aan het Tuya platform.

1. Download **Smart Life** uit de App Store (iPhone) of Play Store (Android)
2. Maak een account aan
   - Kies als land/regio: **Nederland**
   - Onthoud dit — je hebt het later nodig
3. Voeg je lamp(en) toe in de Smart Life app:
   - Tik op **+** rechtsboven
   - Kies **Verlichting → Lichtbron (WiFi)**
   - Volg de instructies (lamp knippert = koppelmodus)
4. Test of je de lamp kunt aan/uit zetten via de app
5. Check je regio: ga naar **Ik → Instellingen → Account en beveiliging → Regio**
   - Noteer wat er staat (bijv. "Europe" of "Netherlands")

---

## Stap 2 — Tuya IoT Platform account aanmaken

1. Ga naar **https://iot.tuya.com**
2. Klik **Register** / **Sign Up**
3. Vul in: email, wachtwoord, verificatiecode
4. Bij "Account Type" kun je **"Skip this step"** kiezen
5. Log in na registratie

---

## Stap 3 — Cloud Project aanmaken

1. Klik links op **Cloud → Development**
2. Klik **"Create Cloud Project"**
3. Vul in:
   - **Project Name**: bijv. `MijnLampen`
   - **Description**: mag leeg
   - **Industry**: kies **Smart Home**
   - **Development Method**: kies **Smart Home**
   - **Data Center**: kies **Central Europe**
     > Dit MOET overeenkomen met je Smart Life app regio. Nederland = Central Europe. Als dit niet klopt krijg je later de fout "data center inconsistency".
4. Klik **Create**

---

## Stap 4 — API Services toevoegen

Na het aanmaken kom je op een configuratie pagina. Zorg dat deze services allemaal aangevinkt/geactiveerd zijn:

- **IoT Core** (verplicht)
- **Authorization Token Management** (verplicht — zonder deze krijg je "permission deny" fout)
- **Smart Home Basic Service**

Klik **Authorize** om te bevestigen.

> **Let op:** IoT Core is een proefabonnement dat na ~1 maand verloopt. Verleng via **Cloud → My Services → IoT Core → Extend Trial Period**.

---

## Stap 5 — API sleutels opzoeken

1. Ga naar **Cloud → Development**
2. Klik op je project naam
3. Klik op het tabblad **Overview**
4. Je ziet:
   - **Access ID / Client ID** → dit is je **API Key**
   - **Access Secret / Client Secret** → dit is je **API Secret**
5. Kopieer beide en bewaar ze

---

## Stap 6 — Smart Life app koppelen aan het platform

Dit is de lastigste stap. Lees alles eerst voordat je begint.

1. Klik in je project op het tabblad **Devices**
2. Klik **"Link Tuya App Account"**
3. Klik **"Add App Account"**
4. Er verschijnt een **QR code**

**Nu snel handelen — de QR code verloopt binnen 1-2 minuten!**

5. Open de **Smart Life** app op je telefoon
6. Ga naar het tabblad **Ik** (rechtsonder)
7. Tik op het **QR scanner icoontje** rechtsboven
8. Scan de QR code op je scherm
9. Bevestig de koppeling op je telefoon

Als het gelukt is, verschijnen je lampen in het **Devices** tabblad.

**Veelvoorkomende problemen:**

| Probleem | Oplossing |
|----------|-----------|
| QR code verlopen | Klik opnieuw op "Add App Account" voor een nieuwe code. Heb Smart Life app al open voordat je de code genereert. |
| "Data center inconsistency" | Je Cloud Project data center komt niet overeen met je Smart Life app regio. Maak een nieuw project met de juiste regio. |
| Geen apparaten zichtbaar | Check of je de Smart Life app gebruikt (niet LSC app). Probeer opnieuw te koppelen. |

---

## Stap 7 — Python en tinytuya installeren

**Mac:**
Open Terminal en typ:
```
git
```
Als macOS vraagt om Xcode Command Line Tools te installeren → klik **Install**.

Dan:
```
pip3 install tinytuya
```

**Windows:**
1. Download Python van https://www.python.org/downloads/
2. Installeer met **"Add Python to PATH"** aangevinkt
3. Open Command Prompt en typ:
```
pip install tinytuya
```

---

## Stap 8 — Local key ophalen met tinytuya

1. Open Terminal / Command Prompt
2. Typ:
```
python -m tinytuya wizard
```
(of `python3 -m tinytuya wizard` op Mac)

3. Vul in wanneer gevraagd:
   - **Enter API Key**: plak je **Access ID / Client ID** uit stap 5
   - **Enter API Secret**: plak je **Access Secret / Client Secret** uit stap 5
   - **Enter any Device ID**: ga naar Tuya IoT Platform → Devices tab → kopieer het Device ID van je lamp
   - **Enter Your Region**: typ `eu` (voor Central Europe)

4. Je krijgt een lijst met al je apparaten. Noteer per lamp:
   - **name** — naam van de lamp
   - **id** — Device ID
   - **ip** — IP adres (bijv. `192.168.1.241`)
   - **key** — Local Key (16 tekens, bijv. `&7tO0>hrmG~1}]]F`)
   - **ver** — versie (waarschijnlijk `3.5`)

---

## Stap 9 — Test de lamp via Python

Controleer of de info klopt:

```
python
```

Typ dan (één regel per keer):
```python
import tinytuya
d = tinytuya.BulbDevice('JOUW_DEVICE_ID', 'JOUW_IP', 'JOUW_LOCAL_KEY')
d.set_version(3.5)
d.turn_on()
```

Gaat de lamp aan? Dan klopt alles. Typ `d.turn_off()` om uit te zetten en `exit()` om Python af te sluiten.

---

## Stap 10 — BLE apparaten vinden (voor aanwezigheidsdetectie)

De controller kan detecteren wie er thuis is via Bluetooth. Je hebt per persoon een BLE apparaat nodig dat je altijd bij je hebt (AirPods, telefoon, etc.).

### Voor Apple apparaten (AirPods, iPhone):

> **Tip:** AirPods werken beter dan iPhone — iPhones roteren hun Bluetooth adres voor privacy.

1. Download **nRF Connect** uit de App Store
2. Open de app en tik op **Scan**
3. Zoek je AirPods in de lijst (doe ze in je oren zodat ze zenden)
4. Tik op je AirPods
5. Zoek de **Service UUID** — een lang getal met streepjes, bijv:
   `87290102-3c51-43b1-a1a9-11b9dc38478b`
6. Noteer deze UUID

### Voor Android apparaten:

1. Ga naar **Instellingen → Over telefoon → Status** → noteer het **Bluetooth adres**
   (formaat: `AA:BB:CC:DD:EE:FF`)
2. Of gebruik nRF Connect op een andere telefoon om het adres te vinden

---

## Stap 11 — Arduino IDE installeren en instellen

1. Download **Arduino IDE 2** van https://www.arduino.cc/en/software
2. Installeer en open Arduino IDE

**Board package installeren:**
3. **Tools → Board → Boards Manager**
4. Zoek **"esp32"** (door Espressif Systems) → klik **Install**

**Libraries installeren:**
5. **Sketch → Include Library → Manage Libraries**
6. Zoek en installeer één voor één:
   - **FastLED**
   - **NimBLE-Arduino** (door h2zero)
   - **SolarCalculator**
   - **WiFiManager** (door tzapu)
   - **PubSubClient**
   - **ArduinoJson**

---

## Stap 12 — Project downloaden en openen

Open Terminal:
```
cd ~/Documents
git clone https://github.com/koenver-art/esp32-tuya-knop.git
open esp32-tuya-knop/ESP32_Tuya_Knop/ESP32_Tuya_Knop.ino
```

Je zou **5 tabs** moeten zien bovenaan in Arduino IDE:
- `ESP32_Tuya_Knop.ino`
- `config.h`
- `EspTuya.h`
- `secrets.example.h`
- `secrets.h`

Als er tabs missen: **Sketch → Add File** en voeg ze handmatig toe.

---

## Stap 13 — secrets.h invullen met jouw gegevens

Klik op de tab **secrets.h** en vervang de inhoud met je eigen gegevens:

```cpp
#ifndef SECRETS_H
#define SECRETS_H

#include "config.h"

// ── Lampen ──────────────────────────────────────────────────────
// Vul per lamp in: naam, IP-adres, local key (uit stap 8), versie
// Versie is bijna altijd 3 (wordt intern als 3.5 behandeld)
//
// Tip: geef je lampen een vast IP in je router (DHCP reservering)

Lamp lampen[] = {
  { "Hoofdlicht", "192.168.1.xxx", "jouw_local_key_hier", 3 },
  // Meer lampen? Voeg regels toe:
  // { "Sfeerlamp", "192.168.1.xxx", "andere_key_hier", 3 },
};

// ── BLE apparaten ───────────────────────────────────────────────
// Apple (AirPods): vul de service UUID in, laat MAC leeg
// Android: laat UUID leeg, vul MAC adres in

BleApparaat bleApparaten[] = {
  { "Koen (AirPods)", "jouw-uuid-hier", "" },
  { "Lotte",          "", "AA:BB:CC:DD:EE:FF" },
};

// ── Locatie ─────────────────────────────────────────────────────
// Voor zonsopkomst/ondergang berekening
// Opzoeken: Google Maps → rechtermuisklik → coördinaten kopiëren
#define LOCATIE_LAT   51.xxxx    // breedtegraad
#define LOCATIE_LON    5.xxxx    // lengtegraad

// Tijdzone Nederland (hoef je niet aan te passen)
#define TIJDZONE  "CET-1CEST,M3.5.0/02,M10.5.0/03"

// ── MQTT (optioneel, voor Home Assistant) ───────────────────────
// Laat leeg als je geen Home Assistant hebt
#define MQTT_SERVER      ""
#define MQTT_PORT        1883
#define MQTT_USER        ""
#define MQTT_PASS        ""

#endif
```

Sla op met **⌘S** (Mac) of **Ctrl+S** (Windows).

---

## Stap 14 — Flashen naar de Atom Lite

1. Sluit de M5Stack Atom Lite aan met USB-C kabel
2. In Arduino IDE:
   - **Tools → Board → esp32 → m5atom** (of M5Stack-ATOM)
   - **Tools → Port** → selecteer de USB poort die verschijnt
3. Klik **Sketch → Upload** (of het pijltje → rechtsboven)
4. Wacht tot je ziet: `hard resetting via rts pin...`
5. Klaar! De firmware staat op de Atom Lite.

---

## Stap 15 — WiFi configureren

1. Na het flashen knippert de LED **lichtblauw** (cyaan)
2. Pak je telefoon → ga naar WiFi instellingen
3. Verbind met het netwerk **"LampController-Setup"**
4. Er opent automatisch een configuratiepagina
5. Kies je **thuis WiFi netwerk** en vul het wachtwoord in
6. De Atom Lite herstart en verbindt met je WiFi

> Dit hoef je maar één keer te doen. Na een stroomonderbreking verbindt hij automatisch opnieuw.

---

## Stap 16 — Testen

### Knop (bovenop de Atom Lite):
| Actie | Wat het doet |
|-------|-------------|
| **1x kort drukken** | Lamp aan/uit |
| **2x snel drukken** | Alles uit |
| **3x snel drukken** | Volgende kleur (wit → warm → koud → rood → groen → blauw → paars → oranje) |
| **Lang indrukken** | Dimmen (20% stappen, wisselt richting) |

### Webinterface:
1. Open **Serial Monitor** in Arduino IDE (Tools → Serial Monitor, 115200 baud)
2. Kijk naar het IP adres (bijv. `Verbonden! IP: 192.168.1.109`)
3. Open in je browser: `http://192.168.1.109:8080/`
4. Je krijgt een bedieningspagina met knoppen, sliders, kleuren, scenes en timer

### BLE telefoonbediening:
1. Download **nRF Connect** of **LightBlue** app
2. Scan voor **"LampController"**
3. Verbind en schrijf commando's als UTF-8 tekst:
   - `aan` / `uit`
   - `dim 50` (0-100)
   - `rood`, `blauw`, `groen`, `warm`, `koud`, `wit`
   - `lamp1_on`, `lamp1_dim_30`, `lamp1_off`

### API (voor Siri Shortcuts):
Maak een iOS Shortcut met "Get Contents of URL":
- `http://192.168.1.109:8080/api/aan` → lamp aan
- `http://192.168.1.109:8080/api/uit` → lamp uit
- `http://192.168.1.109:8080/api/dim/50` → dim 50%
- `http://192.168.1.109:8080/api/kleur/rood` → rood
- `http://192.168.1.109:8080/api/scene/film` → film scene
- `http://192.168.1.109:8080/api/timer/30` → uit over 30 min

---

## Problemen oplossen

| Probleem | Oplossing |
|----------|-----------|
| `config.h not found` bij compileren | Alle bestanden moeten in de map `ESP32_Tuya_Knop/` staan. Gebruik **Sketch → Add File** om ontbrekende bestanden toe te voegen. |
| `EspTuya.h not found` | Zelfde als hierboven — Add File. |
| Board niet gevonden | **Tools → Board → Boards Manager** → installeer "esp32" door Espressif. |
| Geen poort zichtbaar | USB kabel moet data ondersteunen (niet alleen opladen). Probeer een andere kabel. Op Mac: herstart na eerste aansluiting. |
| Lamp reageert niet na druk op knop | Check Serial Monitor (115200 baud). Check of IP en local key kloppen. Test met tinytuya (stap 9). |
| "Session negotiatie mislukt" | Local key is verkeerd, of lamp is niet bereikbaar op het netwerk. |
| BLE "LampController" niet zichtbaar | Check Serial Monitor voor "BLE klaar". Installeer NimBLE-Arduino library. |
| WiFi portal komt niet | Reset de Atom Lite (USB kabel eruit en erin). |
| Webinterface niet bereikbaar | Check of je op hetzelfde WiFi netwerk zit. Probeer poort 8080: `http://IP:8080/` |
| "Data center inconsistency" bij Tuya | Maak nieuw Cloud Project met juiste Data Center (Central Europe voor Nederland). |
| "Permission deny" bij tinytuya | Voeg "Authorization Token Management" API service toe in je Cloud Project. |
| QR code verlopen | Genereer opnieuw. Heb Smart Life app al open op de QR scanner voordat je klikt. |
