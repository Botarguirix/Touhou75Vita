# Iteración 86 — Menu Boot

## Resultado físico de la 85

339 Present originales, 343 esperas reanudadas, 2999 dibujos. Paró por el
límite temporal, 180,532 s, en WaitForSingleObject original; no CPU fault ni
API nueva sin implementar. Los 343 contextos main y 626 de despacho audio
quedaron preservados; salida/shutdown/watchdog sin error.

El usuario escuchó la música completa sin problemas y alcanzó a ver parte
del menú. Los objetos observados confirman la ejecución de TitleScene:

| Escena | Vtable | Update / Draw | Esperas observadas |
|---|---|---|---:|
| Logo | 00657BE0 | 004255D0 / 00425620 | 203 |
| Opening | 00657CE4 | 00427730 / 004277A0 | 43 |
| Menú original | 00658308 | 0043A270 / 0043A7A0 | 97 |

Audio nativo: 5777 bloques, 123,243 s enviados a 48 kHz, 10820535 samples
no cero, un Play. Máximo intervalo 21759 us, cero intervalos mayores a dos
bloques; 473 bloques silenciosos, todos contados también como sin voces
activas. No hubo bloques completamente silenciosos con voces activas. Esto,
junto a la confirmación auditiva, valida la corrección de offsets en esta
corrida; no equivale a validar todas las voces del combate.

Raster/hashes: 99,620 s de 180,532 s (55,2 %). Agrupando por la escena leída
en la espera anterior: unos 29,559 s de logo, 35,589 s de opening y 33,704 s
de menú; los bordes de transición hacen esta atribución aproximada.
Memoria gráfica final 29510472 bytes (28,1 MiB), pico 42896712 (40,9 MiB),
límite 64 MiB. Bajó tras el opening; no se observa acumulación de todas sus
texturas. Eso no descarta problemas de ownership en escenas futuras.
El overlay 1 FPS al final no determina el FPS durante toda la corrida.
Los logs, foto, resumen y extractos ASM quedan fuera de Git.

## Salto al menú solicitado por el usuario

Se conservan el arranque/servicios, un primer update y Present del logo,
su transición original y su destructor. El destino pasa de opening a title;
no se escribe un puntero a una escena artificial ni se devuelve éxito a
una API desconocida. El dispatcher original llama a TitleScene::Initialize,
carga sus propios recursos y ejecuta su update/draw/input.

Cuatro puntos del EXE japonés 1.11 identificado por SHA se modifican **sólo
en la copia mapeada**, antes de ejecutarla. El EXE y DAT en disco no cambian.
Todos los bytes originales se validan antes de la primera escritura; cada
escritura se relee. Fallo exige rollback y rechaza ejecución, incluido fallo
de rollback. Se invalida código traducido de los puntos modificados.
Son diez bytes modificados, no una reconstrucción ni un menú simulado.

| Punto | Cambio | Motivo |
|---|---|---|
| 00425608 | Umbral del logo: 180 a 0 updates previos. | Salir después del primer update/Present genuino. |
| 00425610 | Retorno 220D a 2202. | Dispatcher original crea title, omitiendo opening. |
| 0043A2C4 | Umbral unsigned 57000 a FFFFFFFF ms. | Mantener habilitada la selección mientras se prueba el menú. |
| 0043A625 | Umbral unsigned 60000 a FFFFFFFF ms. | Evitar la demo automática que volvería a escenas no solicitadas. |

Los dos últimos puntos son gates temporales del menú. Al comparar edad
unsigned, el attract demo queda inalcanzable; confirmación conserva la
comparación estricta del EXE (excepto la edad FFFFFFFF puntual). No cambia
la lógica de combate. El logo/transición breve e inicialización general
siguen existiendo: no se promete un menú instantáneo ni FPS determinados.

Se omite también la música del opening: su Initialize llama a
Music::PlayTrackByIndex; TitleScene::Initialize no la inicia. No se inserta
música de diagnóstico para reemplazarla. La corrección PCM permanece y las
escenas/acciones ejecutadas conservan su sonido original. La próxima corrida
puede tener un menú silencioso; eso por sí solo no indica regresión PCM.

## Controles y observación

El adaptador DirectInput ya producía estos bytes de teclado; ahora hay
pruebas COM de la clase enviada y logs de pulsación/liberación realmente
entregadas. GetDeviceState sigue usando snapshots nativos de la Vita,
[API de VitaSDK](https://github.com/vitasdk/vita-headers/blob/master/include/psp2/ctrl.h).
No se insertan selecciones automáticas. El juego interpreta las teclas con
su configuración original; funciones de combate aún requieren comprobación.

| Vita | Tecla / scan DirectInput |
|---|---|
| Cruceta / stick izquierdo | Flechas / C8 D0 CB CD |
| × | Z / 2C |
| ○ | X / 2D |
| □ | C / 2E |
| △ | A / 1E |
| Start | Enter / 1C |
| Select | Escape / 01 |
| L | Shift izquierdo / 2A |
| R | Espacio / 39 |

Stick: menos de 64 o más de 192; valores límite son zona neutral. Poll
retiene su snapshot hasta el GetDeviceState siguiente; GetDeviceState directo
muestrea si no hay uno pendiente. Las teclas no son mensajes Win32 sintéticos.
× sólo sale de la app al estar ya en la pantalla final de diagnóstico;
durante el EXE sigue siendo Z. Joystick DirectInput separado sigue pendiente.

Se agrega startup_frame_title_state (selection, elapsed_ms, vertical,
escape_frames), lectura del objeto original, para contrastar la respuesta
del menú con los scans entregados. No se modifican sus campos.
La validación de rangos se refuerza antes de indexar tablas DirectInput:
un trap ajeno/desalineado devuelve frontera y conserva el callframe.

## Comprobaciones y paquete

96 grupos portables con -Werror y ASan/UBSan: los 80 previos, siete del
preset y nueve DirectInput. Se comprueban fingerprint mismatch antes de
escrituras, fallos de lectura/escritura/readback/rollback, cambios exactos,
12 botones digitales, stick/diagonales, release, adquisición/formato,
referencias, errores nativos, ABI y traps ajenos. La compilación inicial de
la prueba DirectInput señaló indexación sin guard explícito; se corrigió
y se repitieron las comprobaciones.

El original local con SHA conocido valida los cuatro fingerprints y la
tabla dispatcher: title case 2 llama 0043A020, opening case D llama 004275D0.
No se ejecutó el EXE en un host para validar el salto; eso queda para Vita.
Se preservan los 200000 casos bilineales, 500 strips/oráculo, replay de 1312
llamadas y digests POINT/LINEAR/triángulos iguales a -O0/-O2.

VitaSDK compila el paquete y se validan ZIP/CRC, SFO, PNG completos y
SELF/imports/memoria. El VPK local incluye arte de LiveArea de Alice,
decodificado de nuevo del DAT original. El arte y los archivos originales
EXE/DAT/logs quedan fuera de Git; EXE/DAT/logs tampoco se incluyen en el VPK.
Build: iteration86-menu-boot-r1. Versión 01.90, T075VITA1.
VPK local: artifacts/iteration86/Touhou75Vita-iteration86.vpk.
SHA256: 2f82ed4ee7d0530e8f90e2b0121fb45565e6f5a26a0f27e75e454f031273a10c.

## Próxima prueba física

Instalar 86/01.90. Esperar la inicialización general y el logo/transición
breves; el salto al menú es lo que debe validar esta corrida. Probar
arriba/abajo y × con pulsaciones cortas o sostenidas durante el muestreo,
luego ○ para cancelar si aplica. Anotar qué opciones/movimientos responden.
Si se alcanza una nueva API de selección/personaje, el diagnóstico se detendrá
con la frontera original. Eso permite implementarla en la siguiente iteración.

Se conservan hasta 360 Present / 361 esperas / 180 s, watchdog 210 s,
6144 dibujos / 768 Mi píxeles / 64 MiB gráficos y 1024 despachos audio.
El límite puede detener el menú antes de 360 Present. Enviar iteration86.log,
iteration86-runtime.log, iteration86-watchdog.log, log.txt y captura.

Falta confirmar salto, navegación/selección y coste real. El menú sigue
renderizándose por CPU: evitar intro ahorra trabajo total de arranque, no
acelera cada píxel del menú. El siguiente frente de rendimiento es un
backend GPU conservando filtros, alpha, estado y ownership. Se mantienen
como referencias los snapshots TH075 para identidades/callsites y TH08-Web
para separar D3D8 del renderer nativo: [investigación previa](RESEARCH_EXE_ITERATION73.md).
No se presupone un motor TH075 completo en esas fuentes. Combate jugable pendiente.

## Resultado físico recibido: 10 de octubre de 2026

El salto al menú y la navegación funcionaron en Vita; sonidos limpios según
el usuario. 272 Present, 273 esperas, 5811 draws, TitleScene por 250 esperas,
selección original 0..9. Stop time_cap 180,484 s. Raster/hashes 88,729 s;
Present/conversión/captura/esperas 54,769 s. Memoria gráfica final/pico 29,0 MiB.
La CPU sigue cargada. Se distribuye el raster y se reduce diagnóstico en
[iteración 87](STATUS_ITERATION87.md); combate sigue pendiente.
