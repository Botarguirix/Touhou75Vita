# Informe de avance — Touhou 7.5 para PS Vita

Fecha: 7 de octubre de 2026. Nueva entrega: iteración 51, versión 01.55.

Actualización tras la prueba física 52: se confirmó la corrección de
SetTextureStageState; el EXE completó 30 llamadas gráficas y alcanzó
DirectInput8Create en 6.018 segundos. La nueva entrega 53/01.57 incorpora
captura nativa del mando y un adaptador de teclado DirectInput8 para TH075.
Su ejecución física está pendiente. El primer frame del EXE todavía requiere
uploads, Draw y Present. Detalles y roadmap inmediato:
[STATUS_ITERATION53.md](STATUS_ITERATION53.md). Referencias evaluadas:
[REFERENCE_REPENTOGXM.md](REFERENCE_REPENTOGXM.md). El resto conserva
la fotografía histórica del proyecto al entregar la versión 51.

## Dónde estamos

El EXE original ya ejecuta su entrada y parte de la inicialización en la Vita
mediante el traductor x86 a ARM. La última evidencia física, iteración 50,
llega a la consulta gráfica GetDeviceCaps en unos 5.5 segundos. Todavía no
tenemos un menú dibujado por el EXE en la Vita ni una partida jugable.

La captura del original en Windows sí llegó al menú completo. Nos proporciona
una referencia para comparar llamadas y píxeles; su éxito no demuestra que
esas mismas operaciones funcionen en la consola.

## Estado por componente

| Componente | Evidencia / estado | Qué queda |
|---|---|---|
| VPK e identidad | Instalación y ejecución confirmadas hasta 50; icono del EXE, arte e identificación de versión | Validar instalación y ejecución de 51 |
| EXE x86 → ARM | Entrada original, CRT, TEB/FS, TLS y avance por límites de ejecución comprobados | Cobertura de instrucciones y rendimiento durante ejecución sostenida |
| Win32 | Ventana lógica, callbacks originales, paint, relojes, COM y archivos observados funcionan bajo contratos limitados | Bucle de mensajes, más servicios y tratamiento completo de errores |
| Hilos y memoria | Trabajador iniciado, contexto y una reanudación temporizada; heap acotado | Planificación continua, sincronización, reciclaje de memoria y liberaciones |
| Archivos DAT | 286 entradas extraídas y verificadas: 215 base, 37 parche, 34 música | Formatos internos de personajes PAT/SCE y efectos; integración sostenida |
| Imágenes del sistema | 272 imágenes base y 50 del parche; 275 efectivas con reemplazos. Coinciden 225/226 cargas distintas capturadas | Usarlas a través de las cargas y dibujos del EXE en Vita |
| Direct3D | Root COM validado en 50. En 51 se implementan dispositivo, almacenamiento, cargas y estados iniciales | Rasterización, render targets, state blocks, presentación y validación física |
| Controles y audio | Recursos de música WAV verificados; referencias de inicialización en Windows | Implementar contratos DirectInput/DirectSound necesarios y conectarlos a Vita |
| Boot y juego | Entrada de código confirmada; boot visual pendiente | Menú real, selección, combate, retorno y estabilidad |

La carga sin coincidencia DAT es un rectángulo negro verificado. Su generación
por código del juego es una inferencia. Extraer los recursos no recupera por
sí solo la lógica ejecutable ni equivale a decompilar los archivos PAT/SCE.

## Qué aporta la iteración 51

1. Un perfil gráfico limitado a recursos de 1024×1024, con shaders y capacidades
   de rasterización desactivados. No copia los límites del driver de Windows.
2. El fallback que ya trae el EXE: dispositivo REF con procesamiento por
   software. Se reservan buffers de color/profundidad y texturas reales.
3. Carga de texturas A8R8G8B8 y A1R5G5B5, preservando los bytes y la transparencia;
   cada carga registra tamaño, pitch, hash y distribución de alpha.
4. Estados iniciales y diagnósticos por método. Las operaciones faltantes
   detienen la ejecución con su nombre. No vuelve el lote de 300 pruebas.

Es código nuevo compilado para esta prueba; su funcionamiento en hardware
queda por comprobar. El almacenamiento gráfico es una parte del renderer:
DrawPrimitiveUP y Present aún no están implementados.

## Roadmap inmediato y criterios para avanzar

| Orden | Trabajo | Evidencia necesaria para darlo por cumplido |
|---|---|---|
| 1 | Probar 51 y completar el contrato de inicialización gráfica | Dispositivo y recursos del EXE creados; siguiente frontera registrada sin fallo de ABI/watchdog |
| 2 | Cubrir las llamadas observadas de inicialización de entrada y sonido | El EXE completa su inicialización y llega a su bucle de dibujo |
| 3 | Implementar dibujo de vértices transformados y texturados | Triángulos originales, canales, alpha y muestreo coinciden con la referencia |
| 4 | Completar render targets, state blocks y salida de framebuffer a Vita | Clear/BeginScene/dibujos/EndScene/Present forman un frame real y repetible |
| 5 | Llegar al título y hacer interactivo el menú | Captura física del menú generado por el EXE; controles responden y cambian selección |
| 6 | Integrar personajes, escenarios, audio y memoria sostenida | Se inicia una partida, transcurre un round y se regresa al menú |
| 7 | Optimizar y ampliar validación | Rendimiento medido, estabilidad prolongada y pruebas posteriores en otras Vitas |

Los órdenes 2–4 pueden solaparse según las llamadas que revele el EXE. El
objetivo próximo es el menú generado por el juego en la consola. Antes de
llamarlo boot completo debemos distinguir un frame real de una imagen estática
extraída o del arte de LiveArea.

## Ruta elegida y cuánto falta

Seguimos con el EXE original traducido y las APIs reemplazadas. Los DAT ya nos
sirven para comprobar formatos, transparencias y recursos. Reescribir un motor
desde DAT/PAT/SCE sería una ruta adicional con lógica de juego aún por descifrar;
no representa un atajo demostrado.

No hay evidencia suficiente para dar un porcentaje, fecha o número de
iteraciones restante. El principal tramo pendiente para el título es ejecutar
las llamadas gráficas y presentarlas en Vita, además de los contratos de entrada
y sonido que aparezcan antes. Que una iteración llegue a un nuevo import indica
avance de compatibilidad, pero no mide cuántos subsistemas completos faltan.

## Datos a devolver de esta prueba

iteration51.log, iteration51-runtime.log, iteration51-watchdog.log, log.txt y
una foto. El build debe indicar iteration51-graphics-storage-r1 / 01.55.
Con ellos distinguiremos si el siguiente bloqueo está en dispositivo,
recursos, carga de archivos, inicialización de entrada/audio o dibujo.

Detalles de implementación: [STATUS_ITERATION51.md](STATUS_ITERATION51.md).
Datos originales: [ORIGINAL_REFERENCE_PASS_51.md](ORIGINAL_REFERENCE_PASS_51.md).
