# Iteraci�n 64 � PCM Contracts / 01.68

## Causa de la frontera 63

Los logs registran CreateSoundBuffer v�lido: flags 00058080, 32608 bytes,
PCM mono 44100 Hz, 16 bits, align 2, byte rate 88200 y cbSize cero.
QueryInterface devolvi� E_NOINTERFACE; el EXE liber� el temporal y llam�
su helper de error 004066C0, que llega a MessageBoxA. La petici�n IID en
0065B48C se comprob� directamente contra nuestro EXE y corresponde a
IDirectSoundBuffer8: 6825A449-7524-4D82-920F-50E36AB3AB1E.
No se omite ni se devuelve �xito ficticio a MessageBoxA.

## Lote preparado

1. QueryInterface Buffer8: identidad COM compartida, AddRef y vtable de 24 slots.
2. GetCaps: descriptor de 20 bytes y almacenamiento de software.
3. GetCurrentPosition: cursores detenidos; todav�a no se atiende Play.
4. GetFormat: WAVEFORMATEX original, tama�o requerido y validaci�n del output.
5. GetVolume: control almacenado o DSERR_CONTROLUNAVAIL.
6. GetPan: control almacenado o DSERR_CONTROLUNAVAIL.
7. GetFrequency: frecuencia actual si el buffer admite ese control.
8. SetFrequency: rango 100..200000, cero restaura original; capacidad obligatoria.
9. Stop: idempotente para los buffers detenidos actuales.
10. Restore: conserva muestras; todav�a no hay transici�n de p�rdida de dispositivo.

Lock/Unlock, referencias, posici�n y volumen existentes se conservan. GetFormat
usa la frecuencia del formato original aunque SetFrequency cambie el control.
SetFX/AcquireResources/GetObjectInPath tienen slots ABI pero siguen sin atenderse.
Play y mezclado siguen siendo fronteras. Esto no equivale a diez pruebas f�sicas
aprobadas ni garantiza saltar diez fronteras: depende de las llamadas del EXE.

Cada llamada atendida registra startup_dsound_pcm_contract con m�todo, handle,
HRESULT, bytes y cursor. QI registra el IID solicitado y si se reconoce;
Lock/Unlock mantienen tama�os y hash de las muestras cargadas. La ruta siguiente
visible en el EXE es Lock ENTIREBUFFER, copia PCM y Unlock (00406B1D..00406B67).

## Verificaci�n y captura

VitaSDK compil� el paquete; diff --check sin errores. Inspecci�n ZIP CRC,
SFO 01.68/T075VITA1 y cuatro PNG indexados de 8 bits sin entrelazado: aprobada.
La ejecuci�n de estos contratos en la Vita est� pendiente.
SHA256: 6e1083c7308831a77f620b8ed584c183ee786439e4fc2ca90ea9b70089850105.

VPK: artifacts/iteration64/Touhou75Vita-iteration64.vpk.
Build: iteration64-pcm-contracts-r1. Se conservan controles y BGM del visor.
Devolver iteration64.log, iteration64-runtime.log, iteration64-watchdog.log,
iteration64-bgm.log, log.txt y foto. Buscar pcm_qi, pcm_lock, pcm_upload,
pcm_contract y startup_stop_import. No se anuncia arranque completo del juego.
