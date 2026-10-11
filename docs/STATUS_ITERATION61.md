# Iteración 61 — Texture Upload / 01.65

## Evidencia física de la 60

El EXE ejecutó las consultas del módulo D3D y continuó SystemDataInit.
Creó una textura managed A8R8G8B8 de 1024x128, pitch 4096, 524288 bytes,
obtuvo su superficie/descriptor y llamó LockRect nivel cero, rect NULL,
flags cero, output 009FEB7C. Se atendieron 41 llamadas D3D, seis DInput y
cuatro DirectSound antes del fallo. Todavía no se presentó un frame del EXE.

El runtime identifica la causa exacta:
`C3: acces hors arene write va=01400000 n=524288 (span=01000000)`.
La zona de staging de texturas 01400000..01800000 quedaba fuera de la
arena efectiva de 16 MiB. Cpu::map registraba protecciones sin validar
ese límite; Cpu::write rechazó la operación antes de la copia.

WX86_ARENA=02000000 no producía 32 MiB útiles: cpu_box86.cpp descuenta
16 MiB de alineación del tamaño solicitado antes de calcular la reserva.
El runtime registró span 16 Mo, reserve 25 Mo en esta sesión.

La música registró 907 bloques, 42.395154 s, fill_max 168179 us y tres
preparaciones tardías. Drenaje y liberación correctos. No es una medida
directa de underruns ni una comparación controlada con sesiones anteriores.

## Corrección

Se solicita WX86_ARENA=03000000: 48 MiB según la convención existente,
para un mínimo de 32 MiB utilizables más la alineación física necesaria.
La reserva física final puede ser menor de 48 MiB porque el runtime mide
el slack. Se valida hostptr(0,32 MiB) antes de mapear/ejecutar el EXE y se
registra guest_arena_span_validation. Si el tamaño no cabe se falla antes
de ejecutar, manteniendo el guard de memoria.

D3D comprueba que todo el staging de 4 MiB cabe en la arena antes de crear
el dispositivo. LockRect ahora distingue en log fallo de capacidad,
copia de staging y salida D3DLOCKED_RECT. El contrato y los píxeles no
cambian: escribe pitch/pBits, devuelve una dirección guest, y UnlockRect
recupera los bytes y registra hash/alfa. La subida real sigue pendiente
de comprobación física; no se ha implementado rasterización ni Present.

El coste esperado es 16 MiB adicionales de arena útil. Debe comprobarse
la memoria libre/JIT en hardware y el avance después de LockRect. No se
reduce el presupuesto gráfico ni se cambian instrucciones del EXE.

## Paquete y próxima captura

Build iteration61-texture-upload-r1, versión 01.65, TITLE_ID T075VITA1.
VPK artifacts/iteration61/Touhou75Vita-iteration61.vpk.
Se conserva el icono original, la captura del juego de LiveArea de la 60
y el fondo con badge actualizado. La captura PC de resolución completa
ya no estaba en su ruta, por lo que startup.png se preparó a partir del
startup.png existente de la 60.

Controles de visor y música conservados. Devolver iteration61.log,
iteration61-runtime.log, iteration61-watchdog.log, iteration61-bgm.log,
log.txt y foto. Revisar arena span, guest_arena_span_validation,
startup_d3d8_texture_lock/upload y la siguiente frontera. El criterio de
arranque completo sigue pendiente: un frame del EXE y menú interactivo.

VitaSDK compiló sin advertencias. CRC ZIP, SFO 01.65 y cuatro PNG
indexados de 8 bits sin entrelazado inspeccionados.
SHA256: 7e9d44c00ace7091d291f35817ae6e0ebf75c5406db6d25db4bd12b8eb79c3a4.

