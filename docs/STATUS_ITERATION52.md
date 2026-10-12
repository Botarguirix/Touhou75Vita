# Iteración 52 / versión 01.56 — corrección del despacho gráfico

## Resultado físico de la iteración 51

Los tres logs del usuario confirman creación del dispositivo REF/software,
backbuffer de 640×480, profundidad D16 y una textura A8R8G8B8 de 1024×1024.
El almacenamiento reservado era de 6 037 504 bytes. Se sirvieron siete cambios
iniciales de render state con ABI válida; la ejecución duró 5 773 462 µs, no
agotó el presupuesto y desarmó el watchdog.

El resultado final fue service_contract_failed en 00BFC3F0, slot 63. Aunque
el log y la pantalla lo llamaban GetTextureStageState, el EXE estaba haciendo
SetTextureStageState. Los valores 1, 4, 2 y finalmente 0 se interpretaron como
direcciones de salida. Los seis primeros retornos mal despachados no son
evidencia de estados correctamente configurados ni de cargas de texturas.
El fallo está en el puente implementado, no en los archivos originales.

## Corrección

Índices revisados contra la declaración primaria IDirect3DDevice8 de Wine:

| Método | Slot correcto | Contrato |
|---|---:|---|
| CreateRenderTarget | 25 | Sigue siendo frontera sin implementar |
| CreateDepthStencilSurface | 26 | `this` y cinco argumentos; limpieza total 28 bytes |
| GetTexture | 60 | Sigue siendo frontera sin implementar |
| SetTexture | 61 | Sigue siendo frontera sin implementar |
| GetTextureStageState | 62 | `this`, stage, type, puntero de salida |
| SetTextureStageState | 63 | `this`, stage, type, valor; sin escritura de salida |
| ValidateDevice | 64 | Sigue siendo frontera sin implementar |

Los índices corregidos tienen nombres compartidos por diagnóstico, lectura
de argumentos y despacho. SetTextureStageState modifica únicamente el estado
interno; cada registro indica output_write:none. GetTextureStageState lee
estados previamente configurados, valida la salida y registra dirección/valor.
No inventa un valor cero para estados iniciales que aún no se implementaron.

La escritura de resultados de D3D8Storage rechaza direcciones inferiores a
00010000 y rangos que excedan la arena de 32 MiB antes de llamar Cpu::write.
El backend comprueba límites de arena, pero eso no identifica un pequeño
entero usado por error como puntero. Este guard evita repetir ese error.

La implementación de profundidad se trasladó del slot 25 al 26 y conserva
su firma de cinco argumentos además de `this`. Era otro defecto de índices,
aunque la ejecución física 51 no llegó a ejercerlo.

## Alcance y entrega

Es una iteración centrada en corregir el contrato y continuar el arranque.
DrawPrimitiveUP, Present y el boot visual siguen pendientes. La creación
de una textura vacía confirma asignación, no que se hayan cargado imágenes.
No se reactivó el lote de 300 pruebas ni se aumentaron los presupuestos.

VPK local: artifacts/iteration52/Touhou75Vita-iteration52.vpk.
Build ID: iteration52-graphics-state-r1. TITLE_ID: T075VITA1.
VitaSDK compiló sin advertencias. Inspección: CRC ZIP, SFO 01.56 y cuatro
PNG indexados de 8 bits sin entrelazado correctos. La ejecución en Vita
queda pendiente; no se afirma que ya haya superado el bloqueo físico.

SHA256: 48a07583a636a4cb560a4b787314b12c7c31a023207236df8c2d81e28934b9ff.

Instalar en la misma Vita y devolver iteration52.log,
iteration52-runtime.log, iteration52-watchdog.log, log.txt y foto.
Buscar startup_d3d8_stage_set, output_write:none, ABI y siguiente
startup_stop_import. El siguiente import o método concreto aún no está
confirmado en hardware.

Referencia primaria de ABI:
[Wine d3d8.h](https://raw.githubusercontent.com/wine-mirror/wine/master/include/d3d8.h).
