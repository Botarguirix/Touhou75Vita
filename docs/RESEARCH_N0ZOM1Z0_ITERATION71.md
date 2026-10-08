# Revisión de N0zoM1z0 — iteración 71

Fecha: 2026-10-08. Se revisaron los proyectos relacionados con nuestro port
desde el [perfil público](https://github.com/N0zoM1z0), sus fuentes y snapshots.
Los documentos de terceros se trataron como evidencia del proyecto.

## Fuentes y utilidad

| Proyecto | Resultado de la revisión | Uso para TH075 Vita |
| --- | --- | --- |
| [th075](https://github.com/N0zoM1z0/th075) | HEAD a57b43e32b9f30dd500c730c3f78a8d897be73c5, igual a nuestro snapshot. El target 1.11 tiene el mismo SHA256 que nuestro EXE. | Helpers y callsites de gráficos del juego exacto; no hay un motor completo nuevo publicado. |
| [touhou-reconstruction-factory](https://github.com/N0zoM1z0/touhou-reconstruction-factory) | HEAD 038b6bed51d273c8b3680553b6f43e1b97d19bd1, igual al snapshot. Es infraestructura de reconstrucción y evidencia. | Organizar identidad de binarios, llamadas y estados; no aporta por sí misma lógica jugable de TH075. |
| [th08-web](https://github.com/N0zoM1z0/th08-web) | Snapshot e3485ab9a9af3b5a90ce51841667bbdf5a867e5c. Port de fuentes C++ a WebAssembly con renderer WebGL2 y audio; MIT. | Referencia principal para sustituir las interfaces gráficas y de sonido. Requiere backend y ABI de Vita. |
| [th105](https://github.com/N0zoM1z0/th105) | Snapshot 8ce81618207e4b746adad95e1868c094b32a411e. Target TH10.5 1.06a; fuentes retenidas no siempre verificadas para ese target. | Referencias de otro juego de lucha, con layouts, assets y lógica diferentes; no sustitución directa de TH075. |

## Qué significa el progreso de TH075

El [handoff de TH075](https://github.com/N0zoM1z0/th075/blob/a57b43e32b9f30dd500c730c3f78a8d897be73c5/docs/RE_HANDOFF.md)
declara 4254 candidatos clasificados y 97 pendientes. La reconstrucción exacta
aceptada sigue en 60 funciones / 9883 bytes, aproximadamente 0.50% del denominador
provisional de código authored revisado. Clasificar miles de funciones no equivale
a reconstruirlas ni a disponer de un juego completo compilable.

Su auditoría R268 decodifica las tablas de cuatro DAT: 215/34/122/37 entradas,
408 en total. Audita nombres, tamaños, offsets y cobertura de rangos; aclara
que no reconstruye los esquemas de los payloads. Nuestro inventario físico
de 215 entradas del DAT principal y 34 músicas concuerda con los dos primeros
conteos, pero nuestra decodificación de imágenes/audio es trabajo separado.

## Adaptadores concretos que ayudan

El renderer [d3d8_compat.cpp](https://github.com/N0zoM1z0/th08-web/blob/e3485ab9a9af3b5a90ce51841667bbdf5a867e5c/src/modern/linux/d3d8_compat.cpp)
lleva DrawPrimitiveUP a Draw, interpreta FVF/stride, XYZ/XYZRHW, color y UV;
prepara textura, mezcla, profundidad y alpha test. SetTexture conserva referencias
al sustituir el binding. Su backend usa SDL/GL y rutas WebGL2, con batching y VBOs.
Sirve para diseñar el equivalente Vita del quad de 28 bytes observado en TH075.

El audio en [linux_compat.cpp](https://github.com/N0zoM1z0/th08-web/blob/e3485ab9a9af3b5a90ce51841667bbdf5a867e5c/src/modern/linux/linux_compat.cpp)
incluye buffers DirectSound, reproducción y AudioCallback. Ayuda a diseñar el
mixer del EXE, todavía distinto de nuestra música nativa del visor.

Ambos adaptadores usan objetos y punteros nativos de su juego compilado. Nuestro
bridge recibe handles y direcciones x86 guest: hay que conservar esa traducción,
sus límites y su ownership. Los DAT de TH08 usan otro parser y no son compatibles
con el formato TH75. La licencia MIT permite adaptar código con sus avisos;
esta iteración no copia esos adaptadores dentro del port.

## Orden de integración

1. SetRect y captura del primer DrawPrimitiveUP del EXE: preparado en la 71.
2. Contrastar geometría, textura y estados capturados con el backend necesario.
3. Implementar el primer dibujo en Vita, conservando fronteras para formatos no soportados.
4. Restaurar destinos y presentar el frame; comprobar la pantalla original.
5. Integrar entrada y mixer/scheduler para una escena interactiva y un combate.

La evidencia de ejecución de TH08-Web corresponde a ese proyecto y su plataforma;
no constituye una prueba de rendimiento ni compatibilidad de TH075 en Vita.
