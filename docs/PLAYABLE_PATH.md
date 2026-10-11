# Recursos y ruta hacia combate jugable

## Orden de trabajo

| Fase | Evidencia necesaria | Próximo cambio |
|---|---|---|
| Menú → opción | Z entregado, escena o frontera nueva | Iteración 93: input histórico, sesión ampliada y seguimiento |
| Selección → carga | Personajes/escenario y solicitudes reales | Implementar sólo contratos/formatos observados, validar ownership |
| Combate básico | Movimiento, ataques, guardia, HUD y audio | Completar geometría/estados/API que efectivamente use el combate |
| 60 FPS | Actualizaciones y frames completos ≤16,7 ms sostenidos | GXM + uploads por cambio + hot paths x86 medidos |
| Calidad | Combate estable con margen de tiempo/memoria | Recuperar resolución y verificar transparencias/texto |

La reducción de resolución es válida como perfil provisional. La resolución
original no se recuperará antes de medir el coste de escenas y combate.
Un contador instantáneo o un menú correcto no prueba velocidad de lógica ni
jugabilidad. El objetivo se mantiene en 60 FPS; todavía no hay una estimación
fiable que garantice alcanzarlo con el runtime actual.

## Uso concreto de la reconstrucción

[N0zoM1z0/th075](https://github.com/N0zoM1z0/th075) identifica el mismo EXE 1.11
por SHA-256 y declara 60 funciones fuente con coincidencia completa. La copia
local se usa como mapa de funciones/CFG, separando roles inferidos de código
reconstruido y verificado. Su fábrica aporta infraestructura de reconstrucción,
no un motor TH075 completo listo para ARM.

En el ledger local, TitleScene::Advance es 0043A270, RenderMenu 0043A7A0 y
Initialize 0043A020. Sus roles tienen evidencia de CFG; **no constan como fuente
exacta**. Se contrastó Advance con el EXE mediante objdump: confirma mediante
el contador de input en 0067166C y despacha la selección por tabla. Story,
Arcade, Duel y Practice conducen a 2203 con diferentes valores de modo; Options
usa 2209. Las dos modificaciones de edad que desactivan attract-demo mantienen
la rama de confirmación. La 93 observa la escena sin reemplazar su lógica.

Graphics::DrawTexturedQuad (004029F0) y DrawProjectedTriangleStrip (00403200)
sí tienen fuente exacta en la referencia. Ayudan a reproducir vertices, UV,
estados y secuencia COM en un backend GXM, y a seleccionar futuras sustituciones
nativas. Una función exacta para VC7/x86 no es por sí sola un reemplazo ARM:
se deben verificar ABI, pila, FPU, ownership, llamadas y efectos laterales.

## Compresión y exportación

- El extractor local tools/extract_th075.py ya exporta texturas de DAT. El
  cuello de botella actual incluye raster CPU y coste guest; comprimir el DAT
  no elimina ese raster. Se conserva el archivo original para validación.
- [nanovg-gxm](https://github.com/xfangfang/nanovg-gxm) muestra soporte de texturas
  DXT1/DXT5 y memoria CDRAM/LPDDR. Es una referencia para el futuro backend GPU,
  no una integración nueva en la 93.
- [stb_dxt](https://github.com/nothings/stb/blob/master/stb_dxt.h) comprime bloques
  RGBA 4×4 a DXT1/DXT5. Puede servir para preparar una caché **en PC**, verificando
  calidad y alpha por textura. DXT es con pérdida y no conserva automáticamente
  el pixel oracle actual. Evitar recompresión por frame en la Vita.
- [Basis Universal](https://github.com/BinomialLLC/basis_universal) ofrece codecs
  de texturas y transcodificación a formatos GPU. Añade trabajo de preparación
  y necesita un backend compatible. No se asume soporte nativo de Basis en GXM.
- Para recursos dinámicos, cachear la versión GPU y actualizar sólo al cambiar
  Lock/Unlock, UpdateTexture, CopyRects o render target. Las superficies que el
  EXE pueda leer necesitan coherencia/readback, referencias y presupuesto real.

## Análisis del EXE y trazas PC

[Ghidra](https://github.com/NationalSecurityAgency/ghidra) permite desensamblar,
decompilar y revisar funciones/CFG. Ya hay exportador local para el arranque.
[apitrace](https://github.com/apitrace/apitrace) registra llamadas gráficas;
el proyecto dispone del wrapper Win32 D3D8. Una traza original de Practice y
combate permite contrastar llamadas, recursos, blend y filtros con el port.
Ninguna de estas herramientas convierte automáticamente todo el EXE a ARM.

Para GPU se usarán [GXM de VitaSDK](https://docs.vitasdk.org/group__SceGxmUser.html)
y [libvita2d](https://github.com/xerpi/libvita2d) como referencias de contextos,
render targets, shaders y sincronización. La documentación de
[SceCtrl](https://docs.vitasdk.org/group__SceCtrlUser.html) respalda Peek no
bloqueante de hasta 64 muestras con timestamp usado en la 93.

Primero recoger la transición real. Después integrar quads GPU con comparación
de capturas y perfil de tiempos; finalmente optimizar funciones guest calientes
que sigan fuera del presupuesto. [Mediciones de 60 FPS](PERFORMANCE_60FPS.md).
