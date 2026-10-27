# Reproductor portátil de música local

Proyecto universitario para diseñar un reproductor portátil que permita escuchar una biblioteca propia sin internet, streaming ni dependencia de un teléfono. La prioridad es una reproducción local fiable y una interacción física sencilla.

## Objetivos y funciones previstas

El producto deberá explorar una tarjeta microSD, organizar y reproducir archivos de música, mostrar información básica y permitir el control desde botones. También se evalúan acceso USB a la tarjeta y conectividad Bluetooth. Estas son metas de diseño; no se presentan como funciones ya disponibles.

## Arquitectura objetivo

| Área | Selección o propuesta actual | Estado |
|---|---|---|
| Procesamiento | ESP32-S3-WROOM-1-N16R8 | Seleccionado para el diseño objetivo; integración pendiente |
| Audio | ES8388 por I²S y jack de 3,5 mm | Propuesto; sin validación física |
| Almacenamiento | microSD por SPI, prevista con FAT32 | Propuesto; el montaje de la PoC usa otra interfaz |
| Interfaz | OLED SSD1306 y cinco botones físicos | Propuesta; sin implementación |
| Alimentación | Li-Po, MCP73871 y TPS63802 | Selección conceptual; presupuesto y circuito pendientes |
| USB | USB-C con USB MSC previsto | Requisito por validar e implementar |
| Bluetooth | BM83 | Sujeto a una PoC y validación adicionales |

La ESP32-S31-Korvo-1 se usa únicamente como plataforma temporal de prueba de concepto. No es el hardware final: su SoC, códec ES8389 y montaje SDMMC difieren de la arquitectura ESP32-S3, ES8388 y microSD SPI propuesta. Los resultados de compilación de la PoC no validan la electrónica final.

## Estado

La etapa actual implementa el montaje solicitado de SD en la Korvo y el inventario ordenado de archivos `.mp3` en `MUSIC`. La lógica del catálogo tiene pruebas host y el firmware cuenta con evidencia de compilación para ESP32-S31. No hay todavía validación en una placa física ni reproducción de audio. MP3, PCM y WAV, pantalla, botones, USB MSC y Bluetooth no están implementados.

Consulte el [estado técnico y la evidencia de la PoC](poc/esp32-s31-korvo-1/docs/POC_STATUS.md) y las [instrucciones para compilarla](poc/esp32-s31-korvo-1/README.md).

## Organización del repositorio

- `poc/esp32-s31-korvo-1/`: proyecto ESP-IDF independiente de la placa Korvo, sus pruebas host, herramientas y documentación.
- El hardware propio y el firmware definitivo aún no tienen artefactos versionados. Se incorporarán cuando existan diseños y código verificables, manteniéndolos separados de esta PoC.
- Las herramientas de validación actualmente viven junto a la PoC que las usa; no hay una colección de herramientas generales independiente.

## Próximos hitos

1. Validar físicamente montaje SD y catálogo en la ESP32-S31-Korvo-1.
2. Definir y probar el camino de audio en la PoC como etapa posterior, sin asumir que sustituye la validación del ES8388 final.
3. Completar esquemático, interfaces y presupuesto eléctrico del hardware propio y fabricar un prototipo.
4. Implementar y validar el firmware definitivo sobre la plataforma seleccionada.
5. Evaluar USB MSC y, por separado, la pertinencia del BM83.

## Licencia

El proyecto se distribuye bajo GNU GPL versión 3. Consulte el archivo [LICENSE](LICENSE).
