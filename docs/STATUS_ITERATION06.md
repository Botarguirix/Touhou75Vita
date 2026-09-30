# Iteración 06 — diagnóstico del juego base y el parche de traducción

## Objetivo

Inspeccionar por separado el ejecutable japonés del juego y el parche inglés en la Vita, sin ejecutar el código de ninguno.

## Cambio preparado en el espacio de trabajo

- El diagnóstico ahora busca `ux0:data/TH075Vita/TH075.exe` y `ux0:data/TH075Vita/TH075E.exe`.
- Escribe resultados separados con prefijos `game_` y `english_patch_` en `ux0:data/TH075Vita/iteration06.log`.
- Incluye `build_id=iteration06-dual-pe-probe-r1`; el título de LiveArea y el nombre del artefacto también identifican la Iteración 06.
- Registra tamaño, arquitectura, secciones, entry RVA, base preferida y subsistema.
- Distingue un archivo ausente (`ENOENT`) de otro error de apertura (`file_open_error` y `open_errno`).
- Sigue siendo una inspección PE: no mapea las secciones, no resuelve imports y no ejecuta el entry point.

## Huellas proporcionadas

- Japonés, `TH075.exe`: `BD441E99075436E8DCAD26F86FFCF5E6AAC4F58B0ED3EE7442E4CB39D8E22C98`.
- Inglés, `TH075E.exe`: `C8313228A98134B5CEB75027D77B302E4271670B17F6B52593A1D41679834D0E`.

La huella japonesa coincide con el ejecutable incluido en el paquete de análisis. Según el usuario, `TH075E.exe` es un parche que requiere el juego japonés; no debe tratarse como un segundo ejecutable completo del juego.

## Log recibido para la edición inglesa

El usuario probó el parche renombrándolo temporalmente a `TH075.exe`, el único nombre que reconoce el VPK anterior. El log informa:

- Tamaño: 9.728 bytes; máquina I386; formato PE32; seis secciones.
- Entry RVA `0x00001240`, VA `0x00401240`; base preferida `0x00400000`; subsistema 2.
- `recognized_x86_pe32`, con `execution=not_attempted`.

La japonesa analizada mide 2.576.384 bytes y tiene cinco secciones. El usuario confirma que el archivo de 9.728 bytes es un parche para ejecutar la versión traducida. Todavía hay que inspeccionar qué cambia y qué comportamiento requiere. El log no ejecutó el parche ni el juego.

## Validación pendiente

Este cambio local aún no se ha compilado ni empaquetado. Cuando haya un VPK nuevo, colocar el juego japonés como `TH075.exe` y el parche como `TH075E.exe` en `ux0:data/TH075Vita/`, ejecutar el diagnóstico una vez y compartir `iteration06.log`. El VPK de iteraciones anteriores solo inspecciona el archivo llamado `TH075.exe` y genera `iteration01.log`.
