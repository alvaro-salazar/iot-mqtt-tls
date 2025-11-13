# Configuración en Windows

Esta guía te ayuda a configurar y compilar el proyecto en Windows.

## Requisitos Previos

1. **Python 3.7 o superior**
   - Descarga desde: https://www.python.org/downloads/
   - **IMPORTANTE**: Durante la instalación, marca la opción "Add Python to PATH"

2. **PlatformIO**
   - Instala PlatformIO Core:
     ```powershell
     pip install platformio
     ```
   - O instala PlatformIO IDE (VS Code extension)

3. **Git** (opcional, para clonar el repositorio)
   - Descarga desde: https://git-scm.com/download/win

## Configuración del Proyecto

### 1. Crear archivo .env

Crea un archivo llamado `.env` en la raíz del proyecto (al mismo nivel que `platformio.ini`).

**Ejemplo de archivo `.env`:**

```env
# Configuración de ubicación
COUNTRY=colombia
STATE=valle
CITY=tulua

# Configuración MQTT
MQTT_SERVER=mqtt.alvarosalazar.freeddns.org
MQTT_PORT=8883
MQTT_USER=alvaro
MQTT_PASSWORD=supersecreto

# Configuración WiFi
WIFI_SSID=MAXELL_2.4_F2F
WIFI_PASSWORD=a1b2c3d4

# Certificado CA (puede estar en múltiples líneas)
ROOT_CA=-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
...
-----END CERTIFICATE-----
```

**⚠️ IMPORTANTE en Windows:**
- Guarda el archivo con codificación **UTF-8** (sin BOM)
- En Notepad++: Encoding → Convert to UTF-8 (sin BOM)
- En VS Code: El encoding se muestra en la barra inferior, haz clic para cambiarlo a UTF-8

### 2. Verificar que PlatformIO está instalado

Abre PowerShell o CMD y ejecuta:

```powershell
pio --version
```

Si no funciona, prueba:

```powershell
platformio --version
```

Si ninguno funciona, necesitas agregar PlatformIO al PATH o reinstalarlo.

## Compilar el Proyecto

### Opción 1: Usar el script (Recomendado)

```powershell
# Desde la raíz del proyecto
python scripts\build_with_env.py
```

Para compilar y subir al dispositivo:

```powershell
python scripts\build_with_env.py upload
```

### Opción 2: Usar PlatformIO directamente

```powershell
# Compilar
pio run -e esp32dev

# Compilar y subir
pio run -e esp32dev -t upload
```

### Opción 3: Usar VS Code con PlatformIO IDE

1. Abre VS Code
2. Abre la carpeta del proyecto
3. Haz clic en el ícono de PlatformIO en la barra lateral
4. Selecciona "Build" o "Upload"

## Solución de Problemas Comunes en Windows

### Error: "No se encontró el comando 'pio'"

**Solución:**
1. Verifica que PlatformIO está instalado:
   ```powershell
   pip show platformio
   ```

2. Si está instalado pero no funciona, agrega Python al PATH:
   - Abre "Variables de entorno" en Windows
   - Edita la variable PATH
   - Agrega: `C:\Users\TuUsuario\AppData\Local\Programs\Python\Python3x\Scripts`
   - Reinicia PowerShell/CMD

3. Reinstala PlatformIO:
   ```powershell
   pip install --upgrade platformio
   ```

### Error: "encoding" o problemas con caracteres especiales

**Solución:**
1. Abre el archivo `.env` en un editor que soporte UTF-8
2. Guarda el archivo con codificación UTF-8 (sin BOM)
3. En VS Code: Click derecho en el archivo → "Save with Encoding" → "UTF-8"

### Error: "No such file or directory: '.env'"

**Solución:**
1. Verifica que el archivo `.env` está en la raíz del proyecto (mismo nivel que `platformio.ini`)
2. Verifica que no tiene extensión oculta (no debe ser `.env.txt`)
3. En Windows Explorer, ve a "Vista" → marca "Extensiones de nombre de archivo" para ver la extensión real

### Error: Rutas con backslashes

Los scripts ahora manejan esto automáticamente, pero si ves errores:

1. Usa PowerShell en lugar de CMD (mejor manejo de rutas)
2. Ejecuta desde la raíz del proyecto:
   ```powershell
   cd C:\ruta\a\tu\proyecto
   python scripts\build_with_env.py
   ```

### Error: "Permission denied" o problemas de permisos

**Solución:**
1. Ejecuta PowerShell como Administrador (click derecho → "Ejecutar como administrador")
2. O verifica que tienes permisos de escritura en la carpeta del proyecto

### Error al leer el archivo .env

**Solución:**
1. Verifica que el archivo `.env` no tiene caracteres especiales problemáticos
2. Asegúrate de que cada línea tiene el formato: `CLAVE=valor`
3. No dejes espacios alrededor del `=`
4. Si el valor tiene espacios, usa comillas: `WIFI_SSID="Mi Red WiFi"`

## Verificación Rápida

Para verificar que todo está configurado correctamente:

```powershell
# 1. Verificar Python
python --version

# 2. Verificar PlatformIO
pio --version

# 3. Verificar que el archivo .env existe
dir .env

# 4. Compilar (debería mostrar los defines aplicados)
python scripts\build_with_env.py
```

Deberías ver algo como:
```
[add_env_defines] Defines aplicados: ['COUNTRY', 'STATE', 'CITY', ...]
```

## Diferencias entre Windows y Mac/Linux

| Aspecto | Windows | Mac/Linux |
|---------|---------|-----------|
| Separador de rutas | `\` | `/` |
| Scripts | `scripts\build_with_env.py` | `scripts/build_with_env.py` |
| Comando Python | `python` o `python3` | `python3` |
| Encoding por defecto | Puede variar | UTF-8 |
| Fin de línea | `\r\n` (CRLF) | `\n` (LF) |

Los scripts ahora manejan estas diferencias automáticamente.

## Notas Adicionales

1. **Espacios en rutas**: Si tu proyecto está en una ruta con espacios (ej: `C:\Mis Proyectos\IoT`), los scripts deberían manejarlo, pero es mejor evitar espacios en las rutas.

2. **Antivirus**: Algunos antivirus pueden bloquear PlatformIO. Agrega la carpeta del proyecto a las exclusiones si tienes problemas.

3. **Firewall**: El primer uso de PlatformIO puede requerir permisos del firewall de Windows.

4. **Variables de entorno del sistema**: También puedes configurar las variables en Windows:
   - Panel de Control → Sistema → Variables de entorno
   - Agrega variables como `MQTT_USER`, `WIFI_SSID`, etc.
   - Estas tienen prioridad sobre el archivo `.env`

## Soporte

Si sigues teniendo problemas:

1. Verifica que estás usando la versión más reciente de los scripts
2. Revisa los logs de error completos
3. Verifica que el archivo `.env` está bien formateado
4. Prueba ejecutar PlatformIO directamente sin el script para aislar el problema
