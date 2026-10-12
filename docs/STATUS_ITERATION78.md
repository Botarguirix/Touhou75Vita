# Iteración 78 — Resource Lifetime / 01.82

## Resultado de la Vita en la 77

203 Present originales y 203 continuaciones timer/main confirmados. Contextos
preservados; set/vblank/query RC 0 y matches:yes en todos los frames.
Los primeros dieciséis hashes coinciden con la 76. La edad original del logo
llegó a 181, el estado pasó a 220D y el fade salió completamente hasta negro
(frame 203, scanout FNV-1a 75895DC5). La captura negra corresponde a esa salida.

La caché nativa preparó 22 imágenes y reutilizó 181 fuentes idénticas; todos los
Present nativos y updates del EXE siguieron ejecutándose. Present tardó entre
22297 y 327099 us, mediana 26229 us. El tiempo total fue 70830104 us, 70,83 s;
no CPU fault/limit, watchdog desarmado. Son alcances distintos de corridas
anteriores y no constituyen una comparación de rendimiento controlada.

El EXE inició la carga posterior. Antes de MessageBoxA, CreateTexture pidió
512x512, un nivel, usage 0, formato 21/A8R8G8B8, pool 1: 1048576 bytes.
El puente tenía 33197384 bytes usados de 33554432; quedaban 357048 bytes.
La llamada devolvió 8876017C. Después MessageBoxA retornaría a 401A1B, trap
B00760, IAT 65720C; se detuvo sin ejecutar esa API. ESP 9FEB70, EBP 9FEB84,
SEH válido de dos niveles. 5746 servicios D3D, 187 DInput, 199 DirectSound,
14038 imports del main y 30 slices. log.txt conserva LoadEffect OK.

## Causa corregida

La textura del logo ABC938, 1024x512, ocupa 2097152 bytes. Su Release original
tras el frame 203, retorno 40AEE8, dejó refs=1 en el puente anterior. Al crear
la superficie de nivel cero, el puente añadía una referencia permanente al
padre y mantenía la superficie cacheada con otro contador. No existía un camino
que liberara el vector a refs=0; esos 2 MiB seguían dentro del presupuesto.

La 78 usa una sola propiedad para la textura y su superficie de nivel cero:

1. AddRef/Release de la superficie se reenvían al padre. La caché de metadata
   no agrega una referencia permanente.
2. Las referencias externas, bindings, render/depth targets y state blocks
   retienen almacenamiento mientras lo necesitan. Reemplazar una referencia
   primero conserva la entrante y luego libera la saliente.
3. Al desaparecer el último ref válido se libera el vector, se invalidan las
   vtables guest del padre/hijo, se descuenta used y se libera el hold del device.
4. Se rechaza destruir un recurso locked o todavía referenciado por estado.
   Las superficies implícitas no se destruyen; la destrucción final del device
   continúa como frontera pendiente. Handles muertos no se reutilizan.
5. El presupuesto continúa en 32 MiB. Rechazos registran causa storage_budget,
   native_heap o handle_capacity; liberar datos sin uso es distinto de aceptar
   una textura que realmente excede el presupuesto vivo.

La semántica de refs de superficies de textura se contrastó con la
[implementación D3D8 de Wine](https://github.com/wine-mirror/wine/blob/master/dlls/d3d8/surface.c).
Es una referencia de comportamiento; la implementación y tests son propios,
sin incorporar el código de Wine. El runtime continúa siendo el WinVita/Box86
fijado por el proyecto; no se sustituyó la lógica del EXE por otra reconstrucción.

## Diagnóstico y límites

MessageBoxA sigue siendo una parada explícita. Se registran sus cuatro
argumentos y hasta 256 bytes de texto/caption como hexadecimal raw_CP932,
con estado terminated, truncated, null, unreadable u out_of_range. Se validan
stack/arena antes de leer. No se abre un diálogo ni se inventa un resultado.
Una futura parada podrá distinguir el mensaje original de otro error.

Cap de 100 s y watchdog independiente de 120 s: la 77 ya necesitó 70,83 s
para alcanzar esta carga, por lo que 75 s dejaba poca oportunidad de avanzar.
Se conservan 240 Present/waits reales, 512 draws, 128 Mi píxeles cubiertos y
64 slices. Ningún contador original se modifica. X sale del diagnóstico.
LiveArea indica ITERACION 78 / VERSION 01.82. Los controles auxiliares de
música y sprites siguen retirados.

## Comprobaciones y paquete

34 grupos propios con ASan/UBSan, -O2 y -Werror aprobaron; seis nuevos sobre
la clase D3D8Storage de producción y cinco sobre observación de strings.
El renderer a -O0/-O2 conservó salidas y digests idénticos.

Además, se reprodujeron 1286 llamadas de propiedad/estado registradas en la
77: el Release del logo devuelve cero y la última CreateTexture ahora termina
correctamente. Esto verifica el cambio de presupuesto con la secuencia observada.
La reproducción omite datos de recursos, draws, SDK y CPU x86; no prueba que
la siguiente escena haya completado ni que otras futuras asignaciones quepan.
Véase [lista y criterios físicos](EXE_CHECKLIST_ITERATION78.md).

VitaSDK compiló el VPK sin advertencias en esta recompilación. Flags:
-O2 -g -DNDEBUG, -fno-fast-math, -ffp-contract=off. ZIP CRC correcto,
SFO 01.82/T075VITA1 y cuatro PNG indexados de ocho bits sin entrelazado.
Hardware 78 pendiente; menú, gameplay y mixer guest no verificados.

Build: iteration78-resource-lifetime-r1.
VPK local: artifacts/iteration78/Touhou75Vita-iteration78.vpk.
SHA256: ecb8fa28a3baf4f991052549776911ed5c5d733a634e6638037c06255af76628.

## Siguiente comprobación física

1. Instalar 78/01.82 conservando TH075.exe y sus datos originales en la Vita.
2. Esperar al diagnóstico: cap 100 s y watchdog 120 s. Fotografiar captura,
   STOP y PRESENT FRAMES. X sale.
3. Enviar iteration78.log, iteration78-runtime.log, iteration78-watchdog.log,
   log.txt y captura.
4. Verificar destrucción de ABC938/2097152 bytes, asignación 512x512, nuevos
   frames/escena y primera frontera siguiente. Si reaparece MessageBoxA,
   su texto hexadecimal permitirá investigar el error exacto.

## Dónde estamos y qué sigue

El logo y su salida original ya están confirmados. Estamos en la carga de
la siguiente escena; esta build corrige su fallo observado de memoria.
Siguen sus dibujos/contratos, transición al menú, entrada real, scheduler
continuo, audio original, combate y tiempos. El avance se mide por esos hitos,
no por un porcentaje inventado. El estado completo está en [ROADMAP](ROADMAP.md)
y los proyectos/decompiladores revisados en [RESEARCH](RESEARCH_EXE_ITERATION73.md).
