# Próxima prueba: primeras instrucciones del EXE original

Este es un plan pendiente de implementación. La Iteración 10 pasó sus pruebas sintéticas en hardware; aún no ejecuta instrucciones del juego.

## Evidencia estática

Se verificó que la copia japonesa local tiene SHA-256 `BD441E99075436E8DCAD26F86FFCF5E6AAC4F58B0ED3EE7442E4CB39D8E22C98`, el mismo que verificó la Vita. El desensamblado del paquete de ingeniería inversa aportado por el usuario muestra:

| Dirección | Operación | Requisito |
| --- | --- | --- |
| `0x0064232C` | Entry point, prepara dos argumentos y llama a `0x00645414` | Pila invitada válida |
| `0x00645414` | Prólogo de registro de excepciones; lee y escribe `FS:[0]` | TEB y memoria de pila; esto no demuestra manejo de excepciones |
| `0x0064233F` | Llama a `0x00642510` con `EAX=0x94` | Rutina de ajuste/sondeo de pila |
| `0x00642349` | Escribe `0x94` al inicio de la estructura local | Buffer invitado de 148 bytes |
| `0x0064234C` | `call DWORD PTR [0x00657090]` | `KERNEL32.dll!GetVersionExA` |

`GetVersionExA` sigue marcado como `unsupported` en el log de Iteración 10. Es la primera llamada a una API importada del camino inicial mostrado. Las dos rutinas previas son internas al EXE. No se ha observado aún este recorrido en ejecución real en Vita.

## Alcance de la siguiente iteración

1. Mantener las comprobaciones de hash y los diagnósticos ya validados.
2. Preparar un contexto de arranque separado y empezar en `0x0064232C`.
3. Ejecutar un tramo limitado hasta interceptar la llamada real a `GetVersionExA`, sin devolver éxito ni continuar en esa API todavía.
4. Registrar EIP, ESP, la dirección IAT, el argumento y `dwOSVersionInfoSize=148`; comprobar que el buffer y el registro `FS:[0]` están dentro de la pila invitada.
5. Definir un límite efectivo de ejecución y recuperar la pantalla de diagnóstico también ante parada por servicio desconocido o error. El timeout de la pantalla de resultados no limita por sí solo la ejecución x86.

La primera meta verificable es confirmar en hardware la ejecución de instrucciones originales hasta esa frontera. Después se implementará y comprobará el contrato necesario de `GetVersionExA` y se seguirá con la siguiente dependencia del inicio. No se habilitará un arranque ilimitado con los 140 imports aún pendientes.

No hay en esta prueba menú, combate, D3D8, audio ni ejecución del parche inglés. No permite estimar todavía cuántas iteraciones faltan para una imagen del juego.
