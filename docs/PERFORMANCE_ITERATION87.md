# Recursos de la Vita — investigación para la iteración 87

## CPU

VitaSDK publica tres máscaras USER_0/1/2 y una SYSTEM, junto a su máscara
USER_ALL. Los hilos pueden fijarse a una máscara y consultar su núcleo.
La 87 usa sólo los tres núcleos de aplicación. [CPU](https://docs.vitasdk.org/kernel_2cpu_8h.html),
[hilos y afinidad](https://docs.vitasdk.org/kernel_2threadmgr_2thread_8h.html).
Consultar los headers no basta para dar por probado el scheduling físico;
por eso se registra readback y se conserva fallback serial.

El SELF añade sólo la biblioteca SceCpu con sceKernelGetCpuId: library NID
45265161, función 2704CFEE; los 13 imports previos y controles de memoria
SELF no cambian. Es un export user publicado en la base VitaSDK de firmware
3.60, no una llamada kernel/driver. [Definición oficial](https://github.com/vitasdk/vita-headers/blob/master/db/360/SceSysmem.yml).

La unidad paralelizable actual es una banda de filas de un dibujo validado.
El x86/JIT y los contextos guest siguen en un único hilo. Ejecutar el mismo
objeto Cpu desde hilos simultáneos rompería su estado compartido. Los workers
no cruzan esa frontera. No se aumentan clocks ni se ocupa el core de sistema.

## GPU: ruta investigada, implementación pendiente

El programa actual usa CPU y sceDisplay para scanout; no inicializa GXM.
GXM ofrece texturas, filtros, blend por canales y render targets; su
sincronización permite esperar antes de que la CPU lea un resultado.
[API oficial de VitaSDK](https://docs.vitasdk.org/group__SceGxmUser.html).

libvita2d es una referencia real para drawing 2D con GPU. Su fuente configura
texturas/vertices y envía dibujos mediante GXM; no constituye por sí sola
nuestro adaptador completo de D3D8 ni prueba la exactitud del port.
[Repositorio](https://github.com/xerpi/libvita2d),
[texturas/drawing](https://github.com/xerpi/libvita2d/blob/master/libvita2d/source/vita2d_texture.c).
Revisión realizada el 10 de octubre de 2026, sin incorporar código ajeno.

Plan concreto para el siguiente backend:

1. Contexto/render target GXM con formato, viewport y scanout verificados;
   conservar el renderer serial como referencia de píxeles.
2. Upload de A8R8G8B8/A1R5G5B5 al cambiar realmente una textura en UnlockRect,
   con pitch, formato y ownership conocidos, evitando subir todo en cada draw.
3. Shader para XYZRHW, diffuse y UV; half-pixel/texel center y top-left
   contrastados contra nuestros oráculos y capturas del EXE.
4. POINT/LINEAR, WRAP y modulación; alpha GREATEREQUAL antes del blend,
   SRCALPHA/INVSRCALPHA y ONE/ZERO con X8/alpha correctos.
5. State blocks, cambio de render target y recursos alias preservados.
   Lectura CPU de un render target/LockRect exige finalizar GPU y coherencia;
   el paso software/GPU no puede perder dibujos o devolver datos viejos.
6. Draws encadenados y fence al Present, evitando un bloqueo/lectura por quad;
   comparar pixel deltas, draw/frame time, memoria y respuesta del menú en Vita.

La separación de renderer en TH08-Web y los snapshots TH075 para identidades
siguen siendo referencias del proyecto: [investigación anterior](RESEARCH_EXE_ITERATION73.md).
Ninguna fuente revisada proporciona un motor TH075 Vita ya terminado.
En 87 hay offload a otros cores CPU, no offload a GPU; se mide antes de atribuir
un aumento de FPS. [Implementación, evidencia y límites](STATUS_ITERATION87.md).
