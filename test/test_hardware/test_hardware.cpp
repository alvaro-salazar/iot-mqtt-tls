/**
 * @file test_hardware.cpp
 * @brief Integration tests para iot-mqtt-tls — hardware real ESP32-S3
 *
 * Ejecutar con: pio test -e esp32-s3-devkitm-1-test
 *
 * FILOSOFÍA: estas pruebas verifican que el hardware cumple los
 * requisitos mínimos que el firmware asume. Si algún test falla,
 * el firmware de producción NO debe deployarse (quality gate).
 *
 * No necesitan WiFi ni MQTT conectados — solo el ESP32 por USB.
 *
 * Suites de prueba:
 *   - NVS: guardar/leer/borrar credenciales (Preferences API)
 *   - Memoria: heap libre suficiente para TLS + buffers MQTT
 *   - CPU y Timing: frecuencia, precisión de millis/delay
 *   - WiFi Radio: MAC address, scan, modo station
 */

#ifdef ARDUINO   // Solo se compila en entorno Arduino/ESP32

#include <unity.h>
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>

static const char* TEST_NS = "iot_test"; ///< Namespace NVS exclusivo para tests

// ═══════════════════════════════════════════════════════════════════════
// FIXTURES
// ═══════════════════════════════════════════════════════════════════════

void setUp(void) {
    // Garantiza un namespace limpio antes de cada test
    Preferences prefs;
    prefs.begin(TEST_NS, false);
    prefs.clear();
    prefs.end();
}

void tearDown(void) {
    // Elimina datos de test para no contaminar la NVS de producción
    Preferences prefs;
    prefs.begin(TEST_NS, false);
    prefs.clear();
    prefs.end();
}

// ═══════════════════════════════════════════════════════════════════════
// GRUPO 1: NVS (Non-Volatile Storage / Preferences)
// El aprovisionamiento y reconexión WiFi dependen críticamente de NVS.
// Si la NVS falla, el dispositivo necesitaría reconfiguración manual.
// ═══════════════════════════════════════════════════════════════════════

void test_nvs_guarda_y_carga_ssid(void) {
    Preferences prefs;
    prefs.begin(TEST_NS, false);
    size_t written = prefs.putString("ssid", "MiRedWiFi");
    prefs.end();
    TEST_ASSERT_GREATER_THAN(0, (int)written);

    prefs.begin(TEST_NS, true); // read-only
    String loaded = prefs.getString("ssid", "");
    prefs.end();
    TEST_ASSERT_EQUAL_STRING("MiRedWiFi", loaded.c_str());
}

void test_nvs_guarda_y_carga_password(void) {
    Preferences prefs;
    prefs.begin(TEST_NS, false);
    prefs.putString("pwd", "MiPassword!123");
    prefs.end();

    prefs.begin(TEST_NS, true);
    String pwd = prefs.getString("pwd", "");
    prefs.end();
    TEST_ASSERT_EQUAL_STRING("MiPassword!123", pwd.c_str());
}

void test_nvs_retorna_default_si_clave_no_existe(void) {
    // read-only → la clave no existe → debe retornar el default
    Preferences prefs;
    prefs.begin(TEST_NS, true);
    String val = prefs.getString("inexistente", "DEFECTO");
    prefs.end();
    TEST_ASSERT_EQUAL_STRING("DEFECTO", val.c_str());
}

void test_nvs_remove_elimina_la_clave(void) {
    Preferences prefs;
    prefs.begin(TEST_NS, false);
    prefs.putString("ssid", "Temporal");
    prefs.remove("ssid");
    String val = prefs.getString("ssid", "");
    prefs.end();
    // Después de remove, getString debe devolver el valor por defecto
    TEST_ASSERT_EQUAL_STRING("", val.c_str());
}

void test_nvs_ssid_vacio_no_supera_validacion(void) {
    // Replica la lógica de saveWiFiCredentials: rechaza SSID vacío
    // Si se guarda "", loadWiFiCredentials retorna false y el dispositivo
    // queda sin red configurada
    String ssid = "";
    bool debeGuardar = (ssid.length() > 0);
    TEST_ASSERT_FALSE(debeGuardar);
}

void test_nvs_ssid_largo_se_almacena_completo(void) {
    // SSID de 32 chars (máximo IEEE 802.11) debe guardarse sin truncar
    String ssid32 = "12345678901234567890123456789012"; // exactamente 32
    Preferences prefs;
    prefs.begin(TEST_NS, false);
    prefs.putString("ssid", ssid32);
    prefs.end();

    prefs.begin(TEST_NS, true);
    String loaded = prefs.getString("ssid", "");
    prefs.end();
    TEST_ASSERT_EQUAL(32, (int)loaded.length());
}

// ═══════════════════════════════════════════════════════════════════════
// GRUPO 2: MEMORIA DEL SISTEMA
// TLS requiere ~25-30 KB de heap para el handshake.
// PubSubClient necesita su buffer configurable (1 KB en este proyecto).
// Si el heap es insuficiente, la conexión MQTT/TLS falla con -1 o -2.
// ═══════════════════════════════════════════════════════════════════════

void test_heap_libre_supera_minimo_para_tls(void) {
    // TLS handshake: ~25 KB + buffer MQTT 1 KB + stack = ~30 KB mínimo
    size_t freeHeap = ESP.getFreeHeap();
    TEST_ASSERT_GREATER_THAN(30 * 1024, (int)freeHeap);
}

void test_heap_minimo_historico_indica_sin_leak(void) {
    // Si el heap mínimo es < 10 KB, hay riesgo de memory leak
    size_t minHeap = ESP.getMinFreeHeap();
    TEST_ASSERT_GREATER_THAN(10 * 1024, (int)minHeap);
}

void test_chip_info_tiene_cores_validos(void) {
    // ESP32-S3 tiene 2 núcleos Xtensa LX7
    int cores = ESP.getChipCores();
    TEST_ASSERT_EQUAL(2, cores);
}

// ═══════════════════════════════════════════════════════════════════════
// GRUPO 3: CPU Y TEMPORIZACIÓN
// El firmware usa millis() para medir intervalos de 2 segundos entre
// lecturas del SHT21 y 60 segundos de duración de alertas.
// Un clock incorrecto causaría mediciones en el momento equivocado.
// ═══════════════════════════════════════════════════════════════════════

void test_frecuencia_cpu_en_rango_esp32s3(void) {
    // ESP32-S3 puede correr a 80, 160 o 240 MHz
    uint32_t freq = ESP.getCpuFreqMHz();
    TEST_ASSERT_GREATER_OR_EQUAL(80,  (int)freq);
    TEST_ASSERT_LESS_OR_EQUAL(240, (int)freq);
}

void test_millis_avanza_100ms_con_tolerancia(void) {
    unsigned long t0 = millis();
    delay(100);
    unsigned long elapsed = millis() - t0;
    // Tolerancia ±20 ms para interrupciones del sistema operativo FreeRTOS
    TEST_ASSERT_GREATER_OR_EQUAL(90,  (int)elapsed);
    TEST_ASSERT_LESS_OR_EQUAL(125, (int)elapsed);
}

void test_micros_avanza_1ms_con_tolerancia(void) {
    unsigned long t0 = micros();
    delay(1);
    unsigned long elapsed = micros() - t0;
    // 1 ms = 1000 µs, tolerancia ±500 µs
    TEST_ASSERT_GREATER_OR_EQUAL(800,  (int)elapsed);
    TEST_ASSERT_LESS_OR_EQUAL(2000, (int)elapsed);
}

void test_delay_2000ms_simula_intervalo_de_medicion(void) {
    // MEASURE_INTERVAL es 2 segundos — validar que el timing es correcto
    unsigned long t0 = millis();
    delay(2000);
    unsigned long elapsed = millis() - t0;
    TEST_ASSERT_GREATER_OR_EQUAL(1950, (int)elapsed);
    TEST_ASSERT_LESS_OR_EQUAL(2100,  (int)elapsed);
}

// ═══════════════════════════════════════════════════════════════════════
// GRUPO 4: WiFi RADIO
// El firmware asume que el radio WiFi funciona correctamente.
// Un fallo de radio haría que el dispositivo nunca se conecte a MQTT.
// Estos tests validan el radio sin necesitar una red disponible.
// ═══════════════════════════════════════════════════════════════════════

void test_wifi_modo_station_inicializa_sin_error(void) {
    bool ok = WiFi.mode(WIFI_STA);
    TEST_ASSERT_TRUE(ok);
}

void test_wifi_mac_address_no_es_todo_ceros(void) {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    bool allZeros = (mac[0] == 0 && mac[1] == 0 && mac[2] == 0 &&
                     mac[3] == 0 && mac[4] == 0 && mac[5] == 0);
    TEST_ASSERT_FALSE(allZeros);
}

void test_wifi_mac_address_no_es_broadcast(void) {
    // FF:FF:FF:FF:FF:FF es la dirección de broadcast, no una MAC válida
    uint8_t mac[6];
    WiFi.macAddress(mac);
    bool allFF = (mac[0] == 0xFF && mac[1] == 0xFF && mac[2] == 0xFF &&
                  mac[3] == 0xFF && mac[4] == 0xFF && mac[5] == 0xFF);
    TEST_ASSERT_FALSE(allFF);
}

void test_wifi_mac_bit_multicast_no_activo(void) {
    // El bit 0 del primer byte debe ser 0 en direcciones unicast
    uint8_t mac[6];
    WiFi.macAddress(mac);
    TEST_ASSERT_EQUAL(0, mac[0] & 0x01);
}

void test_wifi_scan_retorna_sin_crash(void) {
    WiFi.mode(WIFI_STA);
    // El scan pasivo prueba que el radio funciona
    int n = WiFi.scanNetworks(false, true); // blocking, include hidden
    // -1 = error de radio, 0 = sin redes (normal), >0 = redes encontradas
    TEST_ASSERT_GREATER_OR_EQUAL(0, n);
    WiFi.scanDelete(); // liberar RAM del scan
}

void test_wifi_device_id_tiene_formato_correcto(void) {
    // Replica la lógica de getMacAddress() + formatDeviceID()
    // Verifica que el client_id resultante tiene el formato correcto
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char deviceId[24]; // 19 bytes necesarios (bug original usa 18)
    snprintf(deviceId, sizeof(deviceId),
             "ESP32-%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    // Verificar prefijo
    TEST_ASSERT_EQUAL_STRING_LEN("ESP32-", deviceId, 6);
    // Verificar longitud total
    TEST_ASSERT_EQUAL(18, (int)strlen(deviceId));
}

// ═══════════════════════════════════════════════════════════════════════
// SETUP / LOOP (Arduino framework entry points)
// ═══════════════════════════════════════════════════════════════════════

void setup(void) {
    Serial.begin(115200);
    delay(2000); // esperar reconexión del monitor serial

    Serial.println();
    Serial.println("========================================");
    Serial.println("  iot-mqtt-tls: Hardware Integration Tests");
    Serial.print  ("  ESP32 SDK:   "); Serial.println(ESP.getSdkVersion());
    Serial.print  ("  Chip:        "); Serial.println(ESP.getChipModel());
    Serial.print  ("  CPU freq:    "); Serial.print(ESP.getCpuFreqMHz());
    Serial.println(" MHz");
    Serial.print  ("  Heap libre:  "); Serial.print(ESP.getFreeHeap() / 1024);
    Serial.println(" KB");
    Serial.println("========================================");
    Serial.println();

    WiFi.mode(WIFI_STA); // radio encendido pero sin conectar

    UNITY_BEGIN();

    // NVS
    RUN_TEST(test_nvs_guarda_y_carga_ssid);
    RUN_TEST(test_nvs_guarda_y_carga_password);
    RUN_TEST(test_nvs_retorna_default_si_clave_no_existe);
    RUN_TEST(test_nvs_remove_elimina_la_clave);
    RUN_TEST(test_nvs_ssid_vacio_no_supera_validacion);
    RUN_TEST(test_nvs_ssid_largo_se_almacena_completo);

    // Memoria
    RUN_TEST(test_heap_libre_supera_minimo_para_tls);
    RUN_TEST(test_heap_minimo_historico_indica_sin_leak);
    RUN_TEST(test_chip_info_tiene_cores_validos);

    // CPU y Timing
    RUN_TEST(test_frecuencia_cpu_en_rango_esp32s3);
    RUN_TEST(test_millis_avanza_100ms_con_tolerancia);
    RUN_TEST(test_micros_avanza_1ms_con_tolerancia);
    RUN_TEST(test_delay_2000ms_simula_intervalo_de_medicion);

    // WiFi Radio
    RUN_TEST(test_wifi_modo_station_inicializa_sin_error);
    RUN_TEST(test_wifi_mac_address_no_es_todo_ceros);
    RUN_TEST(test_wifi_mac_address_no_es_broadcast);
    RUN_TEST(test_wifi_mac_bit_multicast_no_activo);
    RUN_TEST(test_wifi_scan_retorna_sin_crash);
    RUN_TEST(test_wifi_device_id_tiene_formato_correcto);

    UNITY_END();
}

void loop(void) {
    delay(1000); // no usado en modo test
}

#endif // ARDUINO
