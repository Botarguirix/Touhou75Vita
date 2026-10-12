# Iteración 60 — D3D Module / 01.64

## Resultado físico de la 59

El usuario confirmó cambios de imagen más rápidos y música correcta.
La sesión registró 1660 bloques de audio; preparación máxima 140544 us,
19 preparaciones tardías, drenaje/liberación correctos. La 58 registró 95
preparaciones tardías. Las sesiones no son un benchmark controlado y la
métrica no mide underruns directamente.

La nueva instrumentación sitúa decodificación entre unos 210 ms y 1041 ms
en los frames visitados, composición de vista entre 46 y 97 ms. Queda
trabajo en la primera carga. El BMP selectchar/6 enviado coincide píxel
por píxel, incluido alfa, con la referencia (512x512).

El log del juego completó DirectInputInit, DirectSoundInit, JoystickInit
y KeyboardInit y entró en SystemDataInit. La frontera final observada fue
GetModuleHandleA("d3d8.dll"), retorno 0060EB48, dentro del helper de
diagnóstico de D3DX en 0060EB24. El disassembly consulta d3d8.dll y
d3d8d.dll, incrementa su carga si están presentes y busca DebugSetMute;
comprueba el puntero antes de llamar. No hay frame original presentado.

## Contrato añadido

- GetModuleHandleA reconoce el módulo virtual d3d8 enlazado mediante IAT:
  handle opaco AB1200, distinto del kernel32 AB1000; no es un PE de DLL.
- LoadLibraryA para ese módulo devuelve el mismo handle y registra un
  incremento de referencias. No carga ni ejecuta bibliotecas Windows.
- La variante debug d3d8d no está instalada: devuelve NULL y error 126.
- GetProcAddress reconoce únicamente DebugSetMute para este handle y
  devuelve NULL/error 127 porque el backend no expone ese diagnóstico
  opcional. El helper del EXE tiene un fallback sin esa llamada.
- Otros módulos/exports mantienen una frontera explícita. FreeLibrary
  y descargas todavía no están implementados; si aparecen deben atenderse
  con el recuento de referencias, nunca descargar el backend enlazado.

Se conserva la ruta de música confirmada y la caché del visor. Cuando
izquierda/derecha conduce al mismo frame (contenedor con una sola imagen),
se omite la decodificación repetida; la 59 gastó casi un segundo cada vez
en load.dat/logo.dat sin cambiar la imagen. No se pierde la exportación.

## Comprobación en Vita

Instalar versión 01.64, build iteration60-d3d-module-r1. Mantener los DAT
y el EXE existentes. Controles conservados: Circle contenedor; izquierda/
derecha frame; Start exportación; Triangle música aleatoria; Square
reinicio; X salida. Devolver iteration60.log, iteration60-runtime.log,
iteration60-watchdog.log, iteration60-bgm.log, log.txt y fotografía.

Buscar startup_module_backend, startup_module_availability,
startup_export_availability y la siguiente startup_stop_import. El
objetivo sigue siendo uploads reales y Draw/Present para el título
original. Esta entrega continúa el arranque, no es una demo jugable.

## Paquete

VitaSDK compiló sin advertencias. CRC ZIP, SFO 01.64/T075VITA1 y cuatro PNG
indexados de 8 bits sin entrelazado inspeccionados. VPK:
artifacts/iteration60/Touhou75Vita-iteration60.vpk.
SHA256: cbde35bb59dbca147c9dea483544780791ba0e173a4975e71d5c2cfb31576bdb.
Validación del nuevo camino del EXE pendiente en hardware.

