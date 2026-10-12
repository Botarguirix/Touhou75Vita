# Iteración 65 — Startup Resume / 01.69

## Evidencia física de la 64

39 buffers secundarios creados y 39 cargas PCM completadas: 3103344 bytes.
QI IDirectSoundBuffer8, referencias, Lock ENTIREBUFFER y Unlock funcionaron.
DirectSound atendió 199 llamadas. Se mantuvieron 61 D3D y seis DInput;
1238 llamadas principales, 371 HeapAlloc y dos threads. El log del EXE
registró DirectSoundInit, JoystickInit y KeyboardInit OK y llegó a SystemDataInit.
Esto no confirma reproducción del audio guest ni dibujo/presentación originales.
La música del visor sigue por un backend independiente; su log cerró el puerto
correctamente y registró dos bloques de llenado tardío, sin ser medida directa
de underrun ni prueba de audio original.

Tiempo de startup: 16183355 us. La parada fue CPU BUDGET en 00B005C0 tras cinco
reanudaciones. startup_resume_stop=invalid_code_stack_or_tib: el runner rechazaba
cualquier EIP que no perteneciera a una sección ejecutable del PE. La dirección
es el slot 92 del puente; la tabla de imports del EXE confirma KERNEL32!HeapAlloc.
El backend Box86 comprueba el presupuesto antes de despachar el callback y declara
esta pausa reanudable al principio del bloque que iba a ejecutar. No es evidencia
de que HeapAlloc fallara ni de que el juego se haya quedado en un bucle.

## Corrección

La reanudación admite secciones ejecutables originales o slots exactos registrados:
imports IAT, dos exports dinámicos conocidos y métodos COM propiedad de los bridges.
Para un slot se exige dirección de retorno mapeada en código ejecutable original.
Siguen obligatorios mapping, stack principal válido y TIB principal correcto.
Se excluyen sentinel, slots desconocidos y direcciones no alineadas. No se llama
manualmente al callback ni se modifica EIP/ESP/retorno: run conserva el contexto
y despacha normalmente después de recargar el presupuesto. Se conservan el límite
de 16 slices, repetición de estado, tope de 45 s y watchdog de 60 s.

El log registra startup_pending_trap, startup_pending_import,
startup_import_resume_slices y scope owned_import_trap. Las pausas fuera del PE
ya no se describen erróneamente como budget_in_image. El diagnóstico de rechazo
incluye código, slot, retorno, stack y TIB para localizar la condición que falló.

## Verificación y siguiente captura

VitaSDK compiló sin advertencias. Prueba local de clasificación: acepta el slot
observado, extremos IAT y exports/COM registrados; rechaza fuera de rango,
sentinel, desconocidos y los 15 offsets internos de slots. No prueba la ejecución
Box86 ni valida hardware. ZIP CRC, SFO 01.69/T075VITA1 y cuatro PNG indexados de
8 bits sin entrelazado: aprobados. Prueba física de reanudación pendiente.

Build iteration65-startup-resume-r1.
VPK artifacts/iteration65/Touhou75Vita-iteration65.vpk.
SHA256: 8ff79915e702ce8f5738ac1aa0d6d9160a591fbb87180511c78837233f3521cc.

Devolver iteration65.log, iteration65-runtime.log, iteration65-watchdog.log,
iteration65-bgm.log, log.txt y captura. Revisar pending_import, import_resume_slices,
pcm_upload, startup_stop_import y la continuación de SystemDataInit.
Los controles de música/visor y exportación se conservan. Pantalla original,
reproducción/mezclado del EXE y gameplay siguen pendientes.
