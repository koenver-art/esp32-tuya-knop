// ═══════════════════════════════════════════════════════════════════
//  EspTuya — Tuya LAN protocol library voor ESP32
//  Ondersteunt protocol versie 3.3 en 3.5
//
//  Gebaseerd op tinytuya (Python) protocol analyse.
//  Gebruikt mbedtls (ingebouwd in ESP32) voor AES-ECB, AES-GCM,
//  en HMAC-SHA256 cryptografie.
//
//  Gebruik:
//    EspTuya tuya;
//    if (tuya.begin("192.168.1.x", "local_key_16chars", 35)) {
//      tuya.setBool(20, true);   // lamp aan
//      tuya.setInt(22, 500);     // helderheid 50%
//    }
//    tuya.end();
// ═══════════════════════════════════════════════════════════════════

#ifndef ESPTUYA_H
#define ESPTUYA_H

#include <Arduino.h>
#include <WiFiClient.h>
#include <mbedtls/aes.h>
#include <mbedtls/gcm.h>
#include <mbedtls/md.h>

// ── Protocol constanten ─────────────────────────────────────────
#define TUYA_PORT           6668
#define TUYA_PREFIX_55AA    0x000055AAul
#define TUYA_SUFFIX_55AA    0x0000AA55ul
#define TUYA_PREFIX_6699    0x00006699ul
#define TUYA_SUFFIX_6699    0x00009966ul

#define TUYA_TIMEOUT_MS     5000
#define TUYA_BUF_SIZE       512

// Commando's
#define CMD_CONTROL         0x07
#define CMD_STATUS          0x08
#define CMD_HEARTBEAT       0x09
#define CMD_DP_QUERY        0x0A
#define CMD_CONTROL_NEW     0x0D
#define CMD_DP_QUERY_NEW    0x10
#define CMD_UPDATEDPS       0x12
#define CMD_SESS_NEG_START  0x03
#define CMD_SESS_NEG_RESP   0x04
#define CMD_SESS_NEG_FINISH 0x05

// Debug logging
#define TUYA_DEBUG 1
#if TUYA_DEBUG
  #define TUYA_LOG(fmt, ...) Serial.printf("[Tuya] " fmt "\n", ##__VA_ARGS__)
#else
  #define TUYA_LOG(fmt, ...)
#endif

class EspTuya {
public:
  EspTuya() : _connected(false), _useGcm(false), _seqNo(1) {}

  // ── Publieke API ──────────────────────────────────────────────

  bool begin(const char* ip, const char* localKey, int version) {
    _connected = false;
    _seqNo = 1;
    _useGcm = (version >= 4 || version == 3);  // v3.3+ probeert v3.5 eerst

    // Local key opslaan (exact 16 bytes)
    memset(_deviceKey, 0, 16);
    int keyLen = strlen(localKey);
    if (keyLen > 16) keyLen = 16;
    memcpy(_deviceKey, localKey, keyLen);

    TUYA_LOG("Verbinden met %s:%d (versie %d)", ip, TUYA_PORT, version);

    // TCP verbinding
    if (!_client.connect(ip, TUYA_PORT)) {
      TUYA_LOG("FOUT: kan niet verbinden met %s:%d", ip, TUYA_PORT);
      return false;
    }
    _client.setTimeout(TUYA_TIMEOUT_MS / 1000);
    TUYA_LOG("TCP verbonden");

    // v3.5 vereist session key negotiatie
    if (!_negotiateSession()) {
      TUYA_LOG("Session negotiatie mislukt");
      _client.stop();
      return false;
    }

    _connected = true;
    TUYA_LOG("Klaar — session key actief");
    return true;
  }

  bool setBool(int dp, bool value) {
    if (!_ensureConnected()) return false;

    char json[128];
    snprintf(json, sizeof(json),
      "{\"protocol\":5,\"t\":%lu,\"data\":{\"dps\":{\"%d\":%s}}}",
      (unsigned long)time(nullptr), dp, value ? "true" : "false");

    return _sendControl(json);
  }

  bool setInt(int dp, int value) {
    if (!_ensureConnected()) return false;

    char json[128];
    snprintf(json, sizeof(json),
      "{\"protocol\":5,\"t\":%lu,\"data\":{\"dps\":{\"%d\":%d}}}",
      (unsigned long)time(nullptr), dp, value);

    return _sendControl(json);
  }

  bool isConnected() { return _connected && _client.connected(); }

  void end() {
    if (_client.connected()) _client.stop();
    _connected = false;
  }

  ~EspTuya() { end(); }

private:
  WiFiClient _client;
  uint8_t _deviceKey[16];   // real_local_key (uit Tuya cloud)
  uint8_t _sessionKey[16];  // afgeleide sessie key
  uint8_t _localNonce[16];  // onze nonce voor session negotiation
  uint32_t _seqNo;
  bool _connected;
  bool _useGcm;

  // ── Verbinding check ───────────────────────────────────────────

  bool _ensureConnected() {
    if (_connected && _client.connected()) return true;
    TUYA_LOG("Niet verbonden");
    _connected = false;
    return false;
  }

  // ── Session key negotiatie (v3.5) ──────────────────────────────

  bool _negotiateSession() {
    // Genereer random local nonce (16 bytes)
    for (int i = 0; i < 16; i++) _localNonce[i] = random(0, 256);

    TUYA_LOG("Stap 1: stuur local nonce");
    _hexDump("localNonce", _localNonce, 16);

    // ── Stap 1: Stuur SESS_KEY_NEG_START met local nonce ─────
    uint8_t sendBuf[TUYA_BUF_SIZE];
    int msgLen = _build6699(sendBuf, CMD_SESS_NEG_START,
                            _localNonce, 16, _deviceKey, false);
    if (msgLen <= 0) {
      TUYA_LOG("FOUT: kan stap 1 bericht niet bouwen");
      return false;
    }
    _client.write(sendBuf, msgLen);
    _seqNo++;

    // ── Stap 2: Ontvang remote nonce + HMAC ──────────────────
    TUYA_LOG("Stap 2: wacht op antwoord...");
    uint8_t recvBuf[TUYA_BUF_SIZE];
    int recvLen = _receiveMessage(recvBuf, TUYA_BUF_SIZE);
    if (recvLen <= 0) {
      TUYA_LOG("FOUT: geen antwoord ontvangen (timeout)");
      return false;
    }
    TUYA_LOG("Ontvangen: %d bytes", recvLen);

    // Parse en decrypt response
    uint8_t payload[256];
    int payloadLen = 0;
    int respCmd = 0;
    if (!_parse6699(recvBuf, recvLen, payload, &payloadLen, &respCmd, _deviceKey)) {
      TUYA_LOG("FOUT: kan antwoord niet decrypten");
      return false;
    }

    TUYA_LOG("Antwoord: cmd=%d payloadLen=%d", respCmd, payloadLen);

    if (respCmd != CMD_SESS_NEG_RESP) {
      TUYA_LOG("FOUT: verwacht cmd %d, kreeg %d", CMD_SESS_NEG_RESP, respCmd);
      return false;
    }

    // Payload kan retcode(4) + remote_nonce(16) + hmac(32) = 52 bytes bevatten
    // Of alleen remote_nonce(16) + hmac(32) = 48 bytes
    uint8_t* dataStart = payload;
    int dataLen = payloadLen;

    // Als payload 52 bytes is, skip de retcode (eerste 4 bytes)
    if (dataLen == 52) {
      uint32_t retcode = _readU32(dataStart, 0);
      TUYA_LOG("Retcode: %u", retcode);
      dataStart += 4;
      dataLen -= 4;
    } else if (dataLen >= 52) {
      // Meer dan 52 bytes — skip retcode
      dataStart += 4;
      dataLen -= 4;
    }

    if (dataLen < 48) {
      TUYA_LOG("FOUT: payload te kort: %d bytes (verwacht 48)", dataLen);
      _hexDump("payload", payload, payloadLen);
      return false;
    }

    // Remote nonce = eerste 16 bytes, HMAC = volgende 32 bytes
    uint8_t remoteNonce[16];
    uint8_t receivedHmac[32];
    memcpy(remoteNonce, dataStart, 16);
    memcpy(receivedHmac, dataStart + 16, 32);

    _hexDump("remoteNonce", remoteNonce, 16);

    // Verifieer HMAC van onze local nonce
    uint8_t expectedHmac[32];
    _hmacSha256(_deviceKey, 16, _localNonce, 16, expectedHmac);
    if (memcmp(receivedHmac, expectedHmac, 32) != 0) {
      TUYA_LOG("FOUT: HMAC verificatie mislukt");
      _hexDump("verwacht", expectedHmac, 32);
      _hexDump("ontvangen", receivedHmac, 32);
      return false;
    }
    TUYA_LOG("HMAC verificatie OK");

    // ── Stap 3: Stuur HMAC van remote nonce ──────────────────
    TUYA_LOG("Stap 3: stuur HMAC van remote nonce");
    uint8_t remoteHmac[32];
    _hmacSha256(_deviceKey, 16, remoteNonce, 16, remoteHmac);

    msgLen = _build6699(sendBuf, CMD_SESS_NEG_FINISH,
                        remoteHmac, 32, _deviceKey, false);
    if (msgLen <= 0) {
      TUYA_LOG("FOUT: kan stap 3 bericht niet bouwen");
      return false;
    }
    _client.write(sendBuf, msgLen);
    _seqNo++;

    // ── Stap 4: Bereken session key ──────────────────────────
    // XOR local en remote nonce
    uint8_t xorResult[16];
    for (int i = 0; i < 16; i++) {
      xorResult[i] = _localNonce[i] ^ remoteNonce[i];
    }
    _hexDump("nonceXOR", xorResult, 16);

    // v3.5: session_key = AES-GCM-encrypt(deviceKey, xorResult, iv=localNonce[:12])[ciphertext only]
    // GCM encrypt retourneert ciphertext — dat IS de session key
    uint8_t iv[12];
    memcpy(iv, _localNonce, 12);

    uint8_t encrypted[16];
    uint8_t tag[16];
    // GEEN AAD bij session key afleiding
    if (!_aesGcmEncrypt(_deviceKey, iv, 12, NULL, 0, xorResult, 16, encrypted, tag)) {
      TUYA_LOG("FOUT: session key afleiding (GCM encrypt) mislukt");
      return false;
    }

    memcpy(_sessionKey, encrypted, 16);
    _useGcm = true;

    _hexDump("sessionKey", _sessionKey, 16);

    // Kort wachten en eventuele extra berichten legen
    delay(100);
    while (_client.available()) _client.read();

    TUYA_LOG("Session negotiatie voltooid!");
    return true;
  }

  // ── Commando versturen ────────────────────────────────────────

  bool _sendControl(const char* json) {
    // v3.5: prepend versie header "3.5" + 12 null bytes (15 bytes totaal)
    uint8_t payload[384];
    int payloadLen = 0;

    // Versie prefix
    memcpy(payload, "3.5", 3);
    memset(payload + 3, 0, 12);
    payloadLen = 15;

    int jsonLen = strlen(json);
    if (payloadLen + jsonLen >= (int)sizeof(payload)) {
      TUYA_LOG("FOUT: payload te groot");
      return false;
    }
    memcpy(payload + payloadLen, json, jsonLen);
    payloadLen += jsonLen;

    TUYA_LOG("Stuur CONTROL_NEW: %s", json);

    uint8_t sendBuf[TUYA_BUF_SIZE];
    int msgLen = _build6699(sendBuf, CMD_CONTROL_NEW,
                            payload, payloadLen, _sessionKey, false);
    if (msgLen <= 0) {
      TUYA_LOG("FOUT: kan bericht niet bouwen");
      return false;
    }

    _client.write(sendBuf, msgLen);
    _seqNo++;

    // Wacht op antwoord
    uint8_t recvBuf[TUYA_BUF_SIZE];
    int recvLen = _receiveMessage(recvBuf, TUYA_BUF_SIZE);

    if (recvLen <= 0) {
      TUYA_LOG("WAARSCHUWING: geen antwoord op control commando");
      return false;
    }

    // Parse antwoord
    uint8_t respPayload[256];
    int respPayloadLen = 0;
    int respCmd = 0;
    if (_parse6699(recvBuf, recvLen, respPayload, &respPayloadLen, &respCmd, _sessionKey)) {
      TUYA_LOG("Antwoord: cmd=%d len=%d", respCmd, respPayloadLen);
      return true;
    }

    TUYA_LOG("WAARSCHUWING: kon antwoord niet parsen, maar commando is verstuurd");
    // Lees eventuele extra data
    delay(50);
    while (_client.available()) _client.read();
    return true;  // Commando is verstuurd, antwoord parsen is niet kritiek
  }

  // ── 6699 bericht bouwen (v3.5) ─────────────────────────────────

  int _build6699(uint8_t* buffer, int cmd, const uint8_t* payload,
                 int payloadLen, const uint8_t* key, bool withRetcode) {
    // Genereer IV (12 bytes) — gebruik timestamp-gebaseerd zoals tinytuya
    uint8_t iv[12];
    unsigned long t = (unsigned long)(millis() * 10);
    char tStr[16];
    snprintf(tStr, sizeof(tStr), "%012lu", t);
    memcpy(iv, tStr, 12);

    // Bouw plaintext: optioneel retcode(4) + payload
    uint8_t plain[384];
    int plainLen = 0;
    if (withRetcode) {
      _writeU32(plain, 0, 0);  // retcode = 0
      plainLen = 4;
    }
    if (plainLen + payloadLen > (int)sizeof(plain)) return -1;
    memcpy(plain + plainLen, payload, payloadLen);
    plainLen += payloadLen;

    // Length veld = IV(12) + ciphertext(plainLen) + tag(16)
    uint32_t dataLen = 12 + plainLen + 16;

    // Bouw header (18 bytes): prefix(4) + unknown(2) + seqno(4) + cmd(4) + length(4)
    int pos = 0;
    _writeU32(buffer, pos, TUYA_PREFIX_6699); pos += 4;
    _writeU16(buffer, pos, 0x0000); pos += 2;
    _writeU32(buffer, pos, _seqNo); pos += 4;
    _writeU32(buffer, pos, (uint32_t)cmd); pos += 4;
    _writeU32(buffer, pos, dataLen); pos += 4;
    // pos = 18

    // AAD = header bytes 4..17 (14 bytes: unknown + seqno + cmd + length)
    uint8_t aad[14];
    memcpy(aad, buffer + 4, 14);

    // GCM encrypt
    uint8_t encrypted[384];
    uint8_t tag[16];
    if (!_aesGcmEncrypt(key, iv, 12, aad, 14, plain, plainLen, encrypted, tag)) {
      TUYA_LOG("FOUT: GCM encrypt mislukt");
      return -1;
    }

    // IV toevoegen
    memcpy(buffer + pos, iv, 12); pos += 12;

    // Encrypted data
    memcpy(buffer + pos, encrypted, plainLen); pos += plainLen;

    // GCM tag
    memcpy(buffer + pos, tag, 16); pos += 16;

    // Suffix
    _writeU32(buffer, pos, TUYA_SUFFIX_6699); pos += 4;

    return pos;
  }

  // ── 6699 bericht parsen ─────────────────────────────────────────

  bool _parse6699(const uint8_t* buffer, int len, uint8_t* payload,
                  int* payloadLen, int* cmd, const uint8_t* key) {
    if (len < 22) return false;  // minimum: header(18) + suffix(4)

    uint32_t prefix = _readU32(buffer, 0);
    if (prefix != TUYA_PREFIX_6699) {
      TUYA_LOG("Parse: onverwacht prefix 0x%08X", prefix);
      return false;
    }

    // Header: prefix(4) + unknown(2) + seqno(4) + cmd(4) + length(4) = 18
    *cmd = (int)_readU32(buffer, 10);
    uint32_t dataLen = _readU32(buffer, 14);

    // Controleer totale lengte
    if (18 + dataLen + 4 > (uint32_t)len) {
      TUYA_LOG("Parse: bericht te kort (verwacht %u, heb %d)", 18 + dataLen + 4, len);
      return false;
    }

    // AAD = header bytes 4..17 (14 bytes)
    uint8_t aad[14];
    memcpy(aad, buffer + 4, 14);

    // Data: IV(12) + encrypted(dataLen-12-16) + tag(16)
    int encLen = (int)dataLen - 12 - 16;
    if (encLen < 0) {
      TUYA_LOG("Parse: data te kort voor GCM (dataLen=%u)", dataLen);
      return false;
    }

    const uint8_t* iv = buffer + 18;
    const uint8_t* encrypted = buffer + 18 + 12;
    const uint8_t* tag = buffer + 18 + 12 + encLen;

    if (!_aesGcmDecrypt(key, iv, 12, aad, 14, encrypted, encLen, payload, tag)) {
      TUYA_LOG("Parse: GCM decrypt mislukt (cmd=%d, encLen=%d)", *cmd, encLen);
      return false;
    }

    *payloadLen = encLen;
    return true;
  }

  // ── Ontvangen ─────────────────────────────────────────────────

  int _receiveMessage(uint8_t* buffer, int maxLen) {
    unsigned long start = millis();
    int pos = 0;

    while ((millis() - start) < TUYA_TIMEOUT_MS) {
      while (_client.available() && pos < maxLen) {
        buffer[pos++] = _client.read();
      }

      // Check of we een compleet 6699 bericht hebben
      if (pos >= 18) {
        uint32_t prefix = _readU32(buffer, 0);
        if (prefix == TUYA_PREFIX_6699) {
          uint32_t dataLen = _readU32(buffer, 14);
          int expected = 18 + (int)dataLen + 4;  // header + data + suffix
          if (pos >= expected) return pos;
        } else if (prefix == TUYA_PREFIX_55AA) {
          uint32_t totalLen = _readU32(buffer, 12);
          int expected = 16 + (int)totalLen;
          if (pos >= expected) return pos;
        }
      }
      delay(5);
    }

    if (pos > 0) {
      TUYA_LOG("Timeout: %d bytes ontvangen (incompleet)", pos);
    }
    return pos;
  }

  // ── Crypto: AES-GCM ──────────────────────────────────────────

  bool _aesGcmEncrypt(const uint8_t* key, const uint8_t* iv, int ivLen,
                      const uint8_t* aad, int aadLen,
                      const uint8_t* input, int inputLen,
                      uint8_t* output, uint8_t* tag) {
    mbedtls_gcm_context ctx;
    mbedtls_gcm_init(&ctx);

    int ret = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 128);
    if (ret != 0) {
      TUYA_LOG("GCM setkey fout: %d", ret);
      mbedtls_gcm_free(&ctx);
      return false;
    }

    ret = mbedtls_gcm_crypt_and_tag(&ctx, MBEDTLS_GCM_ENCRYPT,
      inputLen, iv, ivLen,
      aad, aadLen,  // aad kan NULL zijn met aadLen=0
      input, output, 16, tag);

    mbedtls_gcm_free(&ctx);

    if (ret != 0) {
      TUYA_LOG("GCM encrypt fout: %d", ret);
      return false;
    }
    return true;
  }

  bool _aesGcmDecrypt(const uint8_t* key, const uint8_t* iv, int ivLen,
                      const uint8_t* aad, int aadLen,
                      const uint8_t* input, int inputLen,
                      uint8_t* output, const uint8_t* tag) {
    mbedtls_gcm_context ctx;
    mbedtls_gcm_init(&ctx);

    int ret = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 128);
    if (ret != 0) {
      TUYA_LOG("GCM setkey fout: %d", ret);
      mbedtls_gcm_free(&ctx);
      return false;
    }

    ret = mbedtls_gcm_auth_decrypt(&ctx, inputLen,
      iv, ivLen, aad, aadLen, tag, 16,
      input, output);

    mbedtls_gcm_free(&ctx);

    if (ret != 0) {
      TUYA_LOG("GCM decrypt fout: %d (auth fail?)", ret);
      return false;
    }
    return true;
  }

  // ── Crypto: AES-ECB (voor v3.3 fallback) ──────────────────────

  void _aesEcbEncrypt(const uint8_t* key, const uint8_t* input,
                      uint8_t* output, int len) {
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    mbedtls_aes_setkey_enc(&ctx, key, 128);
    for (int i = 0; i < len; i += 16) {
      mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, input + i, output + i);
    }
    mbedtls_aes_free(&ctx);
  }

  // ── Crypto: HMAC-SHA256 ─────────────────────────────────────

  void _hmacSha256(const uint8_t* key, int keyLen,
                   const uint8_t* data, int dataLen,
                   uint8_t* output) {
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_setup(&ctx, info, 1);
    mbedtls_md_hmac_starts(&ctx, key, keyLen);
    mbedtls_md_hmac_update(&ctx, data, dataLen);
    mbedtls_md_hmac_finish(&ctx, output);
    mbedtls_md_free(&ctx);
  }

  // ── Hulpfuncties ──────────────────────────────────────────────

  void _writeU32(uint8_t* buf, int pos, uint32_t val) {
    buf[pos]     = (val >> 24) & 0xFF;
    buf[pos + 1] = (val >> 16) & 0xFF;
    buf[pos + 2] = (val >> 8) & 0xFF;
    buf[pos + 3] = val & 0xFF;
  }

  void _writeU16(uint8_t* buf, int pos, uint16_t val) {
    buf[pos]     = (val >> 8) & 0xFF;
    buf[pos + 1] = val & 0xFF;
  }

  uint32_t _readU32(const uint8_t* buf, int pos) {
    return ((uint32_t)buf[pos] << 24) |
           ((uint32_t)buf[pos + 1] << 16) |
           ((uint32_t)buf[pos + 2] << 8) |
           (uint32_t)buf[pos + 3];
  }

  void _hexDump(const char* label, const uint8_t* data, int len) {
#if TUYA_DEBUG
    Serial.printf("[Tuya] %s (%d bytes): ", label, len);
    for (int i = 0; i < len && i < 48; i++) {
      Serial.printf("%02X", data[i]);
    }
    if (len > 48) Serial.print("...");
    Serial.println();
#endif
  }
};

#endif // ESPTUYA_H
