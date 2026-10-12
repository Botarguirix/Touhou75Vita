# Iteración 66 — Buffered Startup / 01.70

## Evidencia de la Vita, iteración 65

La corrección de reanudación funcionó: dos pausas en slots registrados del puente
continuaron. Se mantuvieron 39 cargas PCM/3103344 bytes, 199 llamadas DirectSound,
61 D3D y seis DInput. El principal avanzó de 1238 a 3613 llamadas y de 371 a 588
asignaciones de heap. Cada muestra de progreso cambió durante la carga. La parada
se debió a slice_cap=16, a los 39770893 us, EIP 00645414. No se observó una nueva
API incompatible; log.txt aún termina en SystemDataInit. Eso no demuestra que la
carga haya terminado ni que el EXE dibuje o reproduzca audio todavía.
BGM nativo: 170 bloques, cero late_fill_blocks y cierre correcto del puerto.

## Cambios

El presupuesto por tanda conserva 65536 y el máximo pasa de 16 a 32. Se mantienen
tope de tiempo 45 s, watchdog 60 s y comprobaciones de progreso, mapping, stack,
retorno y TIB. Más tandas permiten aprovechar tiempo que antes quedaba disponible;
no garantizan completar la carga antes del límite de tiempo.

Los archivos abiertos solo para lectura tienen setvbuf de 64 KiB por handle,
con memoria estable en nodos std::map hasta fclose. El máximo existente de 32
handles limita memoria extra a 2 MiB. Se registra rc de setvbuf; si falla se
conserva la lectura estándar. No se sustituyen ReadFile, fseek ni ftell, ni se
modifican los datos del juego. CloseHandle cierra antes de liberar el búfer.
El log writable del propio juego mantiene su tratamiento y flush originales.

El log diagnóstico aumenta de 8 a 32 KiB. Se conserva cada línea y se fuerza
flush cada 32 callbacks atendidos, al salir de cada tanda y ante parada, error
o yield. Esto reduce escrituras pequeñas; la ganancia debe medirse en Vita.
Un cierre abrupto podría perder el tramo todavía en búfer (hasta 31 callbacks,
o menos si el búfer se llena), mientras el watchdog conserva su archivo separado.
No se ocultan fallos ni se cambian resultados guest.

## Paquete

VitaSDK compiló sin advertencias; diff --check limpio. Inspección ZIP CRC,
SFO 01.70/T075VITA1 y cuatro PNG indexados sin entrelazado: aprobada.
No se ejecutaron nuevas pruebas unitarias. Rendimiento y progreso de arranque
pendientes de ejecución física.
SHA256: e7fdc1f80d1a967a02d6309876875301dcaf28db7fcca4d99203ccbf057f5055.
Build iteration66-buffered-startup-r1.
VPK artifacts/iteration66/Touhou75Vita-iteration66.vpk.

Enviar iteration66.log, iteration66-runtime.log, iteration66-watchdog.log,
iteration66-bgm.log, log.txt y captura. Revisar startup_file_read_buffer (rc=0),
startup_elapsed_us, slice_progress, resume_stop y startup_stop_import. Comparar
avance y tiempo contra las 3613 llamadas/39,77 s de la 65. Visor, exportación y
controles de música se conservan. Título original, Draw/Present, mezclado guest
real y gameplay siguen pendientes.
