# Iteración 68 — State Blocks / 01.72

## Evidencia física de la 67

9653 llamadas principales, 2483 D3D, seis DInput, 199 DirectSound, 969 HeapAlloc,
346 operaciones adicionales de heap, dos workers, 30 reanudaciones y 13363848 us.
La ejecución llegó a CreateDepthStencilSurface D16 1024x1024 y a BeginStateBlock
(slot 52/trap BFC340). No hubo CPU fault ni límite de ejecución. Reutilización de
heap permitió atravesar la asignación que devolvía cero en la 66. El PASS mostrado
es checkpoint de startup, no boot completo. log.txt conserva LoadEffect ...OK.
BGM nativo cerró correctamente: 61 bloques y cero late_fill_blocks.

## Familia de bloques de estado

BeginStateBlock/EndStateBlock, ApplyStateBlock, CaptureStateBlock y DeleteStateBlock
se implementan juntos para el estado actualmente soportado: render states,
texture stage cero y FVF. Las setters registran valores y máscaras durante Begin,
sin cambiar el estado activo; End conserva ese subconjunto en un token. Getters
siguen consultando el activo. Apply restaura solo máscaras grabadas; Capture lee
el activo para esas mismas máscaras; Delete invalida el token. Tokens nunca se
reutilizan, máximo 64 por ejecución para evitar alias de handles obsoletos.

Begin anidado, End sin Begin y operaciones con tokens inexistentes/borrados o
mientras se graba devuelven D3DERR_INVALIDCALL. Outputs y cleanup COM se validan
por el mismo bridge. Los registros indican inicio, token, cantidad de render/stage
states y FVF; Apply/Capture/Delete registran token y operación. Máximo de tokens
es una frontera explícita. CreateStateBlock por tipo ALL/PIXEL/VERTEX, bindings de
textura, transforms, shaders y nuevos estados aún no soportados conservan frontera.
No se simulan Draw/Present ni se anuncian píxeles originales del EXE.

Referencia ABI/implementación consultada: Wine D3D8 (sin copiar código):
https://raw.githubusercontent.com/wine-mirror/wine/master/dlls/d3d8/device.c

## Paquete y captura

VitaSDK compiló sin advertencias; diff --check limpio. Inspección ZIP CRC,
SFO 01.72/T075VITA1 y cuatro PNG indexados de 8 bits sin entrelazado: aprobada.
No se ejecutaron nuevas pruebas unitarias; ejecución de state blocks pendiente
sobre hardware. Los controles de visor/música, heap reutilizable y límites de
startup se conservan. Boot completo y gameplay permanecen pendientes.

Build iteration68-state-blocks-r1.
VPK artifacts/iteration68/Touhou75Vita-iteration68.vpk.
SHA256: 8339b7958186514ede763c23ce98661199ccd72d75a8fb7cadf887e0f83f1f3f.

Enviar iteration68.log, iteration68-runtime.log, iteration68-watchdog.log,
iteration68-bgm.log, log.txt y captura. Buscar startup_d3d8_stateblock y
startup_stop_import: pueden aparecer nuevos estados/bindings antes de End.
