# Iteración 06 — diagnóstico del juego y del parche de traducción

## Resultado en hardware

**Aprobada como diagnóstico PE; no es una prueba de arranque del juego.** El usuario instaló la VPK en una PS Vita y compartió `ux0:data/TH075Vita/iteration06.log`.

- `build_id=iteration06-dual-pe-probe-r1` confirma que corrió la compilación esperada.
- El juego japonés `TH075.exe` abrió con 2.576.384 bytes y fue reconocido como PE32/I386, cinco secciones, base `0x00400000`, entry VA `0x0064232C`, subsistema 2.
- El parche `TH075E.exe` abrió con 9.728 bytes y fue reconocido como PE32/I386, seis secciones, base `0x00400000`, entry VA `0x00401240`, subsistema 2.
- Ambos resultados fueron `recognized_x86_pe32`.
- `execution=not_attempted` es el resultado previsto: esta VPK no ejecutó ninguno de los dos archivos.

## Identidad de los archivos

Los SHA-256 calculados en la copia de Windows coinciden con los hashes que proporcionó el usuario:

- `TH075.exe`: `BD441E99075436E8DCAD26F86FFCF5E6AAC4F58B0ED3EE7442E4CB39D8E22C98`.
- `TH075E.exe`: `C8313228A98134B5CEB75027D77B302E4271670B17F6B52593A1D41679834D0E`.

La VPK no calcula hashes, así que el log de Vita confirma los tamaños y campos PE, pero no la identidad criptográfica de los bytes presentes en la tarjeta.

## Alcance

El diagnóstico valida acceso a los dos archivos y sus cabeceras. No carga secciones, resuelve imports, ejecuta instrucciones x86 ni inicia el juego. `TH075E.exe` es el componente ejecutable del parche; el paquete local también incluye `th075e.dll` y `th075e.dat`, que se deben analizar junto con él. Los archivos del juego permanecen fuera de Git.

## Siguiente hito

La Iteración 07 debe auditar e integrar el cargador PE32 de WinVita y producir un informe de carga del ejecutable japonés, con secciones e imports resueltos/no resueltos. La prueba debe detenerse antes del entry point y de cualquier ejecución de código invitado. Ver `PORTING_ARCHITECTURE.md` para las condiciones de integración y licencia.
