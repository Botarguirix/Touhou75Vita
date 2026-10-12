# Iteración 54 — Native Audio / 01.58

Actualización física: la Vita confirmó COM, apertura/configuración del
puerto, cooperación con la ventana y liberación. Se alcanzó el buffer
primario esperado. La [iteración 55](STATUS_ITERATION55.md) añade la prueba
audible con un efecto original; sigue pendiente el contrato de buffers del EXE.

## Evidencia física de la 53

Los logs del usuario confirman una llamada atendida a DirectInput8Create:
instancia 00400000, versión 0800, IID original, objeto y vtable propios,
retorno 00403D5F y limpieza stdcall de 24 bytes correctos. La lectura nativa
del mando devolvió botones 0 y analógico 132,124. El EXE escribió
DirectInputInit / OK en su log y comenzó DirectSoundInit.

No llamó todavía a CreateDevice ni GetDeviceState. Por tanto, la aceptación
del teclado y el control interactivo siguen sin validación física.

El arranque atendió las mismas 30 llamadas gráficas y se detuvo en
ole32.dll!CoCreateInstance, retorno 0040670F, en 6.190499 segundos.
No hubo límite de ejecución y el watchdog quedó desarmado.

## Nuevo contrato

La inspección del original en 004066E0 identifica CLSID_DirectSound8
3901cc3f-84b5-4fa4-ba35-aa8172b8a09b e IID_IDirectSound8
c50a7e93-f395-4834-9ef6-7fa99de50966. Se contrastaron con
[Wine dsound.h](https://raw.githubusercontent.com/wine-mirror/wine/master/include/dsound.h).
El EXE pide contexto COM 3, agregación nula y salida en 00671380.
Después llama Initialize con dispositivo predeterminado, SetCooperativeLevel
con nivel 2 y CreateSoundBuffer con descriptor de 36 bytes y flags 00040001.
Esta secuencia procede del análisis estático; falta comprobarla en Vita.

La 54 incorpora:

- Activación restringida de esa clase e interfaz COM, con apartamento
  registrado para el TIB actual. No expone un servidor COM genérico.
- Objeto/vtable de 12 métodos con validación de identidad, GUID, referencias,
  punteros y ABI. QueryInterface, AddRef y Release para el objeto propio.
- Initialize de dispositivo predeterminado mediante sceAudioOutOpenPort:
  puerto MAIN, 1024 frames, 48000 Hz y estéreo. Se comprueban tamaño,
  frecuencia y modo mediante sceAudioOutGetConfig.
- SetCooperativeLevel para la ventana del juego y niveles NORMAL/PRIORITY.
- Liberación del puerto al destruir el objeto o salir del probe, incluyendo
  los fallos posteriores a abrirlo; resultado registrado en el log.
- Registro del descriptor al alcanzar CreateSoundBuffer y parada explícita
  antes de asignarlo: faltan contrato de buffers, formato, mezcla y playback.
- Contador startup_dsound_serviced_calls y registros de argumentos,
  HRESULT, retorno, limpieza de pila y errores nativos.
- LiveArea local actualizado a ITERACION 54 / VERSION 01.58.

No se emiten muestras de audio ni se retorna éxito para crear buffers.
No se añadieron pruebas sintéticas ni se aumentaron los presupuestos.

## Roadmap inmediato

1. Confirmar activación y apertura/configuración/liberación del puerto en Vita.
2. Implementar el buffer primario y su formato según el descriptor observado.
3. Continuar inicialización original, completar el teclado si se solicita
   y registrar las siguientes llamadas reales de archivos, audio y gráficos.
4. Integrar uploads originales, Draw y Present para obtener el primer frame
   del EXE. La imagen DAT de diagnóstico sigue siendo una previsualización.
5. Confirmar menú interactivo, reproducción de audio y después una partida.

## Entrega

Build ID: iteration54-native-audio-r1. TITLE_ID: T075VITA1.
Versión: 01.58. VPK local: artifacts/iteration54/Touhou75Vita-iteration54.vpk.
VitaSDK compiló sin advertencias; se inspeccionaron CRC ZIP, SFO y los
cuatro PNG indexados de 8 bits, sin entrelazado. No se ejecutó el paquete
en hardware durante su construcción.

SHA256: b23858c09c23d06acbbe5faa7c07dfd1b3d815fe5069317b4e0cbb17f26ff980.

Devolver iteration54.log, iteration54-runtime.log,
iteration54-watchdog.log, log.txt y foto. Revisar startup_dsound_create,
startup_dsound_native_open_rc, startup_dsound_native_config,
startup_dsound_native_port, startup_dsound_abi y la frontera final.
