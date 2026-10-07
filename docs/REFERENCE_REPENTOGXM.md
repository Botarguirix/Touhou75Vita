# Referencias aplicadas — iteración 53

Revisión del repositorio [repentogxm](https://github.com/0xl0cal/repentogxm),
commit `2629b331937c1a8e8c7553d62e087ce182c5b3d1`. Se revisó código,
además del README. Sus resultados publicados sobre Isaac no constituyen
una medición del rendimiento de Touhou en nuestra Vita.

## Lo integrado

`recomp/runtime/kage_vita_input.c` y `host_vita_xinput.c` separan la captura
nativa del mando de las interfaces que consumen su estado. La 53 aplica
ese patrón mediante una implementación nueva para el contrato DirectInput8
del EXE de Touhou: una captura con `sceCtrlPeekBufferPositive`, adaptación
a un teclado de 256 bytes y reutilización de la captura entre Poll y
GetDeviceState. No se copiaron código, direcciones ni funciones de Isaac.

Las interfaces y GUID se contrastaron con
[Wine dinput.h](https://raw.githubusercontent.com/wine-mirror/wine/master/include/dinput.h)
y con las llamadas del EXE original. Este puente está compilado; falta
confirmar qué métodos alcanza el juego en la Vita y si acepta el teclado.

## Qué sirve para los siguientes pasos

| Referencia revisada | Aplicación a Touhou | Trabajo pendiente |
| --- | --- | --- |
| `recomp/runtime/gl_bridge.h`, `gl_vita_backend.h` | Separar punteros y ABI de 32 bits del backend nativo; observar estados y llamadas de dibujo | Traducir Direct3D8 a un renderer Vita: textura, FVF, blend, render targets, Draw y Present |
| `recomp/runtime/host_vita_audio.h` | Adaptación de audio a un backend nativo | El puente de Isaac recibe OpenAL; Touhou necesita contratos COM de DirectSound antes del backend de audio |
| `recomp/gen_all.py` | Referencia de traducción estática x86 a C y diagnóstico de funciones sin traducción | Retarget a TH075, descubrir límites de funciones y verificar ABI, x87, SEH y callbacks; no reemplazar el runtime actual sin medir |
| `recomp/vita/CMakeLists.txt` | Ejemplo de integración VitaSDK, bibliotecas nativas y presupuestos de memoria | Evaluar dependencias y compatibilidad ABI antes de enlazarlas |
| `tools/isaac_vita_sync.py` | Transferencias verificadas por hash para un futuro despliegue | Secundario respecto al primer frame del juego |

Los descifradores de archivos de Isaac no decodifican automáticamente los
DAT de Touhou. Conservamos la extracción ya contrastada con el original:
286 entradas verificadas y 225 de 226 uploads comparados con recursos.
La prioridad es conducir esas imágenes a través de las llamadas gráficas
del EXE, no aumentar el contador de pruebas sintéticas.

## Procedencia y licencias

El código propio de repentogxm usa GPL-2.0-or-later. Sus dependencias tienen
licencias independientes: consultar
[THIRD_PARTY.md](https://github.com/0xl0cal/repentogxm/blob/2629b331937c1a8e8c7553d62e087ce182c5b3d1/THIRD_PARTY.md).
La 53 incorpora patrones de diseño mediante código nuevo; no añade fuentes,
binarios ni recursos de Isaac, ni enlaza vitaGL/OpenAL nuevos. Cualquier
incorporación posterior de fuentes deberá mantener su licencia y avisos.
