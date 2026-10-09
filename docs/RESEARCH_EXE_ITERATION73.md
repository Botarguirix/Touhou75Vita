# Recursos y descompilación del EXE — iteración 73

Fecha: 2026-10-08. Se consultaron fuentes primarias y los snapshots ya guardados.

## Recursos de los proyectos anteriores

| Recurso | Qué aporta | Uso concreto |
|---|---|---|
| [N0zoM1z0/th075](https://github.com/N0zoM1z0/th075), snapshot a57b43e32b9f30dd500c730c3f78a8d897be73c5 | Mismo EXE japonés 1.11 y SHA; scripts de Ghidra con identidad del target, evidencia de funciones y auditoría DAT. | Identificar funciones/callsites y contrastar sus layouts con nuestros logs. |
| [Reconstruction Factory](https://github.com/N0zoM1z0/touhou-reconstruction-factory), snapshot 038b6bed51d273c8b3680553b6f43e1b97d19bd1 | Infraestructura de reconstrucción/análisis y registro de evidencia. | Separar funciones reconstruidas, inferidas y pendientes. |
| [TH08-Web renderer](https://github.com/N0zoM1z0/th08-web/blob/e3485ab9a9af3b5a90ce51841667bbdf5a867e5c/src/modern/linux/d3d8_compat.cpp) | DrawPrimitiveUP, FVF, blend y Present sobre SDL/GL/WebGL. | Referencia de la separación textura/render target/backbuffer/scanout. La 73 implementa código propio sobre handles x86 y display Vita. |
| [Repentogxm](https://github.com/0xl0cal/repentogxm) | Traducción estática del EXE a C/ARM, unidades agrupadas, dispatch y fronteras explícitas. | Referencia para medir y sustituir funciones costosas conservando fallback; requiere adaptar el traductor al binario TH075. |
| [vitaGL](https://github.com/Rinnegatamante/vitaGL) | Backend OpenGL sobre GXM de Vita. | Próximo candidato para acelerar gráficos después de verificar escenas/estados; no se incorporó todavía. |
| [Wine d3d8.h](https://raw.githubusercontent.com/wine-mirror/wine/master/include/d3d8.h) | Declaraciones COM/ABI D3D8. | Present slot 15, cinco words con this, cleanup 24 con retorno. |
| [apitrace](https://github.com/apitrace/apitrace) | Captura de llamadas gráficas del original. | Comparar orden, estados, texturas y frames de Windows con Vita. Ya hay herramienta local y scripts de resumen en este proyecto. |

La consulta web de TH075/Factory no devolvió sus documentos en esta pasada.
Las afirmaciones de esas dos filas se basan en los snapshots locales identificados;
no se afirma que sean el HEAD actual. TH08-Web y Repentogxm sí tienen documentación
pública consultada. La reconstrucción exacta aceptada del snapshot TH075 seguía
en 60 funciones/9883 bytes; no representa un motor completo compilable para Vita.
Véase también [la revisión 71](RESEARCH_N0ZOM1Z0_ITERATION71.md).

## Descompiladores encontrados

**Ghidra es la primera opción para este trabajo:** desensamblado, pseudocódigo,
tipos, referencias y grafos; su
[analyzeHeadless](https://github.com/NationalSecurityAgency/ghidra/blob/master/Ghidra/RuntimeScripts/support/analyzeHeadlessREADME.md)
permite analizar el EXE y exportar funciones por lotes. La
[API DecompInterface](https://ghidra.re/ghidra_docs/api/ghidra/app/decompiler/DecompInterface.html)
permite seleccionar funciones y comprobar errores/timeouts.

Se añadió tools/ghidra/ExportTH075Startup.java: verifica SHA256, image base y
x86 de 32 bits; solicita diez entradas, exporta pseudocódigo a un directorio
vacío y deja un manifest.tsv con cada éxito/fallo. No modifica nombres ni
acepta otro ejecutable. El pseudocódigo y los proyectos deben quedar en artifacts,
fuera de Git. El script está preparado, **no ejecutado ni compilado con Ghidra**.
No se encontró Ghidra en el workspace/PATH; Java local es 1.8.0_501.
La [documentación oficial actual](https://github.com/NationalSecurityAgency/ghidra)
requiere un JDK moderno (el README consultado indica JDK 25); se necesita una
distribución Ghidra/JDK compatibles para realizar la exportación.

**[RetDec](https://github.com/avast/retdec)** admite PE y x86, genera C, grafos
y reconstrucción de tipos/clases. Puede servir para comparar una función que
Ghidra interprete de forma ambigua. Su propio README indica mantenimiento
limitado y una instalación de unos 5–6 GB; por eso no lo priorizo para esta pasada.
No está instalado ni se ejecutó.

Descompilar produce pseudocódigo que hay que contrastar con instrucciones,
ABI, datos y trazas. Para ejecutar en ARM siguen siendo necesarios los servicios
Win32, los gráficos, entrada y audio. Extraer DAT aporta recursos; no sustituye
la lógica del EXE.

## Primer lote de diez funciones

| Dirección | Papel observado / análisis propuesto |
|---|---|
| 401F50 | Selección de factores de blend. |
| 4027D0 | Cierre de frame D3DX y render adicional al backbuffer. |
| 4028F0 | Present original y manejo de resultado. |
| 402970 | Variante Present con ventana destino. |
| 4029F0 | Preparación del quad y DrawPrimitiveUP. |
| 40C9A0 | Envío de imagen/recurso al renderer. |
| 425490 | Construcción/carga de logo.dat. |
| 425620 | Dibujo del logo. |
| 432360 | Quad de transición con modulación de color, capturado en la 72. |
| 432410 | Otra transición de color del original. |

Ejemplo una vez disponible una instalación compatible, desde WSL:

```sh
"$GHIDRA_HOME/support/analyzeHeadless" /ruta/workspace/artifacts/ghidra TH075 \
  -import /ruta/workspace/tmp-re/th075_reverse_engineering/th075.exe \
  -scriptPath /ruta/workspace/Touhou75Vita/tools/ghidra \
  -postScript ExportTH075Startup.java /ruta/workspace/artifacts/ghidra-startup-73 \
  -analysisTimeoutPerFile 300 -max-cpu 4
```

La próxima pasada debe recuperar CFG/callees/tipos de esas funciones, comprobar
el pseudocódigo con el desensamblado y asociar las llamadas a los nuevos logs 73.
El [listado de comprobaciones](EXE_CHECKLIST_ITERATION73.md) define los criterios
de ejecución y mantiene separado lo validado en PC de lo observado en Vita.
