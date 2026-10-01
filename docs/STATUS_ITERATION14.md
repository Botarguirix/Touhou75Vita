# Iteración 13 — heap inicial del EXE

La Iteración 12 fue validada en la Vita: el EXE japonés consumió la estructura de versión, recibió su módulo y llegó a `HeapCreate stdcall cleanup(0, 4096, 0)`. Iteración 13 implementa ese heap de forma acotada y continúa las instrucciones originales.

El servicio crea una arena invitada RW en `0x00C00000`, devuelve el handle `0x00AB0000` y atiende `HeapAlloc`, `HeapFree` y `HeapSize` básicos con limpieza stdcall y comprobación de registros. Los bloques se alinean a 16 bytes y `HEAP_ZERO_MEMORY` se respeta. Un import posterior que aún no tenga contrato detiene el recorrido para registrar la siguiente dependencia real.

La compilación local con VitaSDK 2026.08 pasó: WinVita ARMv7, CMake, enlace SELF y generación VPK. Falta publicar y validar en una sola Vita.

Resultado esperado en la pantalla: `ITERATION 13`, `EXE: HEAP SERVICE RETURNED`, `STOP: NEXT IMPORT`. Los archivos serán `iteration14.log`, `iteration14-runtime.log` e `iteration14-watchdog.log` en `ux0:data/TH075Vita/`.

## Publicación y prueba

1. Commit: `feat: continue TH075 startup through heap service`.
2. Esperar Actions en verde y descargar `Touhou75Vita-iteration14-startup-heap-vpk`.
3. Instalar en la misma Vita, conservando el EXE japonés verificado.
4. Fotografiar la pantalla, pulsar X y compartir los tres logs. Los otros dispositivos siguen reservados hasta confirmar el arranque.

Esta iteración no confirma todavía el menú ni el arranque completo. Si pasa, la siguiente implementación se elegirá con base en el import donde se detenga el EXE.
