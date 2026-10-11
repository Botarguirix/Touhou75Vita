# Iteración 62 — Audio Worker / 01.66

## Evidencia de la 61

El runtime confirmó arena utilizable 32 MiB, reserva física 41 MiB y
alineación 9 MiB. Después de reservar el primer segmento JIT de 16 MiB
informó 60 MiB de memoria user libre. No hubo acceso fuera de arena.

El EXE completó dos LockRect/UnlockRect A8R8G8B8:

| Textura | Tamaño | Bytes | FNV1a | Alfa cero/completo/parcial |
| --- | --- | --- | --- | --- |
| ABC018 | 1024x128 | 524288 | CB0BE45E | 86534 / 0 / 44538 |
| ABC030 | 1024x512 | 2097152 | 79D51DC5 | 217088 / 307200 / 0 |

Son uploads reales ejecutados por el EXE, aún sin rasterizador/Present.
La siguiente frontera fue CreateThread, retorno 0040721F, después de 61
llamadas D3D, seis DInput y cuatro DirectSound, en 7.417304 segundos.
BG00c exportado es idéntico a su entrada del DAT original y su BMP/0
coincide píxel por píxel con la referencia, alfa incluido, 1400x900.

## Segundo hilo

El disassembly 004071C0 crea el worker 00407EF0 con parámetro/flags/stack
cero, salida de ID 0067139C, y le asigna prioridad 15. Ese worker crea
su evento en 00671394 y procesa una cola de audio; con cola inactiva,
WaitForSingleObject retorna a 004080FA con timeout 80 ms.

Se extiende el arranque acotado de workers para esa firma exacta. Contexto
independiente: ID 13, handle AB4010, stack A20000..A40000, TEB 770000.
El worker de temporización conserva ID 12, handle AB4000, stack
A00000..A20000 y TEB 760000. El puente de prioridad identifica ambos.

Durante el handoff se ejecuta la rutina original hasta su espera de un
evento realmente creado/no señalado; se guardan registros/flags/FS y el
frame de espera pendiente. El hilo principal se restaura y valida antes
de devolver el handle/ID. No se inventa un worker que nunca ejecutó.
La primera reanudación usa tiempo real transcurrido de 80 ms y devuelve
WAIT_TIMEOUT, conserva el siguiente bloqueo o la frontera no atendida.

Es un scheduler acotado de inicio: no es ejecución concurrente ni servicio
continuo de dos workers durante todo el juego. Señales, cola activa,
terminación y despacho recurrente pueden exigir nuevas extensiones; las
llamadas no implementadas siguen deteniendo el EXE explícitamente.
La música del visor sigue siendo la ruta nativa independiente, no prueba
que DirectSound del EXE reproduzca todavía sus buffers secundarios.

## Entrega / siguiente evidencia

Build iteration62-audio-worker-r1, versión 01.66, TITLE_ID T075VITA1.
VPK artifacts/iteration62/Touhou75Vita-iteration62.vpk.
Controles de visor y música conservados. Devolver iteration62.log,
iteration62-runtime.log, iteration62-watchdog.log, iteration62-bgm.log,
log.txt y foto. Revisar startup_audio_worker, worker_create_arg*, espera
80 ms, startup_thread_create_calls=2, uploads y siguiente frontera.
Si falla el handoff, conservar la evidencia y corregir el contrato.
El título original y menú interactivo siguen pendientes.

VitaSDK compiló sin advertencias. CRC ZIP, SFO 01.66 y cuatro PNG
indexados de 8 bits sin entrelazado inspeccionados.
SHA256: 9c3eab3baa06d771f9c6d617bd9e7f03ec247b34de58ea63bf91bf4526c6937b.
Validación del nuevo handoff pendiente en hardware.

