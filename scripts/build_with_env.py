#!/usr/bin/env python3
"""
Script simple para invocar PlatformIO. La inyección de defines desde
.env/entorno la hace scripts/add_env_defines.py (extra_scripts).

Uso:
  python scripts/build_with_env.py            # compila
  python scripts/build_with_env.py upload     # compila y sube

Compatible con Windows, Linux y macOS.
"""
import os
import sys
import subprocess
import shutil
from pathlib import Path

def find_pio_command():
    """
    Busca el comando 'pio' o 'platformio' en el PATH
    Compatible con Windows, Linux y macOS
    """
    # Probar primero 'pio'
    pio_cmd = shutil.which('pio')
    if pio_cmd:
        return 'pio'
    
    # Probar 'platformio'
    platformio_cmd = shutil.which('platformio')
    if platformio_cmd:
        return 'platformio'
    
    # Si no se encuentra, retornar 'pio' por defecto (puede estar en PATH)
    return 'pio'

def build(target: str = 'run'):
    """
    Ejecuta PlatformIO con el target especificado
    """
    pio_cmd = find_pio_command()
    cmd = [pio_cmd, 'run', '-e', 'esp32dev']
    
    if target == 'upload':
        cmd.extend(['-t', 'upload'])
    
    # Mostrar comando que se ejecutará
    print("Ejecutando:", ' '.join(cmd))
    print("Directorio:", os.getcwd())
    
    try:
        # Ejecutar con shell=True en Windows para mejor compatibilidad
        shell_mode = (sys.platform == 'win32')
        result = subprocess.run(cmd, shell=shell_mode, check=False)
        return result
    except FileNotFoundError:
        print(f"\n❌ Error: No se encontró el comando '{pio_cmd}'")
        print("   Asegúrate de que PlatformIO está instalado y en el PATH")
        print("   Instala PlatformIO desde: https://platformio.org/install")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ Error al ejecutar PlatformIO: {e}")
        sys.exit(1)

if __name__ == '__main__':
    # Cambiar al directorio del script para asegurar rutas correctas
    script_dir = Path(__file__).parent.parent
    os.chdir(script_dir)
    
    if len(sys.argv) > 1 and sys.argv[1] == 'upload':
        sys.exit(build('upload').returncode)
    else:
        sys.exit(build('run').returncode)