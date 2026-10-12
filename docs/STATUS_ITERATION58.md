# Iteración 58 — DAT Browser / 01.62

## Evidencia física y continuidad del EXE

El usuario confirmó que la música de la 57 se escucha fluida. El log
registró 657 bloques en 31.058780 segundos, preparación máxima 21022 us,
ningún bloque de preparación tardía y cierre/drenaje/liberación correctos.
Cambios aleatorios y reinicios quedaron registrados. Se conserva esa ruta.

La 58 atiende la petición observada de buffer primario DirectSound:
descriptor de 36 bytes, flags 00040001, sin memoria PCM secundaria, formato
nulo y sin agregación. Crea un objeto propio con vtable de 21 métodos y
lectura de comprobación, respaldado por el puerto nativo ya abierto.
Implementa QueryInterface para IUnknown/IDirectSoundBuffer y referencias
COM con pin del padre. Los restantes métodos mantienen una frontera
explícita: no se simula Play, Lock o SetFormat. La siguiente llamada real
del EXE se conocerá con el log de la 58.

## Visor nativo

El DAT principal contiene 215 entradas. El inventario identifica 72
contenedores gráficos: 16 del sistema/efectos, 36 fondos y 20 de personajes.
Sus cabeceras contienen 7256 fotogramas. Este recuento no implica que todos
los fotogramas estén validados en hardware o que haya modelos 3D.
El decodificador de referencia en Windows también decodificó correctamente
el primer fotograma de cada uno de los 72 contenedores; no se comprobó
en esta pasada la decodificación completa de los 7256 fotogramas.

- Círculo: siguiente contenedor gráfico del DAT principal.
- Derecha/izquierda: siguiente/anterior fotograma del contenedor actual.
- Start: exportar el DAT seleccionado y el fotograma actual en BMP.
- Triángulo/cuadrado/X: cambiar música, reiniciarla y salir, respectivamente.

El lector valida directorio, cabeceras, extensiones, límites y pares RLE.
Decodifica 8/16/24/32 bits, con primera paleta para imágenes indexadas,
RGB555 para 16 bits y color key negro para 24 bits. Presenta una imagen
proporcionada dentro de un recuadro con tablero de transparencia y muestra
archivo/índice. Solo guarda el fotograma actual en RAM. El límite es
4096 por lado y cuatro millones de píxeles; entradas no compatibles se
registran como tales, sin fingir una imagen o ejecutar datos como código.

La exportación va a `ux0:data/TH075Vita/extracted/`, con nombres planos
derivados del recurso. Conserva el DAT original completo y escribe un
BMP V4 de 32 bits con máscaras RGBA y filas de arriba hacia abajo. Repetir
Start sobre el mismo recurso reemplaza la copia generada. No modifica
los archivos originales. Los resultados y nombres aparecen en el log.
La exportación es síncrona en el hilo de pantalla; falta comprobar su
respuesta y la continuidad musical durante operaciones grandes en Vita.

Los .pat, .sce, stage*.dat, cardlist.dat y musicroom.dat se inventariaron,
pero no se presentan como imágenes. Su semántica necesita el análisis del
motor. La vista y los BMP son recursos de diagnóstico, no un frame del EXE.

## Entrega y validación pendiente

VitaSDK compiló sin advertencias después de corregir el encabezado de
archivos y dos advertencias de indentación. CRC ZIP, SFO 01.62/T075VITA1
y cuatro PNG de LiveArea indexados de 8 bits sin entrelazado comprobados.
Build ID: iteration58-dat-browser-r1.
VPK: artifacts/iteration58/Touhou75Vita-iteration58.vpk.
SHA256: cc1593273d3b660f255a66372173f25737a3b6b5febac0fbfb71b15c84eb0929.

Devolver iteration58.log, iteration58-runtime.log, iteration58-watchdog.log,
iteration58-bgm.log, log.txt y foto. Comprobar Circle, frames y Start;
devolver al menos un DAT/BMP exportado para compararlo con el original.
Revisar `startup_dsound_primary` y la nueva frontera del EXE, así como
`dat_view_inventory`, `dat_view_frame` y `dat_view_export`.

El roadmap inmediato sigue en el contrato de audio del EXE y después
uploads, Draw/Present y callbacks necesarios para un menú interactivo.
