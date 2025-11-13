Import("env")

from pathlib import Path
import os

# Variables que aceptamos
KNOWN = [
    'COUNTRY','STATE','CITY',
    'MQTT_SERVER','MQTT_PORT','MQTT_USER','MQTT_PASSWORD',
    'WIFI_SSID','WIFI_PASSWORD','ROOT_CA'
]


def load_dotenv(dotenv_path: Path):
    """
    Carga variables desde archivo .env
    Compatible con Windows, Linux y macOS
    """
    vals = {}
    if not dotenv_path.exists():
        return vals
    
    try:
        # Leer con encoding UTF-8 y manejar diferentes tipos de fin de línea
        content = dotenv_path.read_text(encoding="utf-8")
        # Normalizar fin de líneas (Windows \r\n -> \n)
        content = content.replace('\r\n', '\n').replace('\r', '\n')
        
        for line in content.splitlines():
            s = line.strip()
            # Saltar líneas vacías y comentarios
            if not s or s.startswith('#'):
                continue
            # Verificar que tenga formato clave=valor
            if '=' not in s:
                continue
            
            # Dividir en clave y valor
            parts = s.split('=', 1)
            if len(parts) != 2:
                continue
                
            k = parts[0].strip()
            v = parts[1].strip()
            
            # Remover comillas si están presentes
            if (v.startswith('"') and v.endswith('"')) or (v.startswith("'") and v.endswith("'")):
                v = v[1:-1]
            
            vals[k] = v
    except Exception as e:
        print(f"[add_env_defines] Error al leer .env: {e}")
        return vals
    
    return vals


# Obtener directorio del proyecto de forma multiplataforma
root = Path(env['PROJECT_DIR'])
# Usar pathlib para manejar rutas correctamente en Windows/Linux/Mac
env_file = root / '.env'

# Cargar variables desde .env
file_vars = load_dotenv(env_file)

# Cargar variables de entorno del sistema (solo las conocidas)
os_vars = {}
for k in KNOWN:
    if k in os.environ:
        try:
            os_vars[k] = os.environ[k]
        except Exception:
            # Ignorar errores de encoding
            pass

# Merge: entorno del sistema sobreescribe al .env
vals = dict(file_vars)
vals.update(os_vars)

# Construir CPPDEFINES seguros
cpp_defines = []
for k in KNOWN:
    if k not in vals or vals[k] == '':
        continue
    v = vals[k]
    if k == 'MQTT_PORT':
        try:
            cpp_defines.append((k, int(v)))
        except ValueError:
            # Si no es entero, pásalo como string
            cpp_defines.append((k, v))
    else:
        # Pasar como string literal C (PlatformIO se encarga del quoting)
        cpp_defines.append((k, v))

if cpp_defines:
    env.Append(CPPDEFINES=cpp_defines)
    print("[add_env_defines] Defines aplicados:", [d[0] if isinstance(d, tuple) else d for d in cpp_defines])
else:
    print("[add_env_defines] Sin defines desde .env/entorno; se usarán defaults del código.")
