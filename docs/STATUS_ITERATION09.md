# Iteración 09 — identidad, memoria, archivos y pantalla

Estado: r1 probada en hardware: identidad, CPU, puente, heap y archivo pasaron; la presentación falló y el usuario observó una pantalla negra. Corrección r2 preparada localmente, pendiente de compilación y consola. No se ejecuta el entry point, callbacks TLS ni código comercial del juego.

## Resultado r1 y corrección r2

Los logs aportados confirman `result=identity_cpu_iat_heap_file_passed`, hash calculado idéntico al japonés esperado, diez llamadas de servicios, ocupación del heap cero y cabecera comparada correctamente. Se conserva un extracto en docs/hardware/iteration09-r1/result-excerpt.txt.

La pantalla falla con `screen_present_rc=0x80290006`. El [header oficial de VitaSDK](https://github.com/vitasdk/vita-headers/blob/master/include/psp2common/display.h) identifica ese valor como `SCE_DISPLAY_ERROR_INVALID_UPDATETIMING`. El código r1 pidió SCE_DISPLAY_SETBUF_IMMEDIATE. r2 cambia la presentación y desconexión a SCE_DISPLAY_SETBUF_NEXTFRAME, espera vblank y consulta el framebuffer activo, registrando tanto el resultado como la coincidencia de dirección, pitch, tamaño y formato. La presentación visible aún debe confirmarse con hardware y foto. Si desconectar el buffer falla, su liberación se difiere al cierre del proceso.

## Comprobaciones

1. SHA-256: comprobaciones internas con vectores vacío, `abc` y otro de 56 bytes. Se calcula el hash de todos los bytes del EXE en Vita. Debe ser `BD441E99075436E8DCAD26F86FFCF5E6AAC4F58B0ED3EE7442E4CB39D8E22C98`; una discrepancia detiene el cargador antes de procesar el PE.
2. Mapeo PE y ejecución de la rutina x86 de control.
3. SetLastError, GetLastError y GetTickCount: valores, reloj, contadores y restauración de la pila.
4. GetProcessHeap, HeapAlloc y HeapFree: se reutiliza `wx86::GuestRegion` de WinVita, con shims de diagnóstico que validan handles y opciones. Se reservan 256 bytes con HEAP_ZERO_MEMORY, se comprueban ceros, se escribe/lee `0x75` mediante instrucciones x86 y se libera. Debe quedar ocupación cero. También se comprueban doble liberación rechazada y petición demasiado grande rechazada.
5. CreateFileA, GetFileSize, ReadFile y CloseHandle: por la IAT original se abre TH075.exe, se comprueba tamaño y se comparan 64 bytes de cabecera con la lectura nativa. Un segundo cierre debe fallar.
6. Pantalla nativa de resultados: X sale; salida automática tras 120 segundos. Usa el framebuffer de VitaSDK; no implementa Direct3D 8 ni muestra imágenes del juego. Referencias: [display](https://docs.vitasdk.org/group__SceDisplayUser.html) y [controller](https://github.com/vitasdk/vita-headers/blob/master/include/psp2/ctrl.h).

## Límites

Diez imports tienen shims de diagnóstico; se esperan 147 pendientes. Esto no implica compatibilidad completa de ninguna DLL. El servicio de archivos solo admite la ruta exacta TH075.exe, OPEN_EXISTING, GENERIC_READ, FILE_ATTRIBUTE_NORMAL y lectura síncrona de hasta 4096 bytes. No hay escritura, directorios, archivos DAT, OVERLAPPED ni mapping. HeapAlloc admite solo el heap de diagnóstico y HEAP_ZERO_MEMORY. No hay HeapReAlloc, múltiples heaps ni VirtualAlloc. LastError sigue limitado al hilo de diagnóstico. TEB/FS/TLS, excepciones y callbacks del juego quedan pendientes.

## Memoria

- PE: base `0x00400000`, final anterior a `0x00700000`.
- CPU/IAT: `0x00700000`, `0x00710000`, `0x00711000`.
- Servicios: página `0x00712000`. Se descarta la traducción antes de cada sustitución y se comprueban los bytes leídos de vuelta.
- Buffers: `0x00740000..0x00744000`.
- Pila: `0x00800000..0x00A00000`.
- Trampas: `0x00B00000..0x00C00000`.
- Heap: `0x00C00000..0x00D00000`, 1 MiB.

Todo cabe en el espacio invitado de 16 MiB probado previamente. La pantalla usa un framebuffer nativo separado en CDRAM. Un resultado de servicios exitoso no demuestra que la pantalla se vea: su presentación tiene resultado independiente y debe confirmarse con una foto.

## Prueba

1. Commit y push en codex/d2vita-runtime-review o main. Esperar el workflow exitoso del commit nuevo.
2. Descargar **Touhou75Vita-iteration09-r2-heap-file-screen-vpk**, extraer e instalar la VPK. Title ID T075VITA1; versión de paquete `01.11`.
3. Mantener el EXE japonés en ux0:data/TH075Vita/TH075.exe. No se necesitan nuevos archivos.
4. Abrir **Touhou 7.5 Vita - Iteration 09 r2 Services Test**. Esperar, fotografiar los resultados y pulsar X para salir.
5. Compartir iteration09.log, iteration09-runtime.log y la foto. Ambos logs se reinician con cada lanzamiento.

Indicadores esperados, pendientes de comprobar en hardware:

```text
build_id=iteration09-winvita-heap-file-screen-r2
sha256_selfcheck=passed
game_sha256_result=passed
game_sha256_verified_on_vita=yes
game_code_executed=no
dynarec_smoke_result=passed
import_bridge_unresolved=147
import_smoke_result=passed
heap_zero_result=passed
heap_live_bytes=0
heap_smoke_result=passed
file_header_compare=passed
file_smoke_result=passed
result=identity_cpu_iat_heap_file_passed
screen_present_rc=0x00000000
screen_present_sync=nextframe
screen_active_matches=yes
screen_result=presented
```

Después: preparar el entorno TEB/FS/TLS y el estado del proceso, auditar los imports del inicio original y añadir una ejecución controlada que identifique la primera API o instrucción bloqueante. Aún no hay una estimación fiable del número de iteraciones hasta el menú del juego.
