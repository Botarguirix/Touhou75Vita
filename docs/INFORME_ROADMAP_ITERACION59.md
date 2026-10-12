# Roadmap y recursos — iteración 59

## Dónde vamos

| Área | Evidencia | Falta |
| --- | --- | --- |
| Ejecución original | PE, x86, CRT, ventana, hilos, archivos y contratos iniciales avanzan en Vita | Completar servicios usados después de EnumDevices |
| Recursos | Directorio 215 entradas; 72 contenedores gráficos; 7256 cabeceras de frames; exportaciones retornadas exactas | Validar todos los frames y semántica PAT/SCE/escenarios |
| Audio | Música DAT nativa fluida confirmada; puerto y objeto primario DirectSound creados por el EXE | Buffers secundarios, mezcla y reproducción controladas por el juego |
| Gráficos | Almacenamiento/estados iniciales D3D8 y visor DAT | Uploads reales, primitivas, blend, escenas y Present |
| Entrada | Pad nativo y puente teclado implementados | Verificar formato, adquisición y lectura dentro del juego |
| Juego completo | No confirmado | Menú original interactivo, combate, estabilidad y rendimiento |

La imagen del visor y la música nativa no prueban el arranque completo.
El siguiente criterio útil es un frame producido por Draw/Present del
EXE, seguido por un menú que responda a entrada.

## ¿Se puede jugar solamente con los DAT?

Los DAT aportan recursos y datos, pero falta el programa que interpreta
esos datos y ejecuta el combate. Un motor nativo podría usar únicamente
los DAT como archivos externos después de portar o reconstruir esa
lógica. Extraer imágenes/música no recupera automáticamente el motor.

Para una demo fiel necesitamos dibujo de sprites, estados de animación,
interpretación PAT, movimiento/colisiones, hitboxes/hurtboxes, ataques,
daño, temporización y estado de ronda. Una primera meta acotada sería un
escenario y dos personajes con movimiento, salto, un ataque y vida.
Implementar reglas nuevas sobre los sprites produciría una demo propia;
para conservar el juego se necesita recuperar y comprobar sus reglas.

Ya tenemos el EXE original, los archivos y referencia Windows necesarios
para continuar. No hace falta que el usuario aporte otro archivo para la
siguiente iteración. Grabaciones/mediciones Windows de acciones concretas
ayudarán cuando lleguemos al combate; ahora la prioridad es terminar el
camino de inicio y render del programa original.

## Referencias revisadas y reutilización

Hallazgo del 8 de octubre de 2026:
[N0zoM1z0/th075](https://github.com/N0zoM1z0/th075), revisión
a57b43e32b9f30dd500c730c3f78a8d897be73c5, licencia MIT. Clon local
artifacts/th075-reconstruction-reference, fuera del repositorio del port.
Su objetivo tiene exactamente nuestro SHA256
bd441e99075436e8dcad26f86ffcf5e6aac4f58b0ed3ee7442e4cb39d8e22c98.
El README declara 60 funciones con comparación exacta y 9883 bytes; el
proyecto no proporciona un motor completo. No hemos repetido su
comparación binaria ni incorporado código externo en esta entrega.

Inspección de GraphicsTexture.cpp: redondeo de dimensiones a potencias
de dos, opción de texturas cuadradas, A1R5G5B5 para 8/16 bits y A8R8G8B8
para 24/32. GraphicsTexturedDraw.cpp muestra quads XYZRHW con ajuste de
medio píxel, coordenadas UV derivadas del tamaño de superficie y
DrawPrimitiveUP TRIANGLESTRIP, dos primitivas. Esto proporciona hipótesis
concretas para contrastar con nuestro disassembly/captura Windows cuando
implementemos el dibujo; no se considera una validación local del motor.

[repentogxm](https://github.com/0xl0cal/repentogxm) sigue siendo útil para
patrones de recompilación x86, separación guest/native, entrada y audio.
Sus direcciones, lógica Isaac y formato de recursos no son TH075.
[vitaGL](https://github.com/Rinnegatamante/vitaGL) puede servir como backend
para traducir primitivas D3D8; requiere implementar sus estados y
convenciones. WinVita/Box86 ya forman nuestra ruta de ejecución.
Los lectores Twilight Frontier de arc_unpacker/thtk/arc_conv sirven como
referencia de formatos, no como sustitutos del motor de combate.

## Orden de trabajo

1. Medir la 59: respuesta del visor y siguiente frontera real del EXE.
2. Completar inicio de entrada/audio según llamadas observadas.
3. Implementar uploads y el subconjunto DrawPrimitiveUP/Present observado,
   usando la referencia Windows y las funciones reconstruidas como apoyo.
4. Mostrar el título original y validar navegación con controles.
5. Cargar selección y un escenario; comprobar animación y combate frente
   a Windows antes de declarar una demo fiel jugable.
6. Medir memoria/frame time/audio y ampliar personajes/modos.

No se asigna un porcentaje global de port ni un número prometido de
iteraciones: los contratos restantes y el rendimiento no están medidos.
