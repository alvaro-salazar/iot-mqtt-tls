#!/usr/bin/env bash
# run_tests.sh — Pipeline de testing para iot-mqtt-tls
#
# Uso:
#   ./run_tests.sh unit       → Unit tests nativos (sin hardware, ~2 s)
#   ./run_tests.sh hardware   → Integration tests en ESP32-S3 (~35 s)
#   ./run_tests.sh all        → Ambas suites en secuencia
#   ./run_tests.sh ci         → Solo unit tests (para GitHub Actions)
#   ./run_tests.sh coverage   → Unit tests + reporte de cobertura HTML
#
# Salida:
#   0 = todos los tests pasaron
#   1 = algún test falló

set -euo pipefail

# Buscar pio en el venv estándar de PlatformIO (funciona en Mac y Linux/Raspberry Pi)
PIO="$HOME/.platformio/penv/bin/pio"
# Fallback: buscar pio en PATH
if ! command -v "$PIO" &>/dev/null; then
  PIO="pio"
fi

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_header() {
  echo ""
  echo -e "${BLUE}════════════════════════════════════════════════${NC}"
  echo -e "${BLUE}  $1${NC}"
  echo -e "${BLUE}════════════════════════════════════════════════${NC}"
}

run_unit() {
  print_header "Unit Tests — lógica pura (sin hardware)"
  echo -e "${YELLOW}Entorno: native | Duración estimada: ~2 s${NC}"
  echo ""
  if "$PIO" test -e native; then
    echo -e "\n${GREEN}✓ Unit tests: PASSED${NC}"
    return 0
  else
    echo -e "\n${RED}✗ Unit tests: FAILED${NC}"
    return 1
  fi
}

run_coverage() {
  print_header "Coverage — unit tests con instrumentación gcov"
  echo -e "${YELLOW}Entorno: native-coverage | Requiere: lcov${NC}"
  echo ""
  if "$PIO" test -e native-coverage; then
    echo -e "\n${GREEN}✓ Tests: PASSED${NC}"
    echo -e "${YELLOW}Generando reporte de cobertura...${NC}"
    bash scripts/coverage.sh
    return 0
  else
    echo -e "\n${RED}✗ Tests: FAILED — no se genera reporte${NC}"
    return 1
  fi
}

run_hardware() {
  print_header "Integration Tests — ESP32-S3 hardware real"
  echo -e "${YELLOW}Entorno: esp32-s3-devkitm-1-test | Duración estimada: ~35 s${NC}"
  echo -e "${YELLOW}Asegúrate de tener el ESP32-S3-DevKitM-1 conectado por USB.${NC}"
  echo ""
  if "$PIO" test -e esp32-s3-devkitm-1-test; then
    echo -e "\n${GREEN}✓ Hardware tests: PASSED${NC}"
    return 0
  else
    echo -e "\n${RED}✗ Hardware tests: FAILED${NC}"
    return 1
  fi
}

case "${1:-all}" in
  unit|native)
    run_unit
    ;;
  hardware|hw)
    run_hardware
    ;;
  all)
    UNIT_STATUS=0
    HW_STATUS=0
    run_unit    || UNIT_STATUS=1
    run_hardware || HW_STATUS=1
    echo ""
    print_header "Resumen final"
    [ $UNIT_STATUS -eq 0 ] && echo -e "${GREEN}  Unit tests:     PASSED${NC}" \
                           || echo -e "${RED}  Unit tests:     FAILED${NC}"
    [ $HW_STATUS   -eq 0 ] && echo -e "${GREEN}  Hardware tests: PASSED${NC}" \
                           || echo -e "${RED}  Hardware tests: FAILED${NC}"
    echo ""
    [ $((UNIT_STATUS + HW_STATUS)) -eq 0 ] && exit 0 || exit 1
    ;;
  ci)
    # En CI solo corren los unit tests (no hay hardware conectado)
    run_unit
    ;;
  coverage|cov)
    run_coverage
    ;;
  *)
    echo "Uso: $0 [unit|hardware|all|ci|coverage]"
    exit 1
    ;;
esac
