# Iteración 53 — Native Input / 01.57

Actualización física: el usuario confirmó DirectInput8Create, captura nativa
y ABI correctos; el EXE pasó a DirectSoundInit y alcanzó CoCreateInstance
en 6.190499 segundos. Todavía no ejerció el teclado. Evidencia y continuación:
[iteración 54](STATUS_ITERATION54.md). El texto siguiente conserva la entrega original.

## Evidencia física recibida: iteración 52

La Vita atendió 30 llamadas de Direct3D8, incluidas 11 llamadas a
SetTextureStageState en el slot correcto 63, sin escribir en punteros
de salida inexistentes. El EXE creó el dispositivo REF/software, buffers
640x480 y una textura de 1024x1024. En 6.018 segundos alcanzó
`DINPUT8.dll!DirectInput8Create`, retorno 00403D5F, con watchdog desarmado.
Esto confirma la corrección del contrato gráfico de la 52. No se observaron
uploads, Draw ni Present; la imagen DAT de diagnóstico no es un frame del EXE.

## Cambios de la 53

- Puente DirectInput8A con objetos y vtables x86 propios, referencias COM,
  validación de this, GUID, punteros, pila y retorno stdcall.
- DirectInput8Create para la instancia 00400000, versión 0800 y el IID
  original; configura captura analógica y registra un muestreo real de Vita.
- CreateDevice para SysKeyboard, SetDataFormat con validación de los 256
  objetos del formato original, SetCooperativeLevel para nuestra ventana,
  Acquire, Unacquire, Poll y GetDeviceState de 256 bytes.
- Una captura nativa alimenta la adaptación al teclado. Si falla la lectura,
  el estado se neutraliza y se registra el fallo. Poll y GetDeviceState
  reutilizan la misma captura pendiente.
- Contador `startup_dinput_serviced_calls`, argumentos, HRESULT, limpieza
  de pila y frontera de método explícitos en el log.
- LiveArea local con imagen del juego, icono original y etiqueta inferior
  izquierda ITERATION 53 / VERSION 01.57; cuatro PNG indexados de 8 bits.

Mapeo provisional: cruceta/analógico izquierdo a flechas; X a Z, círculo
a X, cuadrado a C, triángulo a A, Start a Enter, Select a Escape,
L a Shift y R a Space. Falta comprobarlo dentro del juego.

EnumDevices conserva una parada explícita: necesita ejecutar el callback
original para enumerar dispositivos. No se declara éxito para métodos sin
implementar. Tampoco se implementa en esta entrega el renderer o DirectSound.
La siguiente frontera concreta se conocerá con el log físico de la 53.

## Roadmap inmediato

1. Confirmar DirectInput8Create en Vita y registrar qué inicialización sigue.
2. Completar los contratos de teclado/enumeración y audio que exija el EXE.
3. Integrar uploads originales y estado de render en el backend Vita.
4. Dibujar y presentar un frame del EXE, después confirmar menú y controles.
5. Continuar con audio, partida, guardado y rendimiento medido en hardware.

No se reactivan las 300 pruebas ni se cambian los límites de ejecución.
El arranque visual completo sigue pendiente. No hay base para asignarle
un porcentaje o una fecha de terminación. Referencias revisadas y alcance
de su reutilización: [informe](REFERENCE_REPENTOGXM.md).

## Entrega y comprobación

Build ID: `iteration53-native-input-r1`; TITLE_ID: `T075VITA1`.
VPK local: `artifacts/iteration53/Touhou75Vita-iteration53.vpk`.
Compilación VitaSDK completada sin advertencias. Inspección de paquete:
CRC ZIP, APP_VER 01.57, TITLE_ID y cuatro PNG correctos.
La ejecución en Vita queda pendiente; no se añadieron pruebas sintéticas.

Devolver `iteration53.log`, `iteration53-runtime.log`,
`iteration53-watchdog.log`, `log.txt` y foto. Buscar `startup_dinput_create`,
`startup_dinput_native_sample`, `startup_dinput_abi`, métodos atendidos y
la siguiente `startup_stop_import`.

SHA256: 40a859b261cdebe86f32d9115b075499767615fceb57ac4b405dc91d8a1eb6cb.
