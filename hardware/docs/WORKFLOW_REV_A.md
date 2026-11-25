# PCB académica Rev A | Plan de trabajo

Estado (2026-11-15): **diseño de la Rev A congelado (ERC 0 errores, DRC 0 violaciones), revisado por el profesor
(positivo) y entregado con la etiqueta `rev-a`.**

## G0 | Configuración del repositorio y de Altium
- [x] Proyecto en `hardware/altium/MP3_Player/MP3_Player.PrjPcb` con 7 hojas de esquemático y la placa
- [x] Librería propia `MP3_RevA` enlazada al proyecto
- [x] Librería del profesor usada en local y excluida de Git hasta tener permiso de redistribución
- [x] Archivos nativos del proyecto en `main` (PR #1 esquemáticos, PR #2 PCB, PR #3–#5 correcciones, documentación y logo)

## G1 | Librería y evidencia de componentes
- [x] Símbolos y huellas propios revisados contra los datasheets, pin ↔ pad (ver `altium/libraries/project/README.md`)
- [x] Preferencia por piezas de la librería del curso; piezas propias justificadas (`MANUAL_CONEXIONES_RevA.md`, sección 7)
- [x] Correcciones de huellas encontradas por el DRC: agujeros de anclaje de la microSD y del jack (sin metalizar), excepción de separación en el USB-C
- [x] BOM exportada con la agrupación que pide el profesor (los 4 pasivos 0603 propios comparten una línea con fabricante `TBD`; los puntos de prueba figuran como KOA RCWCTE)
- [ ] Huellas confirmadas contra las piezas físicas antes de soldar

## G2 | Arquitectura del sistema y distribución
- [x] Interfaces y tabla de GPIO (USB, I²S, SPI, I²C, botones, ADC): `MANUAL_CONEXIONES_RevA.md`, sección 3
- [x] Solo dos capas (Top y Bottom), Bottom principalmente plano de GND
- [x] Contorno de 65 × 90.17 mm (según el perfil del Gerber); límite del curso 100 × 100 mm
- [x] Antena fuera del borde de la placa, como indicó el profesor

## G3 | Esquemático por bloques
- [x] Energía: cargador TP4056, selección de fuente AN1149 (AP2301 + Schottky), LDO AP2112K, divisores para el ADC
- [x] Núcleo del MCU: desacople, RC de reset en EN, botones BOOT/RESET, pines de arranque y de PSRAM libres, header UART
- [x] USB-C: pull-downs CC, protección ESD USBLC6
- [x] Almacenamiento: microSD por SPI con pull-ups y detección de tarjeta
- [x] Audio: PCM5102A (valores de la Figura 33 de TI) → filtro RC → TPA6138A2 (G = −1) → jack de 3.5 mm
- [x] Interfaz de usuario: cinco botones con pull-up, header de la OLED con pull-ups I²C
- [x] Nivel superior: puertos jerárquicos y entradas de hoja
- [x] ERC: 0 errores (las advertencias "no driving source" restantes se suprimieron con No ERC en redes revisadas)
- [x] Presupuesto de potencia y autonomía (`POWER_BUDGET_REV_A.md`; validación con medidas pendiente)

## G4 | Diseño de la placa
- [x] Reglas de JLCPCB para 2 capas con margen para soldar a mano (ver `hardware/README.md`)
- [x] Ubicación revisada antes de rutear; primero conectores de borde, zona de la OLED y cruz de botones
- [x] Primero la alimentación (VBUS, VBAT, VSYS, +3V3), luego el par USB, redes locales, buses e interfaz
- [x] Rellenos de GND en Top y Bottom, vías de unión, sin islas de GND aisladas
- [x] Puntos de prueba en redes de alimentación, arranque y audio (colocados desde los esquemáticos, fuera de la BOM)
- [x] Serigrafía depurada (texto de 1.0 mm, trazo de 0.15 mm); etiquetas de botones y conectores (J1–J7); logo del dinosaurio en el Bottom Overlay
- [x] DRC: 0 violaciones, 0 advertencias; revisión visual en 3D
- [x] Revisión del profesor (2026-11-14, positiva)

## G5 | Entregables académicos
- [x] Proyecto nativo de Altium y librería propia
- [x] PDFs: esquemáticos (`SHC/SCH.pdf`), plano de fabricación en Draftsman, PDF 3D y modelo STEP
- [x] BOM (`BOM/BOM.xlsx`); decisiones de diseño en `MANUAL_CONEXIONES_RevA.md` y en los README
- [x] Reporte DRC exportado (0 violaciones, 0 advertencias; dentro de `GERBERS/GERBERS.zip`)
- [x] ERC: 0 errores en Altium; el curso no exige exportar el reporte
- [x] Salidas de fabricación: Gerbers y taladros NC para JLCPCB, en `hardware/outputs/MP3_entregables/` (2026-11-14)
- [x] Punto de control en Git con etiqueta de revisión (`rev-a`, tras la corrección del USB del 2026-11-15)
- [x] Entregado al profesor (Classroom, 2026-11-15)
- [x] Nota de aplicación y pitch del producto (`docs/`, 2026-11-25)

## Riesgos conocidos
- La polaridad del cable no es estándar entre baterías JST-PH: verificarla antes de conectar.
- El orden de pines de los módulos OLED cambia según el fabricante: comprar uno GND–VCC–SCL–SDA y revisar su serigrafía.
- La caída del AP2112K deja sin usar la última parte de la carga de la batería.
- El TP4056 disipa hasta ≈ 0.4 W mientras carga; el cobre a su alrededor actúa como disipador.
- El USB-C (paso de 0.5 mm) y las piezas TSSOP son las uniones más difíciles de soldar a mano.
- La antena sobresale del contorno de 65 × 90.17 mm; revisar el tamaño total si se hace una carcasa.
- En la Rev A los nombres de red de los botones no coinciden con la serigrafía; la función de cada GPIO se toma del manual de conexiones.
- Pasar ERC y DRC no es validación funcional: todavía no se ha fabricado ni probado nada.

## Disciplina de revisión
Necesidad → requisito numérico/eléctrico → alternativas → selección verificada → implementación en Altium → revisiones ERC/DRC → estado (aprobado, provisional o pendiente).
No editar el firmware de la PoC durante este flujo de trabajo.
