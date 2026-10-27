# Reproductor de audio sin distracciones

Proyecto universitario individual para diseñar un reproductor portátil de música local, centrado en escuchar una biblioteca propia sin internet, streaming ni dependencia de un teléfono. El objetivo es ofrecer reproducción directa desde microSD y una interacción física sencilla, reduciendo interrupciones y funciones ajenas a la escucha.

## Objetivo y alcance

El producto previsto reproducirá archivos locales desde una tarjeta microSD. La experiencia propuesta usa botones físicos, una pantalla OLED y salida por jack de 3,5 mm. La reproducción MP3 en el dispositivo final es un requisito del producto, pero todavía no está implementada ni validada.

## Arquitectura objetivo

La arquitectura de referencia actualmente propuesta es:

- **Procesamiento:** módulo ESP32-S3-WROOM-1-N16R8.
- **Audio:** códec ES8388 conectado mediante I²S y salida analógica a jack de 3,5 mm.
- **Biblioteca:** microSDHC con sistema FAT32, conectada por SPI.
- **Interfaz:** OLED SSD1306 y cinco botones físicos.
- **Alimentación:** batería Li-Po, cargador/gestor MCP73871 y convertidor TPS63802.
- **USB:** conector USB-C; se contempla USB MSC para acceso a la tarjeta.
- **Bluetooth:** módulo BM83 como función condicionada al resultado de una prueba de concepto específica.

Estos elementos describen la arquitectura de producto propuesta. Aún deben verificarse interfaces, presupuesto eléctrico, selección de componentes y detalles de diseño. No hay evidencia aquí de una PCB fabricada ni de un prototipo físico funcional.

## Estado de desarrollo y evidencia

La primera etapa de firmware se desarrolló para la placa ESP32-S31-Korvo-1 de Espressif con ESP-IDF 6.1 y BSP 1.0.1. Esa plataforma es una herramienta temporal para validar parcialmente el flujo de almacenamiento; no representa el hardware final.

La PoC actual monta la microSD a través del BSP y explora el directorio `MUSIC` para formar un catálogo ordenado de nombres `.mp3`. La lógica del catálogo tiene pruebas host. El firmware compiló para el target preliminar ESP32-S31. **La compilación y las pruebas host no verifican el montaje de la tarjeta en la placa.** La placa física y el hardware final aún requieren validación.

No hay todavía reproducción MP3, PCM o WAV, ni una demostración de audio operativo. USB MSC, pantalla, controles y Bluetooth tampoco están implementados en esta etapa.

### Clasificación de decisiones y evidencia

- **Requisitos del producto:** música local desde microSD, uso sin internet ni teléfono y manejo físico directo.
- **Decisiones de diseño propuestas:** ESP32-S3, ES8388/I²S, microSDHC/SPI, OLED, botones y cadena de alimentación indicada arriba.
- **Hipótesis pendientes:** compatibilidad e integración eléctrica de la arquitectura, calidad de audio, autonomía, desempeño de USB MSC y utilidad de BM83.
- **Validado experimentalmente hasta ahora:** pruebas de catálogo en PC y compilación del firmware para ESP32-S31. No se ha validado funcionamiento físico.

## Organización

- `poc/esp32-s31-korvo-1/`: firmware, pruebas host, scripts y documentación de la PoC Korvo.
- `docs/`: documentación general del proyecto cuando existan artefactos revisados y vigentes.

La documentación específica de estado y limitaciones de la PoC está en `poc/esp32-s31-korvo-1/docs/POC_STATUS.md`.

## Próximos hitos técnicos

1. Validar físicamente el montaje SD y el catálogo en ESP32-S31-Korvo-1.
2. Probar generación y salida PCM en la plataforma de PoC; luego implementar y validar lectura/reproducción WAV PCM.
3. Evaluar e integrar decodificación MP3 después de establecer el camino PCM/WAV.
4. Definir y verificar interfaces, alimentación y restricciones del hardware ESP32-S3/ES8388/microSD SPI.
5. Implementar y probar por separado pantalla, botones, USB MSC y, si la PoC lo justifica, Bluetooth BM83.
6. Construir y validar el prototipo de hardware final antes de declarar operativo el producto.

## Entorno de la PoC

Consulte `poc/esp32-s31-korvo-1/README.md` para requisitos y comandos de compilación host y ESP-IDF. La prueba de concepto requiere ESP-IDF 6.1 y `--preview` para el target ESP32-S31.
