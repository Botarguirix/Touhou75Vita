# Iteración 71 — Random DAT Rect / 01.75

## Resultado físico de la 70

El EXE superó SetRenderState(14,1), (19,5) y (20,6): escritura Z y factores
SRCALPHA/INVSRCALPHA. Clear registró 307200 píxeles en la viewport 640×480.
La nueva frontera fue USER32.dll!SetRect, retorno 00425681, trap 00B00770,
destino RECT 009FEB9C, ESP 009FEB80. No hubo límite ni fallo de CPU.

Se registraron 9669 llamadas principales, 2499 D3D, seis DInput, 199 DirectSound,
30 reanudaciones y 13347898 us. El watchdog terminó disarmed. log.txt conserva
LoadEffect ...OK. BGM cerró correctamente, con 242 bloques y un late_fill_block;
ese contador no demuestra por sí solo un corte acústico.

La 70 no alcanzó SetTexture ni DrawPrimitiveUP. Los bindings de textura y la
captura del primer draw preparados previamente todavía necesitan evidencia.

## Avance del EXE

Se implementa SetRect con cinco argumentos stdcall: destino más cuatro LONG
firmados. Conserva las coordenadas sin normalizar ni recortar, escribe y verifica
los 16 bytes y devuelve BOOL 1. NULL devuelve FALSE. No cambia LastError.
El puntero debe estar dentro del espacio guest válido y ser escribible.
La limpieza incluye 24 bytes contando el retorno y conserva registros no volátiles.
Referencia: [Microsoft SetRect](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setrect).

La IAT original importa solo SetRect de la familia RECT; el desensamblado contiene
353 llamadas directas y su thunk. Desde 425681, el helper 411400 convierte RECT
a un quad y 40C9A0 entra en 4029F0. Después consulta la superficie y su descripción,
libera la referencia, llama SetTexture en 402D17 y DrawPrimitiveUP en 402D3E.
Esa última llamada solicita TRIANGLESTRIP, dos primitivas y stride 28.
La captura física debe confirmar la ruta. Draw/Present siguen pendientes.

## Círculo: gráfica aleatoria

- Elige un contenedor aleatorio distinto del mostrado y un frame aleatorio de él.
  Si solo existe un contenedor, evita repetir el frame cuando hay alternativas.
- La semilla combina el reloj del proceso y el tamaño del archivo.
- Conserva la caché de índices y decodifica directamente el frame elegido;
  no decodifica antes el frame cero del nuevo contenedor.
- Mantiene la gráfica anterior hasta que la nueva se decodifica. Si falla,
  prueba hasta ocho contenedores distintos; si todos fallan o falta memoria,
  restaura la gráfica anterior. El respaldo puede aumentar temporalmente la
  memoria al tamaño de dos imágenes, ambas sujetas al límite previo de píxeles.
- Los botones del visor se habilitan solo si el DAT abrió correctamente.
  Izquierda/Derecha siguen cambiando frames; Start exporta el mostrado.
  Triángulo cambia música, Cuadrado la reinicia y X sale.

El log incluye dat_view_random_seed, dat_view_random y los tiempos de decode,
índice y preview. No se promete que el primer acceso a cada recurso sea instantáneo.

## Investigación y próximo hito

La [revisión actualizada de N0zoM1z0](RESEARCH_N0ZOM1Z0_ITERATION71.md) confirma
el mismo target de TH075 y localiza adaptadores de gráficos/audio en TH08-Web.
Se usan como referencia para adaptar el backend, sin declarar compatible su
lógica ni sus formatos de recursos. Esta iteración incorpora el contrato RECT
y el visor aleatorio; no incorpora aún el renderer o mixer de TH08-Web.

El próximo hito es capturar el primer draw real del EXE y producir sus píxeles.
Después: restauración de destinos, Present, menú interactivo y combate.

## Paquete y captura requerida

Compilación VitaSDK sin advertencias. ZIP CRC correcto; SFO 01.75/T075VITA1 y
cuatro PNG indexados de 8 bits sin entrelazado. LiveArea conserva el icono original
y muestra iteración 71/version 01.75. diff --check limpio.
No se añadieron ni ejecutaron nuevas pruebas unitarias; falta prueba física.

Build: iteration71-random-dat-rect-r1.
VPK: artifacts/iteration71/Touhou75Vita-iteration71.vpk.
SHA256: 3f7a1781a925355b0344be25318e238b75e0ccf4dc492970a0ac9dd9a9832625.

Enviar los cuatro iteration71*.log, log.txt y captura. Revisar RECT, binding,
draw_boundary/vertex y random/timing. El PASS del diagnóstico acredita el
checkpoint alcanzado, no un arranque completo ni gameplay.
