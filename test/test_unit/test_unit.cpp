/**
 * @file test_unit.cpp
 * @brief Unit tests para iot-mqtt-tls — lógica pura, sin hardware
 *
 * Ejecutar con: pio test -e native
 *
 * FILOSOFÍA: estas pruebas verifican la lógica de negocio extraída del
 * firmware. Se ejecutan en milisegundos y no necesitan ningún hardware.
 * Esta es la base de la pirámide de testing en IoT industrial.
 *
 * Funciones probadas (extraídas de libiot.cpp / libstorage.cpp):
 *   - buildSensorPayload()   → formato JSON del mensaje MQTT
 *   - isAlertMessage()       → detección de alertas en el payload
 *   - isOTAMessage()         → comparación de topics MQTT
 *   - isValidSSID()          → validación IEEE 802.11
 *   - isValidVersion()       → formato de versión semántica vX.Y.Z
 *   - formatDeviceID()       → identificador único del dispositivo
 *   - isMeasureTime()        → temporización de mediciones
 *   - isAlertExpired()       → expiración de alertas
 */

#ifndef ARDUINO   // Solo se compila en entorno native

#include <unity.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

// ═══════════════════════════════════════════════════════════════════════
// FUNCIONES PURAS EXTRAÍDAS DEL FIRMWARE
// En producción estas viven en libiot.cpp y libstorage.cpp.
// Las aislamos aquí para que los tests sean completamente autónomos:
// cero dependencias de Arduino, WiFi, MQTT, sensores.
// ═══════════════════════════════════════════════════════════════════════

/**
 * Construye el payload JSON que se publica al broker MQTT.
 * Extraída de sendSensorData() en libiot.cpp.
 *
 * Formato de salida: {"temperatura": 25.5, "humedad": 60.3}
 */
static void buildSensorPayload(float temp, float hum, char* buf, size_t len) {
    snprintf(buf, len,
             "{\"temperatura\": %.1f, \"humedad\": %.1f}",
             temp, hum);
}

/**
 * Detecta si un mensaje MQTT contiene la cadena "ALERT".
 * Extraída de receivedCallback() en libiot.cpp.
 * El broker envía ALERT en mayúsculas — la comparación es case-sensitive.
 */
static bool isAlertMessage(const char* payload) {
    return payload != nullptr && strstr(payload, "ALERT") != nullptr;
}

/**
 * Verifica si el topic recibido coincide con el topic OTA configurado.
 * Extraída de receivedCallback() en libiot.cpp.
 */
static bool isOTAMessage(const char* receivedTopic, const char* otaTopic) {
    return strcmp(receivedTopic, otaTopic) == 0;
}

/**
 * Valida un SSID según la especificación IEEE 802.11:
 * longitud entre 1 y 32 bytes.
 */
static bool isValidSSID(const char* ssid) {
    if (ssid == nullptr) return false;
    size_t len = strlen(ssid);
    return len > 0 && len <= 32;
}

/**
 * Valida el formato de versión de firmware: debe iniciar con 'v'
 * y tener al menos 5 caracteres (ej: v1.0.0).
 */
static bool isValidVersion(const char* version) {
    if (version == nullptr) return false;
    if (strlen(version) < 5) return false;
    return version[0] == 'v';
}

/**
 * Formatea la MAC address como Device ID único del dispositivo.
 * Versión corregida de getMacAddress() en libiot.cpp.
 *
 * BUG en el original: usa char macStr[18] pero necesita 19 bytes
 * (18 caracteres + 1 null terminator), lo que causa truncamiento.
 */
static void formatDeviceID(const uint8_t mac[6], char* out, size_t len) {
    snprintf(out, len,
             "ESP32-%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

/**
 * Verifica si ya transcurrió el intervalo de medición.
 * Extraída de measure() en libiot.cpp.
 * Usa aritmética unsigned para manejar correctamente el overflow de millis().
 */
static bool isMeasureTime(unsigned long lastMs, unsigned long nowMs,
                           unsigned long intervalMs) {
    return (nowMs - lastMs) >= intervalMs;
}

/**
 * Verifica si una alerta ya expiró (debe ocultarse en el display).
 * Extraída de checkAlert() en libiot.cpp.
 */
static bool isAlertExpired(unsigned long alertStartMs, unsigned long nowMs,
                            unsigned long durationMs) {
    return (nowMs - alertStartMs) >= durationMs;
}

// ═══════════════════════════════════════════════════════════════════════
// FIXTURES DE UNITY
// setUp se ejecuta ANTES de cada test, tearDown DESPUÉS.
// ═══════════════════════════════════════════════════════════════════════

void setUp(void)    {}
void tearDown(void) {}

// ═══════════════════════════════════════════════════════════════════════
// GRUPO 1: PAYLOAD JSON DEL SENSOR
// Verifica que el firmware construye mensajes MQTT correctos.
// Un payload malformado causaría que el broker o el backend rechacen
// los datos silenciosamente — muy difícil de depurar sin tests.
// ═══════════════════════════════════════════════════════════════════════

void test_payload_tiene_claves_temperatura_y_humedad(void) {
    char buf[80];
    buildSensorPayload(25.5f, 60.3f, buf, sizeof(buf));
    TEST_ASSERT_NOT_NULL(strstr(buf, "temperatura"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "humedad"));
}

void test_payload_es_objeto_json_con_llaves(void) {
    char buf[80];
    buildSensorPayload(25.5f, 60.3f, buf, sizeof(buf));
    TEST_ASSERT_EQUAL('{', buf[0]);
    TEST_ASSERT_EQUAL('}', buf[strlen(buf) - 1]);
}

void test_payload_temperatura_con_un_decimal(void) {
    char buf[80];
    buildSensorPayload(25.5f, 60.0f, buf, sizeof(buf));
    TEST_ASSERT_NOT_NULL(strstr(buf, "25.5"));
}

void test_payload_humedad_con_un_decimal(void) {
    char buf[80];
    buildSensorPayload(25.0f, 60.3f, buf, sizeof(buf));
    TEST_ASSERT_NOT_NULL(strstr(buf, "60.3"));
}

void test_payload_temperatura_negativa_bajo_cero(void) {
    // SHT21 mide desde -40°C — el payload debe manejar negativos
    char buf[80];
    buildSensorPayload(-10.0f, 80.0f, buf, sizeof(buf));
    TEST_ASSERT_NOT_NULL(strstr(buf, "-10.0"));
}

void test_payload_temperatura_limite_maximo_sht21(void) {
    // Límite superior del SHT21: 125°C
    char buf[80];
    buildSensorPayload(125.0f, 100.0f, buf, sizeof(buf));
    TEST_ASSERT_NOT_NULL(strstr(buf, "125.0"));
}

void test_payload_cabe_en_buffer_mqtt_por_defecto(void) {
    // PubSubClient tiene buffer de 256 bytes por defecto
    // El payload más largo posible: temp=-40.0 hum=100.0
    char buf[80];
    buildSensorPayload(-40.0f, 100.0f, buf, sizeof(buf));
    int len = (int)strlen(buf);
    TEST_ASSERT_LESS_THAN(256, len);
}

// ═══════════════════════════════════════════════════════════════════════
// GRUPO 2: DETECCIÓN DE ALERTAS
// El broker envía "ALERT: ..." al topic /in para activar una alarma
// en el display. Un falso positivo o falso negativo pasaría inadvertido
// sin pruebas explícitas.
// ═══════════════════════════════════════════════════════════════════════

void test_alerta_detectada_con_prefijo_ALERT(void) {
    TEST_ASSERT_TRUE(isAlertMessage("ALERT: temperatura alta 38C"));
}

void test_alerta_detectada_con_ALERT_al_inicio(void) {
    TEST_ASSERT_TRUE(isAlertMessage("ALERT"));
}

void test_alerta_no_activada_por_mensaje_sensor_normal(void) {
    TEST_ASSERT_FALSE(isAlertMessage("{\"temperatura\": 25.0, \"humedad\": 60.0}"));
}

void test_alerta_no_activada_por_payload_vacio(void) {
    TEST_ASSERT_FALSE(isAlertMessage(""));
}

void test_alerta_es_case_sensitive_minuscula_no_dispara(void) {
    // El protocolo del backend usa MAYÚSCULAS — "alert" en minúsculas
    // NO debe activar la alarma (evita falsos positivos con texto libre)
    TEST_ASSERT_FALSE(isAlertMessage("alert: temperatura alta"));
}

void test_alerta_no_activada_por_mensaje_ota(void) {
    TEST_ASSERT_FALSE(isAlertMessage("{\"url\":\"https://...\",\"version\":\"v1.2.0\"}"));
}

// ═══════════════════════════════════════════════════════════════════════
// GRUPO 3: COMPARACIÓN DE TOPICS MQTT
// Un error en la comparación de topics haría que mensajes OTA se
// ignoren o que mensajes normales activen una actualización de firmware.
// ═══════════════════════════════════════════════════════════════════════

void test_ota_topic_coincide_exactamente(void) {
    TEST_ASSERT_TRUE(
        isOTAMessage("dispositivo/device1/ota", "dispositivo/device1/ota")
    );
}

void test_ota_topic_data_no_coincide_con_ota(void) {
    TEST_ASSERT_FALSE(
        isOTAMessage("dispositivo/device1/data", "dispositivo/device1/ota")
    );
}

void test_ota_topic_vacio_no_coincide(void) {
    TEST_ASSERT_FALSE(isOTAMessage("", "dispositivo/device1/ota"));
}

void test_ota_topic_con_wildcard_no_coincide(void) {
    // Los topics se comparan exactamente — wildcards MQTT no aplican aquí
    TEST_ASSERT_FALSE(
        isOTAMessage("dispositivo/+/ota", "dispositivo/device1/ota")
    );
}

void test_ota_topic_diferente_device_no_coincide(void) {
    TEST_ASSERT_FALSE(
        isOTAMessage("dispositivo/device2/ota", "dispositivo/device1/ota")
    );
}

// ═══════════════════════════════════════════════════════════════════════
// GRUPO 4: VALIDACIÓN DE SSID (IEEE 802.11)
// Un SSID vacío en Preferences causaría reconexiones infinitas de WiFi.
// ═══════════════════════════════════════════════════════════════════════

void test_ssid_valido_nombre_tipico(void) {
    TEST_ASSERT_TRUE(isValidSSID("MiRedWiFi"));
}

void test_ssid_valido_de_un_caracter(void) {
    TEST_ASSERT_TRUE(isValidSSID("X"));
}

void test_ssid_valido_limite_maximo_32_chars(void) {
    // 32 caracteres es el máximo según IEEE 802.11
    TEST_ASSERT_TRUE(isValidSSID("12345678901234567890123456789012")); // 32
}

void test_ssid_invalido_vacio(void) {
    TEST_ASSERT_FALSE(isValidSSID(""));
}

void test_ssid_invalido_33_chars_supera_limite(void) {
    TEST_ASSERT_FALSE(isValidSSID("123456789012345678901234567890123")); // 33
}

// ═══════════════════════════════════════════════════════════════════════
// GRUPO 5: VERSIÓN DE FIRMWARE
// El sistema OTA compara versiones para decidir si actualizar.
// Un formato inválido causaría que nunca se apliquen actualizaciones.
// ═══════════════════════════════════════════════════════════════════════

void test_version_formato_correcto_v1_1_1(void) {
    TEST_ASSERT_TRUE(isValidVersion("v1.1.1"));
}

void test_version_formato_mayor_v2_0_0(void) {
    TEST_ASSERT_TRUE(isValidVersion("v2.0.0"));
}

void test_version_sin_prefijo_v_rechazada(void) {
    // Todos los firmwares deben iniciar con 'v'
    TEST_ASSERT_FALSE(isValidVersion("1.1.1"));
}

void test_version_demasiado_corta_rechazada(void) {
    // "v1.0" tiene 4 chars — no cumple el mínimo de 5
    TEST_ASSERT_FALSE(isValidVersion("v1.0"));
}

void test_version_vacia_rechazada(void) {
    TEST_ASSERT_FALSE(isValidVersion(""));
}

// ═══════════════════════════════════════════════════════════════════════
// GRUPO 6: DEVICE ID (MAC ADDRESS)
// El client_id de MQTT se basa en la MAC. Un formato incorrecto
// causaría conflictos si dos dispositivos usan el mismo client_id.
//
// BUG DETECTADO: getMacAddress() en libiot.cpp usa char macStr[18]
// pero el resultado tiene 18 chars + 1 null = necesita 19 bytes.
// snprintf() con buffer de 18 trunca el último carácter hex.
// Este test documenta el comportamiento CORRECTO.
// ═══════════════════════════════════════════════════════════════════════

void test_device_id_comienza_con_prefijo_ESP32(void) {
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    char buf[24];
    formatDeviceID(mac, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING_LEN("ESP32-", buf, 6);
}

void test_device_id_longitud_exacta_18_caracteres(void) {
    // "ESP32-" (6) + 6 bytes x 2 hex cada uno (12) = 18 chars
    // BUG: el buffer original [18] trunca el null terminator
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    char buf[24]; // 24 para no truncar — el original usa 18 (insuficiente)
    formatDeviceID(mac, buf, sizeof(buf));
    TEST_ASSERT_EQUAL(18, (int)strlen(buf));
}

void test_device_id_hex_en_mayusculas(void) {
    // Los bytes 0xab 0xcd 0xef deben aparecer como AB CD EF
    uint8_t mac[6] = {0xab, 0xcd, 0xef, 0x12, 0x34, 0x56};
    char buf[24];
    formatDeviceID(mac, buf, sizeof(buf));
    TEST_ASSERT_NOT_NULL(strstr(buf, "AB"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "CD"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "EF"));
}

void test_device_id_formato_completo_conocido(void) {
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    char buf[24];
    formatDeviceID(mac, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("ESP32-AABBCCDDEEFF", buf);
}

// ═══════════════════════════════════════════════════════════════════════
// GRUPO 7: TEMPORIZACIÓN
// El firmware decide cuándo medir y cuándo ocultar alertas usando
// millis(). Un error aquí causaría mediciones perdidas o alertas eternas.
// ═══════════════════════════════════════════════════════════════════════

void test_medicion_se_activa_exactamente_al_cumplir_intervalo(void) {
    // lastMeasure = 0, now = 2000ms, interval = 2000ms → medir
    TEST_ASSERT_TRUE(isMeasureTime(0UL, 2000UL, 2000UL));
}

void test_medicion_no_se_activa_antes_del_intervalo(void) {
    // lastMeasure = 0, now = 1999ms, interval = 2000ms → esperar
    TEST_ASSERT_FALSE(isMeasureTime(0UL, 1999UL, 2000UL));
}

void test_alerta_expira_despues_de_la_duracion_configurada(void) {
    // alertStart = 0, now = 61000ms, duration = 60000ms → expirada
    TEST_ASSERT_TRUE(isAlertExpired(0UL, 61000UL, 60000UL));
}

void test_alerta_no_expira_antes_de_la_duracion(void) {
    // alertStart = 0, now = 30000ms, duration = 60000ms → activa
    TEST_ASSERT_FALSE(isAlertExpired(0UL, 30000UL, 60000UL));
}

void test_medicion_maneja_overflow_de_millis_correctamente(void) {
    // millis() desborda cada ~49.7 días (2^32 ms).
    // La aritmética unsigned sin signo hace el wrap-around automáticamente:
    // elapsed = (0x000007D4 - 0xFFFFFF00) en uint32
    //         = 2004 + 256 = 2260 ms > 2000 ms → medir
    unsigned long lastMeasure = 0xFFFFFF00UL;
    unsigned long now         = 0x000007D4UL; // 2004 ms después del desborde
    TEST_ASSERT_TRUE(isMeasureTime(lastMeasure, now, 2000UL));
}

void test_medicion_no_dispara_con_overflow_si_intervalo_no_cumplido(void) {
    // Justo antes: elapsed = 0xF0 = 240 ms < 2000 ms → no medir
    unsigned long lastMeasure = 0xFFFFFF00UL;
    unsigned long now         = 0xFFFFFFF0UL; // 240 ms después
    TEST_ASSERT_FALSE(isMeasureTime(lastMeasure, now, 2000UL));
}

// ═══════════════════════════════════════════════════════════════════════
// TEST RUNNER
// ═══════════════════════════════════════════════════════════════════════

void setup(void) {
    UNITY_BEGIN();

    // Grupo 1: JSON payload
    RUN_TEST(test_payload_tiene_claves_temperatura_y_humedad);
    RUN_TEST(test_payload_es_objeto_json_con_llaves);
    RUN_TEST(test_payload_temperatura_con_un_decimal);
    RUN_TEST(test_payload_humedad_con_un_decimal);
    RUN_TEST(test_payload_temperatura_negativa_bajo_cero);
    RUN_TEST(test_payload_temperatura_limite_maximo_sht21);
    RUN_TEST(test_payload_cabe_en_buffer_mqtt_por_defecto);

    // Grupo 2: Alert detection
    RUN_TEST(test_alerta_detectada_con_prefijo_ALERT);
    RUN_TEST(test_alerta_detectada_con_ALERT_al_inicio);
    RUN_TEST(test_alerta_no_activada_por_mensaje_sensor_normal);
    RUN_TEST(test_alerta_no_activada_por_payload_vacio);
    RUN_TEST(test_alerta_es_case_sensitive_minuscula_no_dispara);
    RUN_TEST(test_alerta_no_activada_por_mensaje_ota);

    // Grupo 3: OTA topic
    RUN_TEST(test_ota_topic_coincide_exactamente);
    RUN_TEST(test_ota_topic_data_no_coincide_con_ota);
    RUN_TEST(test_ota_topic_vacio_no_coincide);
    RUN_TEST(test_ota_topic_con_wildcard_no_coincide);
    RUN_TEST(test_ota_topic_diferente_device_no_coincide);

    // Grupo 4: SSID validation
    RUN_TEST(test_ssid_valido_nombre_tipico);
    RUN_TEST(test_ssid_valido_de_un_caracter);
    RUN_TEST(test_ssid_valido_limite_maximo_32_chars);
    RUN_TEST(test_ssid_invalido_vacio);
    RUN_TEST(test_ssid_invalido_33_chars_supera_limite);

    // Grupo 5: Firmware version
    RUN_TEST(test_version_formato_correcto_v1_1_1);
    RUN_TEST(test_version_formato_mayor_v2_0_0);
    RUN_TEST(test_version_sin_prefijo_v_rechazada);
    RUN_TEST(test_version_demasiado_corta_rechazada);
    RUN_TEST(test_version_vacia_rechazada);

    // Grupo 6: Device ID / MAC
    RUN_TEST(test_device_id_comienza_con_prefijo_ESP32);
    RUN_TEST(test_device_id_longitud_exacta_18_caracteres);
    RUN_TEST(test_device_id_hex_en_mayusculas);
    RUN_TEST(test_device_id_formato_completo_conocido);

    // Grupo 7: Timing
    RUN_TEST(test_medicion_se_activa_exactamente_al_cumplir_intervalo);
    RUN_TEST(test_medicion_no_se_activa_antes_del_intervalo);
    RUN_TEST(test_alerta_expira_despues_de_la_duracion_configurada);
    RUN_TEST(test_alerta_no_expira_antes_de_la_duracion);
    RUN_TEST(test_medicion_maneja_overflow_de_millis_correctamente);
    RUN_TEST(test_medicion_no_dispara_con_overflow_si_intervalo_no_cumplido);

    UNITY_END();
}

int main(void) {
    setup();
    return 0;
}

#endif // !ARDUINO
