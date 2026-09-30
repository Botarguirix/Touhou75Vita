# Iteración 07 — prueba de carga PE32 sin ejecución

## Estado

Plan de implementación. Iteración 06 confirmó en una PS Vita que el ejecutable japonés y el lanzador del parche son PE32/I386; todavía no existe una prueba que cargue el juego.

## Objetivo

Demostrar que el cargador genérico puede leer la imagen PE32 japonesa de Touhou 7.5 y reportar el estado de sus secciones e imports. No iniciar el entry point, no ejecutar callbacks TLS y no ejecutar el parche inglés.

## Trabajo necesario

1. Fijar una revisión concreta de WinVita y revisar el árbol de dependencias, avisos de terceros y condiciones de redistribución antes de añadirlo al repositorio.
2. Hacer un programa de host que use el analizador PE32 del motor para inventariar módulos e imports de `TH075.exe`, `TH075E.exe` y `th075e.dll`. No subir los binarios ni el DAT.
3. Crear una app Vita que cargue el PE japonés en el espacio de direcciones invitado, respetando la base preferida `0x00400000`, y escriba un log de secciones e imports.
4. Asegurar por diseño que la app no llama al entry point ni arranca el scheduler/dynarec. El resultado debe distinguir módulos cargados, símbolos resueltos y fallos concretos.
5. Documentar el VPK y validar en la PS Vita que se genera el nuevo log y que se detiene antes de ejecutar código x86.

## Criterios de aceptación

- La revisión de WinVita y sus componentes está fijada y documentada.
- El análisis usa el ejecutable japonés identificado por SHA-256 `BD441E99075436E8DCAD26F86FFCF5E6AAC4F58B0ED3EE7442E4CB39D8E22C98`.
- El informe deja claro si se mapearon cabeceras y secciones, y enumera imports resueltos/no resueltos.
- No se invoca el entry point, TLS callbacks, `TH075E.exe` ni el código del juego.
- La VPK produce un log nuevo en la Vita con un `build_id` único.

## Límite conocido

WinVita aporta un cargador PE32, un puente de imports y un dynarec ARMv7 derivado de Box86, pero no aporta shims Win32 listos para Touhou. Esta iteración solo prueba la frontera de carga; no incluye renderizado Direct3D 8, audio, controles, ni arranque jugable. D2Vita separa ese motor genérico del código específico de Diablo II; Touhou debe mantener la misma separación.
