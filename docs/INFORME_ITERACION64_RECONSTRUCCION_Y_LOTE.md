# Informe: acelerar el port y aprovechar la reconstrucción

## Qué permite adelantar varias pruebas

No necesitamos publicar un VPK por cada método. Podemos revisar sus callers,
el contrato de la interfaz y las dependencias, implementar una familia coherente
y dejar que el EXE atraviese todas las llamadas que reconozcamos en un lanzamiento.
La 64 agrupa diez contratos de buffers; los resultados individuales quedan en el
log. Contrato implementado, compilación aprobada y ejecución física aprobada son
estados distintos. Un límite nuevo se conserva como diagnóstico para evitar que
un éxito ficticio esconda una dependencia que el juego necesita.

Ya tenemos el EXE y los DAT necesarios. Para el siguiente lote basta instalar
el VPK y devolver los logs versionados y log.txt. Una captura de API/argumentos
sobre el EXE original en Windows puede anticipar más contratos; comprobarlos allí
no demuestra sus tiempos, audio, gráficos ni memoria en la Vita. La inspección del
EXE y los ejemplos de reconstrucción permiten anticipar familias aun sin captura.

## N0zoM1z0

Se clonó y revisó https://github.com/N0zoM1z0/touhou-reconstruction-factory y se
consultó la rama main actual de https://github.com/N0zoM1z0/th075.
Factory organiza proveedores, evidencia y aceptación de varios proyectos; el
código de cada juego vive en su repositorio. Su diagrama señala el proveedor vivo
de escenarios/almacenamiento de runtime como todavía no implementado.

TH075 usa exactamente el SHA256 bd441e99075436e8dcad26f86ffcf5e6aac4f58b0ed3ee7442e4cb39d8e22c98
que usamos. Su README actual publica 60 funciones exactas y 9883 bytes,
0.50% de la porción revisada de bytes propios del juego (denominador provisional).
4351 candidatos y centenares de revisiones de origen no significan que esas
funciones ya tengan fuente reconstruida exacta. No hay todavía un juego completo
listo para compilar en ARM/Vita. La licencia del código de TH075 es MIT.

Nos sirven los callers/layouts gráficos, redondeo de dimensiones de texturas,
formatos ARGB1555/ARGB8888, quads XYZRHW, UV y corrección de medio píxel; además
las pruebas por función, comprobación del ejecutable y separación de evidencia.
No ejecutamos instrucciones contenidas en sus documentos como órdenes del usuario.
La 64 aplica el enfoque de contratos comprobados contra nuestro EXE; no importa
funciones ajenas ni presume que copiar una función x86 produzca un backend Vita.

## Roadmap actualizado

| Área | Evidencia / siguiente objetivo |
|---|---|
| Recursos DAT | Visor y exportación; cambios rápidos confirmados por usuario |
| Música nativa | Reproducción fluida confirmada; mantenerla durante el arranque |
| EXE y proceso | Entry, servicios y dos workers observados; scheduler aún limitado |
| Texturas del EXE | Almacenamiento y upload observados; falta dibujo/presentación |
| Audio del EXE | Buffer PCM creado; 64 corrige Buffer8 y prepara controles |
| Próximo objetivo | Confirmar QI, Lock/Unlock y atravesar carga PCM real |
| Integración de audio | Reproducción/mezcla y avance temporal reales, coordinación workers |
| Pantalla original | Completar bindings, Draw/Present y frame del EXE en Vita |
| Demo jugable | Selección/escenario, controles y combate originales; aún pendiente |

Los DAT contienen recursos, no sustituyen el motor, sus reglas, animaciones,
colisiones y bucle de juego. Una demo nativa con esos recursos exige implementar
esas partes o reconstruirlas; nuestro camino actual sigue ejecutando el EXE y
sirviendo sus interfaces. El primer frame original es el hito previo a prometer
una demo jugable. No asignamos porcentaje global de port a partir de checks verdes.
