# Iteración 69 — Render Targets / 01.73

## Evidencia física de la 68

BeginStateBlock atendido: slot 52, cleanup 8 y HRESULT cero. La siguiente llamada
fue slot 40 (trap BFC280), que el bridge rotulaba UnimplementedSlot y que es
SetViewport según IDirect3DDevice8. Se mantuvieron 9654 llamadas principales,
2484 D3D, 199 DirectSound, seis DInput, 30 reanudaciones y 13313848 us. No CPU fault
ni límite. BGM terminó correctamente: 943 bloques y un late_fill_block (no es
medición directa de underrun). El game log conserva LoadEffect ...OK.

La secuencia del propio EXE en 00609C86..00609D70 es BeginStateBlock, SetViewport,
EndStateBlock, CaptureStateBlock, GetRenderTarget/GetDepthStencilSurface,
SetRenderTarget, SetViewport y BeginScene. Se prepara esta familia en conjunto;
no se presume que todas las llamadas se ejecutarán o aprobarán en hardware.
Referencia ABI: https://raw.githubusercontent.com/wine-mirror/wine/master/include/d3d8.h

## Contratos preparados

SetViewport/GetViewport usan los seis DWORD de D3DVIEWPORT8 (X/Y/Width/Height y
MinZ/MaxZ como float). Valida tamaños no cero, límites y rango de profundidad
finito 0..1. En grabación admite el límite de recurso 1024; fuera de ella comprueba
el destino activo. El activo inicial corresponde al backbuffer 640x480. En modo
grabación la nueva viewport se conserva en el bloque sin sustituir la activa;
Apply/Capture se amplían para incluir su máscara y valores.

SetRenderTarget resuelve superficies hijas de texturas y usa sus datos padre,
valida superficie color renderable A8R8G8B8/X8R8G8B8 y profundidad D16 suficiente.
Color NULL conserva el destino actual; profundidad NULL desactiva depth. Los
bindings poseen referencias y liberan el pin anterior al cambiar. El destino
inicial obtiene pins separados de su reserva de almacenamiento. Se reinicia
viewport al tamaño del destino. GetDepthStencilSurface sin depth devuelve NULL
con D3DERR_NOTFOUND. Los recursos siguen sin destruirse en su último Release,
que conserva la frontera de ownership ya existente.

Clear resuelve también el almacenamiento de superficies hijas y admite ambos
formatos de color; profundidad solo es obligatoria cuando flags incluye Z.
No se presenta el resultado en pantalla ni se implementan triángulos/texturas
rasterizados. DrawPrimitiveUP/Present siguen siendo fronteras explícitas.

## Paquete y siguiente captura

VitaSDK compiló sin advertencias; diff --check limpio. ZIP CRC,
SFO 01.73/T075VITA1 y cuatro PNG indexados sin entrelazado: aprobados.
No se ejecutaron nuevas pruebas unitarias; contratos nuevos pendientes en Vita.
Build iteration69-render-targets-r1.
VPK artifacts/iteration69/Touhou75Vita-iteration69.vpk.
SHA256: b99d48573d337c5d48eba13072a511f44f6992dc8f9fd90d3076d38425cd1a11.

Enviar iteration69.log, iteration69-runtime.log, iteration69-watchdog.log,
iteration69-bgm.log, log.txt y foto. Revisar viewport request, stateblock end,
render_target, clear y startup_stop_import. Controles, música, heap reutilizable
y límites de ejecución se conservan. La pantalla original y gameplay permanecen
pendientes; el PASS del diagnóstico no indica que el juego haya booteado.
