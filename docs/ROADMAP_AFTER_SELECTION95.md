# Roadmap: selección visible → combate → 60 FPS

## Dónde estamos

**La selección de personajes ya se presenta en Practice y Story.** Las dos
capturas del usuario muestran esa pantalla y después el diagnóstico de la 95.
Ambas identifican KERNEL32!GetTimeZoneInformation como parada. El único juego
de logs recibido registra 188 Present, 189 esperas y cuatro pulsaciones Cross/Z;
la otra captura registra 191 Present. No son dos trazas independientes.

El puente detiene la ejecución antes de atender esa API de Windows: es una
frontera de compatibilidad identificada. El watchdog quedó desarmado y el
límite CPU no se alcanzó. La 95 procesó 339 draws de área cero; no registra
rechazos de geometría ni de asignación. Hay 73,81 MiB gráficos dentro del límite
de 96 MiB, con 399/512 handles asignados. Combate todavía sin comprobar.

**Estamos más cerca del primer combate; el objetivo de rendimiento sigue
requiriendo trabajo considerable.** La selección tiene 115 ciclos medidos,
135,287 ms de media: aproximadamente 7,39 ciclos por segundo. Para 60 FPS se
necesita reducir ese tiempo unas 8,1 veces, y medir también la lógica del juego.
No hay base para dar una fecha, porcentaje de finalización o número fijo de
iteraciones restantes.

## Diez pasos con condiciones de cierre

| Paso | Trabajo | Se cierra cuando… |
|---|---|---|
| 1. Zona horaria — implementada en la 96 | Implementar GetTimeZoneInformation con estructura x86, RTC de Vita, retorno y epílogo correctos | La consulta devuelve datos comprobados y ambas rutas avanzan más allá de esa llamada |
| 2. Selección → carga | Registrar la confirmación de personaje/escenario y la siguiente escena; completar las APIs realmente solicitadas | Practice y Story llegan a cargar la partida con trazas separadas y reproducibles |
| 3. Primer combate visible | Completar recursos, estados, geometría y mezcla observados en arena, luchadores y HUD | Aparecen escenario, dos personajes y HUD, con varios frames sucesivos válidos |
| 4. Combate controlable | Validar movimiento, salto, ataques, guardia, dash, pausa y regreso a selección | Se puede jugar un segmento de Practice y comenzar una pelea de Story con las acciones registradas |
| 5. Primer renderer GPU | Reproducir el perfil gráfico de selección en GXM y compararlo con el renderer CPU | Cobertura, UV, alpha y mezcla coinciden con las referencias aceptadas; baja el coste de dibujo medido |
| 6. Texturas y Present | Mantener texturas GPU por versión, subir cambios y presentar sin copia CPU completa ni miniatura por frame | Las texturas estáticas se suben una vez; capturas/readback son explícitos y no hay recursos obsoletos ni fugas |
| 7. Coste guest y recursos | Perfilar JIT, imports, carga DAT y logging; usar funciones reconstruidas y workers registrados para trabajo separable | Las optimizaciones reducen costes identificados y conservan lógica, ABI, ownership y resultados |
| 8. Cadencia, input y audio | Verificar actualización de juego y presentación a 60 Hz, latencia de input y abastecimiento de audio durante cargas | Se registran 60 updates y frames completos por segundo, con controles y sonido estables |
| 9. Rondas y modos | Validar reinicio, nueva pareja de personajes, cambio de escenario, otra ronda y progresión de Story | Se completan rutas repetidas sin nueva frontera ni crecimiento continuo de memoria/handles |
| 10. Aceptación y calidad | Medir combate sostenido y después recuperar resolución/calidad por niveles | Una sesión de 10 minutos sostiene 60 updates/frames por segundo, con tiempos, controles, audio y memoria documentados |

Los pasos 5–7 pueden prepararse con la selección ya disponible mientras se
completa la ruta de combate. Sus referencias deben conservar un escenario,
pareja de personajes y secuencia de entrada comparables. Los pasos de
jugabilidad no se consideran cerrados mediante un microbenchmark gráfico.

## Siguiente iteración: contrato de zona horaria

GetTimeZoneInformation recibe una estructura y retorna UNKNOWN, STANDARD,
DAYLIGHT o INVALID. El signo del bias sigue UTC = hora local + bias; un retorno
0 es válido cuando no se usan transiciones DST.
[Contrato de Microsoft](https://learn.microsoft.com/en-us/windows/win32/api/timezoneapi/nf-timezoneapi-gettimezoneinformation).

Para el EXE PE32, TIME_ZONE_INFORMATION ocupa 172 bytes: LONG de 32 bits,
nombres WCHAR de 16 bits y dos SYSTEMTIME de 16 bytes. Se debe serializar
explícitamente, comprobar offsets y el buffer guest completo.
[Layout oficial](https://learn.microsoft.com/en-us/windows/win32/api/timezoneapi/ns-timezoneapi-time_zone_information).

Propuesta acotada: derivar el offset actual de UTC/local en la Vita, declarar
una zona de offset fijo sin reglas DST y registrar esa política. Un offset
actual no proporciona una tabla anual de transiciones. VitaSDK expone
conversión UTC/local y lectura de reloj local del sistema.
[RTC de VitaSDK](https://docs.vitasdk.org/group__SceRtcUser.html).

Comprobaciones previstas antes de empaquetar: bias positivo/negativo/cero,
offsets/terminadores UTF-16, puntero inválido y límites del buffer, readback,
fallos RTC, retorno coherente, preservación de registros y limpieza stdcall
de 8 bytes. Después: build Vita, ELF/SELF/SFO/imports/PNG y prueba física en
Practice y Story. La iteración 96/version 02.00 ya está implementada y compilada, con 168
grupos C++ y 11 pruebas Python locales. Su continuación física está pendiente.
[Reporte del paquete 96](STATUS_ITERATION96.md).

Resolver esta API permitirá observar la siguiente frontera. La carga de
combate puede requerir más servicios, recursos o perfiles gráficos: no está
demostrado que una sola corrección complete el arranque de la pelea.

## Presupuesto actual y trabajo de rendimiento

Mediciones de selección, 115 ciclos válidos de la corrida 95:

| Parte del ciclo | Media | Trabajo previsto |
|---|---:|---|
| Raster CPU | 64,623 ms | Quads y texturas GXM; conservar las reglas de cobertura/mezcla |
| Preparación de Present/captura | 38,874 ms | Presentación directa; readback y miniatura fuera del camino habitual |
| Resto: guest, bridges, timer y diagnóstico | 31,790 ms | Perfilar funciones/imports y reducir trabajo auxiliar medido |
| Total | 135,287 ms | Objetivo ≤16,667 ms, incluida la actualización del juego |

El resto ya supera por sí solo el presupuesto final. Mover el raster a GPU
debe acompañarse de cambios en presentación y trabajo guest. Esos 31,790 ms
no son una medición exclusiva de JIT. La carga/transición máxima fue 8,404 s,
que se contabiliza separadamente para no mezclar carga con FPS sostenidos.

La integración GXM requiere shaders, contextos, render targets, uploads,
sincronización y vida de recursos; se empezará por el perfil reproducible de
selección. La GPU aún no está integrada en el paquete 95.
[API GXM de VitaSDK](https://docs.vitasdk.org/group__SceGxmUser.html).

Se conservan 320×240 y las descripciones guest originales durante la búsqueda
de jugabilidad. Compresión/cachés de texturas se prepararán fuera del ciclo
de frame y se evaluará pérdida de alpha/calidad y ahorro real. Los 96 MiB de
gráficos, 128 MiB de heap y 512 handles deben observarse en carga y cambio
de ronda; las cifras actuales no garantizan que quepa combate.

Para los núcleos CPU, priorizar audio, preparación/carga y trabajo independiente
con hilos registrados y sincronización comprobada. La secuencia guest x86 y
sus estados compartidos requieren orden. El pool raw que falló en 87–89
permanece desactivado hasta verificar TLS, espera, afinidad y teardown.

## Recursos que ya ayudan

- La copia local de N0zoM1z0/th075 permite contrastar funciones/CFG con el
  mismo SHA del EXE. Su ledger registra fuente exacta para
  Graphics::DrawTexturedQuad (004029F0) y DrawProjectedTriangleStrip (00403200):
  son candidatos para el renderer GPU y futuros handlers nativos con ABI
  comprobada. Los roles inferidos deben contrastarse con la corrida real.
- El extractor DAT existente permite inventariar tamaños/formatos y preparar
  referencias fuera de Git. El archivo original se conserva para comparar
  lectura, locks, alpha y memoria.
- Ghidra y el desensamblado permiten revisar el caller de zona horaria y nuevas
  fronteras. Una traza PC original de selección/combate mediante apitrace
  puede aportar la secuencia D3D8 de referencia; todavía está pendiente.

[Ruta y herramientas](PLAYABLE_PATH.md) · [Métricas y aceptación](PERFORMANCE_60FPS.md)
· [Reporte de construcción 95](STATUS_ITERATION95.md).

## Pruebas físicas comparables

Guardar por separado los logs de Practice y Story: el programa usa los mismos
nombres en cada lanzamiento. Para cada corrida anotar modo, personajes,
escenario, acciones y pantalla alcanzada; conservar la captura y su diagnóstico.
Si aparece combate, comprobar movimiento y un ataque antes de cambiar de ruta.
Cada parada se estudia con su última API/paquete de draw.

Audio 95: 1205 bloques y un intervalo entre envíos mayor que dos bloques,
con máximo 289,534 ms; dispatch guest llegó a 15,858 s de retraso. Son medidas
host, no un contador de underrun físico ni prueba de audio audible. Revisar
las cargas largas y registrar el reporte del usuario.
