#!/usr/bin/env bash
# coverage.sh — Genera reporte de cobertura de unit tests
#
# Uso:
#   bash scripts/coverage.sh           → genera HTML en coverage_html/
#   bash scripts/coverage.sh --text    → imprime resumen en terminal
#
# Requiere: lcov (sudo apt install lcov  /  brew install lcov)
#
# Flujo:
#   1. pio test -e native-coverage   (instrumenta con gcov)
#   2. bash scripts/coverage.sh      (genera el reporte)

set -euo pipefail

BUILD_DIR=".pio/build/native-coverage"
OUTPUT_DIR="coverage_html"
INFO_FILE="coverage.info"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Verificar dependencias
if ! command -v lcov &>/dev/null; then
  echo -e "${RED}Error: lcov no está instalado.${NC}"
  echo "  macOS:  brew install lcov"
  echo "  Linux:  sudo apt install lcov"
  exit 1
fi

if [ ! -d "$BUILD_DIR" ]; then
  echo -e "${RED}Error: carpeta de build no encontrada: $BUILD_DIR${NC}"
  echo "Ejecuta primero: pio test -e native-coverage"
  exit 1
fi

echo -e "${BLUE}════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  Coverage Report — iot-mqtt-tls unit tests${NC}"
echo -e "${BLUE}════════════════════════════════════════════════${NC}"

# Capturar datos gcov
echo -e "\n${YELLOW}Capturando datos de cobertura...${NC}"
lcov \
  --capture \
  --directory "$BUILD_DIR" \
  --output-file "$INFO_FILE" \
  --quiet

# Excluir Unity framework del reporte
# Solo filtramos patrones que definitivamente coinciden (Unity).
# Los headers del sistema (/usr/*, /Library/*) no aparecen en el
# reporte gcov porque no se compilaron con -fprofile-arcs.
lcov \
  --remove "$INFO_FILE" \
  '*/Unity/*' \
  '*/unity*' \
  --output-file "$INFO_FILE" \
  --ignore-errors unused \
  --quiet

if [ "${1:-}" == "--text" ]; then
  echo -e "\n${YELLOW}Resumen de cobertura:${NC}\n"
  lcov --list "$INFO_FILE"
else
  # Generar HTML
  echo -e "${YELLOW}Generando reporte HTML en $OUTPUT_DIR/ ...${NC}"
  genhtml \
    "$INFO_FILE" \
    --output-directory "$OUTPUT_DIR" \
    --title "iot-mqtt-tls unit test coverage" \
    --quiet

  echo -e "\n${GREEN}✓ Reporte generado: $OUTPUT_DIR/index.html${NC}"

  # Abrir automáticamente si hay display (falla silenciosa en CI)
  if command -v open &>/dev/null; then
    open "$OUTPUT_DIR/index.html" 2>/dev/null || true
  elif command -v xdg-open &>/dev/null; then
    xdg-open "$OUTPUT_DIR/index.html" 2>/dev/null || true
  fi
fi

# Imprimir línea de resumen
echo ""
lcov --summary "$INFO_FILE" 2>&1 | grep -E "lines|functions|branches"
echo ""
