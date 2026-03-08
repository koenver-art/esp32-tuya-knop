// ═══════════════════════════════════════════════════════════════════
//  EspTuya — Tuya LAN protocol library voor ESP32
//  Ondersteunt protocol versie 3.3 en 3.5
//
//  Gebruikt mbedtls (ingebouwd in ESP32) voor AES-ECB, AES-GCM,
//  en HMAC-SHA256 cryptografie.
//
//  Gebruik:
//    EspTuya tuya;
//    if (tuya.begin("192.168.1.x", "local_key_hex", 3)) {
//      tuya.setBool(20, true);   // lamp aan
//      tuya.setInt(22, 500);     // helderheid 50%
//    }
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
#define CMD_DP_QUERY        0x0A
#define CMD_CONTROL_NEW     0x0D
#define CMD_DP_QUERY_NEW    0x10
#define CMD_SESS_NEG_START  0x03
#define CMD_SESS_NEG_RESP   0x04
#define CMD_SESS_NEG_FINISH 0x05

class EspTuya {
public:
  EspTuya() : _connected(false), _seqNo(1) {}

  // ── Publieke API ──────────────────────────────────────────────

  bool begin(const char* ip, const char* localKey, int version) {
    _version = version;
    _connected = false;
    _seqNo = 1;

    // Local key opslaan (max 16 bytes)
    memset(_deviceKey, 0, 16);
    int keyLen = strlen(localKey);
    if (keyLen > 16) keyLen = 16;
    memcpy(_deviceKey, localKey, keyLen);

    // TCP verbinding
    if (!_client.connect(ip, TUYA_PORT)) {
      Serial.printf("EspTuya: kan niet verbinden met %s:%d\n", ip, TUYA_PORT);
      return false;
    }
    _client.setTimeout(TUYA_TIMEOUT_MS / 1000);

    // v3.5 vereist session key negotiatie
    if (_version >= 5) {  // versie 3 met sub .5 = doorgegeven als 3
      // Eigenlijk: als het een v3.5 apparaat is
      // We proberen session negotiation
    }

    // Voor nu: probeer session key negotiatie voor alle versies >= 3
    if (_version >= 3) {
      if (!_negotiateSession()) {
        Serial.println("EspTuya: session negotiatie mislukt, probeer zonder");
        // Fallback: gebruik device key direct (v3.3 compat)
        memcpy(_sessionKey, _deviceKey, 16);
        _useGcm = false;
      }
    } else {
      memcpy(_sessionKey, _deviceKey, 16);
      _useGcm = false;
    }

    _connected = true;
    return true;
  }

  bool setBool(int dp, bool value) {
    if (!_connected) return false;

    char json[128];
    snprintf(json, sizeof(json),
      "{\"protocol\":5,\"t\":%lu,\"data\":{\"dps\":{\"%d\":%s}}}",
      (unsigned long)time(nullptr), dp, value ? "true" : "false");

    return _sendControl(json);
  }

  bool setInt(int dp, int value) {
    if (!_connected) return false;

    char json[128];
    snprintf(json, sizeof(json),
      "{\"protocol\":5,\"t\":%lu,\"data\":{\"dps\":{\"%d\":%d}}}",
      (unsigned long)time(nullptr), dp, value);

    return _sendControl(json);
  }

  void end() {
    if (_client.connected()) _client.stop();
    _connected = false;
  }

  ~EspTuya() { end(); }

private:
  WiFiClient _client;
  uint8_t _deviceKey[16];
  uint8_t _sessionKey[16];
  uint8_t _localNonce[16];
  int _version;
  uint32_t _seqNo;
  bool _connected;
  bool _useGcm = true;

  // ── Session key negotiatie (v3.4/v3.5) ────────────────────────

  bool _negotiateSession() {
    // Genereer local nonce
    for (int i = 0; i < 16; i++) _localNonce[i] = random(0, 256);

    // Stap 1: Stuur local nonce (GCM-encrypted met device key)
    uint8_t sendBuf[TUYA_BUF_SIZE];
    int msgLen = _buildMessage(sendBuf, CMD_SESS_NEG_START, _localNonce, 16, _deviceKey);
    if (msgLen <= 0) return false;

    _client.write(sendBuf, msgLen);
    _seqNo++;

    // Stap 2: Ontvang remote nonce + HMAC
    uint8_t recvBuf[TUYA_BUF_SIZE];
    int recvLen = _receiveMessage(recvBuf, TUYA_BUF_SIZE);
    if (recvLen <= 0) return false;

    // Parse en decrypt response
    uint8_t payload[256];
    int payloadLen = 0;
    int respCmd = 0;
    if (!_parseMessage(recvBuf, recvLen, payload, &payloadLen, &respCmd, _deviceKey)) {
      Serial.println("EspTuya: kan response niet parsen");
      return false;
    }

    if (respCmd != CMD_SESS_NEG_RESP || payloadLen < 48) {
      Serial.printf("EspTuya: onverwacht antwoord cmd=%d len=%d\n", respCmd, payloadLen);
      return false;
    }

    // Remote nonce = eerste 16 bytes, HMAC = volgende 32 bytes
    uint8_t remoteNonce[16];
    uint8_t receivedHmac[32];
    memcpy(remoteNonce, payload, 16);
    memcpy(receivedHmac, payload + 16, 32);

    // Verifieer HMAC van local nonce
    uint8_t expectedHmac[32];
    _hmacSha256(_deviceKey, 16, _localNonce, 16, expectedHmac);
    if (memcmp(receivedHmac, expectedHmac, 32) != 0) {
      Serial.println("EspTuya: HMAC verificatie mislukt");
      return false;
    }

    // Stap 3: Stuur HMAC van remote nonce
    uint8_t remoteHmac[32];
    _hmacSha256(_deviceKey, 16, remoteNonce, 16, remoteHmac);

    msgLen = _buildMessage(sendBuf, CMD_SESS_NEG_FINISH, remoteHmac, 32, _deviceKey);
    if (msgLen <= 0) return false;

    _client.write(sendBuf, msgLen);
    _seqNo++;

    // Stap 4: Bereken session key
    // XOR local en remote nonce
    uint8_t xorResult[16];
    for (int i = 0; i < 16; i++) {
      xorResult[i] = _localNonce[i] ^ remoteNonce[i];
    }

    // v3.5: GCM encrypt XOR resultaat met device key
    uint8_t iv[12];
    memcpy(iv, _localNonce, 12);

    uint8_t encrypted[16];
    uint8_t tag[16];
    if (!_aesGcmEncrypt(_deviceKey, iv, 12, iv, 12, xorResult, 16, encrypted, tag)) {
      Serial.println("EspTuya: session key afleiding mislukt");
      return false;
    }

    memcpy(_sessionKey, encrypted, 16);
    _useGcm = true;

    // Kort wachten op eventueel antwoord
    delay(100);
    // Lees en negeer eventueel antwoord
    while (_client.available()) _client.read();

    Serial.println("EspTuya: session key OK");
    return true;
  }

  // ── Commando versturen ────────────────────────────────────────

  bool _sendControl(const char* json) {
    // Versie prefix toevoegen voor CONTROL commando's
    uint8_t payload[256];
    int payloadLen = 0;

    if (_useGcm) {
      // v3.5: prepend "3.5" + 12 null bytes (15 bytes totaal)
      memcpy(payload, "3.5", 3);
      memset(payload + 3, 0, 12);
      payloadLen = 15;
    } else {
      // v3.3: prepend "3.3" + 12 null bytes
      memcpy(payload, "3.3", 3);
      memset(payload + 3, 0, 12);
      payloadLen = 15;
    }

    int jsonLen = strlen(json);
    if (payloadLen + jsonLen >= (int)sizeof(payload)) return false;
    memcpy(payload + payloadLen, json, jsonLen);
    payloadLen += jsonLen;

    int cmd = _useGcm ? CMD_CONTROL_NEW : CMD_CONTROL;

    uint8_t sendBuf[TUYA_BUF_SIZE];
    int msgLen = _buildMessage(sendBuf, cmd, payload, payloadLen, _sessionKey);
    if (msgLen <= 0) return false;

    _client.write(sendBuf, msgLen);
    _seqNo++;

    // Wacht op antwoord
    delay(100);
    uint8_t recvBuf[TUYA_BUF_SIZE];
    int recvLen = _receiveMessage(recvBuf, TUYA_BUF_SIZE);

    // Lees eventuele extra berichten
    delay(50);
    while (_client.available()) _client.read();

    return (recvLen > 0);
  }

  // ── Berichten bouwen en parsen ─────────────────────────────────

  int _buildMessage(uint8_t* buffer, int cmd, const uint8_t* payload,
                    int payloadLen, const uint8_t* key) {
    if (_useGcm) {
      return _buildMessage6699(buffer, cmd, payload, payloadLen, key);
    } else {
      return _buildMessage55AA(buffer, cmd, payload, payloadLen, key);
    }
  }

  // v3.5: 6699 framing met AES-GCM
  int _buildMessage6699(uint8_t* buffer, int cmd, const uint8_t* payload,
                        int payloadLen, const uint8_t* key) {
    // Genereer random IV
    uint8_t iv[12];
    for (int i = 0; i < 12; i++) iv[i] = random(0, 256);

    // Length = IV(12) + encrypted(payloadLen) + tag(16)
    uint32_t dataLen = 12 + payloadLen + 16;

    // Bouw header (18 bytes)
    int pos = 0;
    _writeU32(buffer, pos, TUYA_PREFIX_6699); pos += 4;
    _writeU16(buffer, pos, 0x0000); pos += 2;  // unknown
    _writeU32(buffer, pos, _seqNo); pos += 4;
    _writeU32(buffer, pos, (uint32_t)cmd); pos += 4;
    _writeU32(buffer, pos, dataLen); pos += 4;
    // pos = 18

    // AAD = header bytes 4-17 (14 bytes)
    uint8_t aad[14];
    memcpy(aad, buffer + 4, 14);

    // GCM encrypt
    uint8_t encrypted[256];
    uint8_t tag[16];
    if (!_aesGcmEncrypt(key, iv, 12, aad, 14, payload, payloadLen, encrypted, tag)) {
      return -1;
    }

    // IV toevoegen
    memcpy(buffer + pos, iv, 12); pos += 12;

    // Encrypted data toevoegen
    memcpy(buffer + pos, encrypted, payloadLen); pos += payloadLen;

    // Tag toevoegen
    memcpy(buffer + pos, tag, 16); pos += 16;

    // Suffix
    _writeU32(buffer, pos, TUYA_SUFFIX_6699); pos += 4;

    return pos;
  }

  // v3.3: 55AA framing met AES-ECB
  int _buildMessage55AA(uint8_t* buffer, int cmd, const uint8_t* payload,
                        int payloadLen, const uint8_t* key) {
    // PKCS7 padding
    int paddedLen = ((payloadLen / 16) + 1) * 16;
    uint8_t padded[256];
    memcpy(padded, payload, payloadLen);
    uint8_t padVal = paddedLen - payloadLen;
    for (int i = payloadLen; i < paddedLen; i++) padded[i] = padVal;

    // Encrypt
    uint8_t encrypted[256];
    _aesEcbEncrypt(key, padded, encrypted, paddedLen);

    // CRC32 placeholder (4 bytes)
    // Length = encrypted + CRC(4) + suffix(4)
    uint32_t totalLen = paddedLen + 4 + 4;

    int pos = 0;
    _writeU32(buffer, pos, TUYA_PREFIX_55AA); pos += 4;
    _writeU32(buffer, pos, _seqNo); pos += 4;
    _writeU32(buffer, pos, (uint32_t)cmd); pos += 4;
    _writeU32(buffer, pos, totalLen); pos += 4;

    // Encrypted payload
    memcpy(buffer + pos, encrypted, paddedLen); pos += paddedLen;

    // CRC32 (over alles behalve suffix)
    uint32_t crc = _crc32(buffer, pos);
    _writeU32(buffer, pos, crc); pos += 4;

    // Suffix
    _writeU32(buffer, pos, TUYA_SUFFIX_55AA); pos += 4;

    return pos;
  }

  bool _parseMessage(const uint8_t* buffer, int len, uint8_t* payload,
                     int* payloadLen, int* cmd, const uint8_t* key) {
    if (len < 18) return false;

    uint32_t prefix = _readU32(buffer, 0);

    if (prefix == TUYA_PREFIX_6699) {
      return _parseMessage6699(buffer, len, payload, payloadLen, cmd, key);
    } else if (prefix == TUYA_PREFIX_55AA) {
      return _parseMessage55AA(buffer, len, payload, payloadLen, cmd, key);
    }

    return false;
  }

  bool _parseMessage6699(const uint8_t* buffer, int len, uint8_t* payload,
                         int* payloadLen, int* cmd, const uint8_t* key) {
    // Header: prefix(4) + unk(2) + seqno(4) + cmd(4) + length(4) = 18
    *cmd = (int)_readU32(buffer, 10);
    uint32_t dataLen = _readU32(buffer, 14);

    // AAD = bytes 4-17
    uint8_t aad[14];
    memcpy(aad, buffer + 4, 14);

    // Na header: IV(12) + encrypted(dataLen-12-16) + tag(16) + suffix(4)
    int encLen = dataLen - 12 - 16;
    if (encLen < 0 || 18 + dataLen + 4 > (uint32_t)len) return false;

    const uint8_t* iv = buffer + 18;
    const uint8_t* encrypted = buffer + 18 + 12;
    const uint8_t* tag = buffer + 18 + 12 + encLen;

    if (!_aesGcmDecrypt(key, iv, 12, aad, 14, encrypted, encLen, payload, tag)) {
      return false;
    }

    *payloadLen = encLen;
    return true;
  }

  bool _parseMessage55AA(const uint8_t* buffer, int len, uint8_t* payload,
                         int* payloadLen, int* cmd, const uint8_t* key) {
    // Header: prefix(4) + seqno(4) + cmd(4) + length(4) = 16
    // + retcode(4) + encrypted + crc(4) + suffix(4)
    *cmd = (int)_readU32(buffer, 8);
    uint32_t totalLen = _readU32(buffer, 12);

    // Retcode na header
    int encStart = 16 + 4;  // na header + retcode
    int encLen = totalLen - 4 - 4 - 4;  // minus retcode, crc, suffix

    if (encLen <= 0 || encStart + encLen > len) return false;

    // Decrypt
    _aesEcbDecrypt(key, buffer + encStart, payload, encLen);

    // PKCS7 unpadding
    uint8_t padVal = payload[encLen - 1];
    if (padVal > 16) padVal = 0;
    *payloadLen = encLen - padVal;

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
      if (pos >= 18) {
        // Check of we een compleet bericht hebben
        uint32_t prefix = _readU32(buffer, 0);
        int headerSize = (prefix == TUYA_PREFIX_6699) ? 18 : 16;
        uint32_t dataLen;

        if (prefix == TUYA_PREFIX_6699) {
          dataLen = _readU32(buffer, 14);
          int expected = 18 + dataLen + 4;  // header + data + suffix
          if (pos >= expected) return pos;
        } else if (prefix == TUYA_PREFIX_55AA) {
          dataLen = _readU32(buffer, 12);
          int expected = 16 + dataLen;  // header + rest (length includes through suffix)
          if (pos >= expected) return pos;
        }
      }
      delay(10);
    }

    return pos;
  }

  // ── Crypto functies ───────────────────────────────────────────

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

  void _aesEcbDecrypt(const uint8_t* key, const uint8_t* input,
                      uint8_t* output, int len) {
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    mbedtls_aes_setkey_dec(&ctx, key, 128);

    for (int i = 0; i < len; i += 16) {
      mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_DECRYPT, input + i, output + i);
    }

    mbedtls_aes_free(&ctx);
  }

  bool _aesGcmEncrypt(const uint8_t* key, const uint8_t* iv, int ivLen,
                      const uint8_t* aad, int aadLen,
                      const uint8_t* input, int inputLen,
                      uint8_t* output, uint8_t* tag) {
    mbedtls_gcm_context ctx;
    mbedtls_gcm_init(&ctx);

    int ret = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 128);
    if (ret != 0) { mbedtls_gcm_free(&ctx); return false; }

    ret = mbedtls_gcm_crypt_and_tag(&ctx, MBEDTLS_GCM_ENCRYPT,
      inputLen, iv, ivLen, aad, aadLen,
      input, output, 16, tag);

    mbedtls_gcm_free(&ctx);
    return (ret == 0);
  }

  bool _aesGcmDecrypt(const uint8_t* key, const uint8_t* iv, int ivLen,
                      const uint8_t* aad, int aadLen,
                      const uint8_t* input, int inputLen,
                      uint8_t* output, const uint8_t* tag) {
    mbedtls_gcm_context ctx;
    mbedtls_gcm_init(&ctx);

    int ret = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 128);
    if (ret != 0) { mbedtls_gcm_free(&ctx); return false; }

    ret = mbedtls_gcm_auth_decrypt(&ctx, inputLen,
      iv, ivLen, aad, aadLen, tag, 16,
      input, output);

    mbedtls_gcm_free(&ctx);
    return (ret == 0);
  }

  void _hmacSha256(const uint8_t* key, int keyLen,
                   const uint8_t* data, int dataLen,
                   uint8_t* output) {
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);

    const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_setup(&ctx, info, 1);  // 1 = HMAC
    mbedtls_md_hmac_starts(&ctx, key, keyLen);
    mbedtls_md_hmac_update(&ctx, data, dataLen);
    mbedtls_md_hmac_finish(&ctx, output);

    mbedtls_md_free(&ctx);
  }

  // ── CRC32 (voor v3.3) ────────────────────────────────────────

  uint32_t _crc32(const uint8_t* data, int len) {
    uint32_t crc = 0xFFFFFFFF;
    for (int i = 0; i < len; i++) {
      crc ^= data[i];
      for (int j = 0; j < 8; j++) {
        crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
      }
    }
    return ~crc;
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
};

#endif // ESPTUYA_H
