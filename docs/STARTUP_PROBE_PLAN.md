# Recorrido inicial del EXE original

La Iteración 11 R2 confirmó en hardware el primer recorrido de este plan: entrada original, pila, FS/SEH y primera llamada importada. R1 había fallado al crear el watchdog; R2 lo armó y desarmó correctamente. Véase [evidencia](hardware/iteration11-r2/result-excerpt.txt). La [Iteración 12](STATUS_ITERATION12.md) continúa dos contratos más y queda pendiente de compilación y prueba en Vita.

## Evidencia estática

Se verificó que la copia japonesa local tiene SHA-256 `BD441E99075436E8DCAD26F86FFCF5E6AAC4F58B0ED3EE7442E4CB39D8E22C98`, el mismo que verificó la Vita. El desensamblado del paquete de ingeniería inversa aportado por el usuario muestra:

| Dirección | Operación | Requisito |
| --- | --- | --- |
| `0x0064232C` | Entry point, prepara dos argumentos y llama a `0x00645414` | Pila invitada válida |
| `0x00645414` | Prólogo de registro de excepciones; lee y escribe `FS:[0]` | TEB y memoria de pila; esto no demuestra manejo de excepciones |
| `0x0064233F` | Llama a `0x00642510` con `EAX=0x94` | Rutina de ajuste/sondeo de pila |
| `0x00642349` | Escribe `0x94` al inicio de la estructura local | Buffer invitado de 148 bytes |
| `0x0064234C` | `call DWORD PTR [0x00657090]` | `KERNEL32.dll!GetVersionExA` |

`GetVersionExA` es la primera API importada de este recorrido; las dos rutinas previas son internas al EXE. Iteración 11 R2 confirmó en Vita la IAT `0x00657090`, retorno `0x00642352`, tamaño 148 y registro SEH esperado.

El desensamblado de la misma copia japonesa prevé el siguiente recorrido para Iteración 12:

| Dirección | Operación | Comprobación |
| --- | --- | --- |
| `0x00642352` a `0x00642391` | Lee la estructura de versión y llena globals | Plataforma 2, build 2600, versión combinada `0x501`, major 5 y minor 1 |
| `0x0064239F` | Llama a `GetModuleHandleA(NULL)` por IAT `0x006570A8` | Retorno `0x006423A1` y módulo real `0x00400000` |
| `0x006423A1` en adelante | Examina las cabeceras DOS/PE del módulo | Debe recorrer las cabeceras reales cargadas |
| `0x00649746` | Llama a `HeapCreate` por IAT `0x00657160` | Retorno `0x0064974C`, argumentos `(0, 4096, 0)` |

## Alcance de Iteración 12

1. Mantener las comprobaciones de hash y los diagnósticos ya validados.
2. Preparar un contexto de arranque separado y empezar en `0x0064232C`.
3. Servir la estructura ANSI observada de `GetVersionExA` y `GetModuleHandleA(NULL)` y reanudar las instrucciones originales después de ambas llamadas.
4. Comprobar retornos stdcall, consumo de la estructura de versión por el EXE, registro `FS:[0]` y la pila al detenerse antes de `HeapCreate`. Cualquier import o contrato inesperado se registra y detiene la ejecución.
5. Aplicar un presupuesto aproximado de 512 entradas a bloques del dynarec y un watchdog nativo de cinco segundos. Las paradas cooperativas recuperan la pantalla de diagnóstico; un timeout duro cierra la aplicación y deja un log separado. El timeout de la pantalla de resultados no limita por sí solo la ejecución x86.

La siguiente meta verificable es confirmar en hardware que los dos primeros contratos permiten continuar hasta el heap del juego. Después se implementará ese servicio y se seguirá la dependencia que indique el recorrido real. Los 140 imports pendientes del preflight siguen requiriendo desarrollo; estos dos contratos nuevos pertenecen a la fase de arranque.

No hay en esta prueba menú, combate, D3D8, audio ni ejecución del parche inglés. No permite estimar todavía cuántas iteraciones faltan para una imagen del juego.
