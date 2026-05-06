# IoT MQTT TLS (ESP32)

Proyecto base para ESP32 con MQTT seguro (TLS), provisión Wi‑Fi por portal AP, actualizaciones OTA vía GitHub Actions e infraestructura completa de testing con PlatformIO y Unity.

![CI](https://github.com/alvaro-salazar/iot-mqtt-tls/actions/workflows/ci.yml/badge.svg)

## Requisitos

- **PlatformIO** CLI o extensión de VS Code
- **Python 3.11+**
- **ESP32-S3-DevKitM-1** conectado por USB
- `lcov` para reportes de cobertura (`brew install lcov` / `sudo apt install lcov`)

## Quick Start

### 1. Clonar y configurar

```bash
git clone https://github.com/alvaro-salazar/iot-mqtt-tls.git
cd iot-mqtt-tls
```

Crea el archivo `.env` en la raíz del proyecto:

```ini
COUNTRY=colombia
STATE=valle
CITY=tulua
MQTT_SERVER=mqtt.tu-dominio.com
MQTT_PORT=8883
MQTT_USER=miuser
MQTT_PASSWORD=supersecreto
WIFI_SSID=MiWiFiInicial
WIFI_PASSWORD=MiPassInicial
```

### 2. Compilar y subir al ESP32

```bash
# Opción recomendada
python scripts/build_with_env.py upload

# Opción manual
set -a && source .env && set +a
pio run -t upload
```

### 3. Configurar Wi‑Fi (primera vez)

El dispositivo crea un AP `ESP32-Setup-XXXXXX`. Conéctate y abre `http://192.168.4.1`. Ingresa SSID y contraseña — las credenciales se guardan en NVS y persisten tras OTA.

Para reconfigurar: mantén presionado **BOOT (GPIO0)** durante 3+ segundos al encender.

---

## Testing

El proyecto incluye una pirámide de testing completa con 57 tests automatizados.

### Correr los tests

```bash
# Unit tests — lógica pura, sin hardware (~2 s)
./run_tests.sh unit

# Integration tests — ESP32-S3 conectado (~35 s)
./run_tests.sh hardware

# Pipeline completo
./run_tests.sh all

# Cobertura de código (genera coverage_html/index.html)
./run_tests.sh coverage
```

### Suites

| Suite | Entorno | Tests | Qué verifica |
|---|---|---|---|
| `test_unit` | native (sin hardware) | 38 | JSON payload, alertas, SSID, MAC, timing, overflow de millis |
| `test_hardware` | ESP32-S3-DevKitM-1 | 19 | NVS, heap libre para TLS, CPU freq, WiFi radio |

### Cobertura

```bash
pio test -e native-coverage
bash scripts/coverage.sh
# Abre coverage_html/index.html en el navegador
```

Resultado actual: **100 % líneas, 100 % funciones**.

---

## CI/CD Pipeline

Un solo workflow (`.github/workflows/ci.yml`) con tres jobs. Cada job tiene su propia regla de disparo:

```
unit-tests ──────────────────────────────────────────────────► siempre (en main, develop, PRs a main, tags)
    │
    └──► hardware-tests ──────────────────────────────────────► solo en PRs a main (best-effort)
    │
    └──► build-and-publish ──────────────────────────────────► solo si unit-tests pasó + (main o tag)
```

### ¿Cuándo se dispara cada job?

| Evento | `unit-tests` | `hardware-tests` | `build-and-publish` (OTA) |
|---|---|---|---|
| Push a rama `feature/**` | — no corre | — no corre | — no corre |
| Push a `develop` | ✓ corre | — omitido | — omitido (no es main) |
| PR a `main` | ✓ corre | ✓ best-effort con runner self-hosted | — omitido (es PR, no push) |
| Push a `main` (merge) | ✓ corre | — omitido | ✓ si unit-tests pasó |
| Tag `v*.*.*` | ✓ corre | — omitido | ✓ si unit-tests pasó |

**Reglas clave:**

- `unit-tests` es el único quality gate obligatorio. Si falla, nada más corre.
- `hardware-tests` corre solo en PRs a `main` porque requiere un runner self-hosted con ESP32. Si el runner no está disponible, `continue-on-error: true` evita que bloquee el merge.
- `build-and-publish` usa `always()` en su condición `if` para que GitHub Actions evalúe la condición incluso cuando `hardware-tests` está en estado `skipped` (comportamiento por defecto de GitHub: un job con `needs` skipped se omite a menos que use `always()`).
- El OTA deploy **nunca corre sin que los unit tests hayan pasado**.

### Configurar GitHub Secrets

Para que el OTA deploy funcione, configura estos secrets en tu repo → Settings → Secrets:

| Secret | Descripción |
|---|---|
| `MQTT_SERVER` | Host del broker MQTT |
| `MQTT_PORT` | Puerto (normalmente 8883) |
| `MQTT_USER` / `MQTT_PASSWORD` | Credenciales MQTT |
| `WIFI_SSID` / `WIFI_PASSWORD` | Red WiFi de producción |
| `COUNTRY` / `STATE` / `CITY` | Metadatos del certificado TLS |
| `S3_BUCKET_NAME` | Bucket S3 para el firmware |
| `AWS_ACCESS_KEY_ID` / `AWS_SECRET_ACCESS_KEY` / `AWS_REGION` | Credenciales AWS |

### Runner self-hosted (hardware tests en CI)

```bash
# Instalar PlatformIO en venv (requerido en Raspberry Pi OS 12+ / Ubuntu 24.04+)
python3 -m venv ~/.platformio/penv
~/.platformio/penv/bin/pip install platformio

# Dar permisos al runner para el puerto serie
sudo usermod -aG dialout $USER

# Registrar el runner en GitHub → Settings → Actions → Runners
# Luego:
./run.sh
```

El runner debe tener el label `ARM64` (o el que hayas configurado en `ci.yml`).

---

## OTA Deploy

Para enviar una actualización a los dispositivos en campo:

```bash
git tag v1.2.0
git push origin v1.2.0
```

GitHub Actions compilará el firmware, lo subirá a S3 como `firmware_v1.2.0.bin` y publicará el mensaje MQTT al tópico OTA (`dispositivo/device1/ota`). Los dispositivos suscritos actualizarán automáticamente.

---

## Estructura del proyecto

```
├── src/
│   ├── main.cpp          # Punto de entrada
│   ├── libiot.*          # Cliente MQTT con TLS
│   ├── libwifi.*         # Gestión Wi‑Fi
│   ├── libota.*          # Actualizaciones OTA
│   ├── libprovision.*    # Portal de configuración AP
│   ├── libstorage.*      # Persistencia en NVS
│   └── secrets.cpp       # Credenciales (build_flags desde .env)
├── test/
│   ├── test_unit/        # Unit tests — entorno native
│   └── test_hardware/    # Integration tests — ESP32-S3
├── scripts/
│   ├── build_with_env.py # Build con variables de entorno
│   ├── add_env_defines.py# Inyección de ROOT_CA multilínea
│   └── coverage.sh       # Generador de reporte lcov
├── .github/workflows/
│   └── ci.yml            # Pipeline CI/CD unificado
├── platformio.ini        # 4 entornos: esp32dev, native, native-coverage, esp32-s3-devkitm-1-test
├── run_tests.sh          # Pipeline de testing local
└── .env                  # Variables de entorno (no commitear)
```

## Troubleshooting

| Problema | Solución |
|---|---|
| Portal no abre | Escribe manualmente `http://192.168.4.1` y desactiva datos móviles |
| No aparece el AP | Apaga/enciende o mantén BOOT 3+ s al encender |
| `pio run` falla con `MQTT_PORT` vacío | Carga el `.env` antes: `set -a && source .env && set +a` |
| OTA no llega | Verifica los Secrets de GitHub y que el dispositivo esté suscrito al tópico OTA |
| Runner offline | El pipeline continúa — hardware tests tienen `continue-on-error: true` |

## Documentación adicional

- [SECRETS_SETUP.md](SECRETS_SETUP.md) — Configurar `.env` y GitHub Secrets
- [WIFI_SETUP.md](WIFI_SETUP.md) — Portal de configuración Wi‑Fi
- [OTA_SETUP.md](OTA_SETUP.md) — Guía completa de actualizaciones OTA
- [WINDOWS_SETUP.md](WINDOWS_SETUP.md) — Instrucciones para Windows

## Licencia

MIT — ver [LICENSE](LICENSE).
