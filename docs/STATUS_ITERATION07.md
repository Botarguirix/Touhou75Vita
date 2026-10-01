# Iteración 07 — PE japonés y prueba del dynarec ARMv7

## Objetivo

Integrar una revisión fijada de WinVita, mapear la imagen PE32 japonesa en la memoria invitada y ejecutar un fragmento x86 de control a través del recompilador dinámico ARMv7. La app no ejecuta el entry point ni callbacks TLS del juego o del parche inglés.

## Alcance de la prueba

- Lee `ux0:data/TH075Vita/TH075.exe`; requiere el tamaño conocido de 2.576.384 bytes. El SHA-256 esperado se registra, pero esta versión no calcula ni verifica el hash en Vita.
- Usa `d2rt::PeImage` para mapear cabeceras y secciones en una imagen PE en memoria, conserva la base preferida y enumera imports por DLL. La resolución de imports todavía no se intenta.
- Copia la imagen PE al espacio de memoria invitado del backend ARMv7.
- El dynarec ejecuta únicamente `mov eax, 0x75; jmp trap`. La prueba pasa si se detiene en la trampa y EAX conserva `0x75`.
- `TH075.exe` no se ejecuta; el parche `TH075E.exe`, sus DLL y datos tampoco se cargan.

## Dependencia fijada

Se integra [Franckrst/WinVita](https://github.com/Franckrst/WinVita) en la revisión `8065b469da1b65b1f968024b881ef187e7f7fef7`, con su `LICENSE` y `THIRD-PARTY-NOTICES.md`. Su `build.sh` compila el runtime genérico y el dynarec Box86 para ARMv7. El workflow de GitHub Actions quedó configurado para producir `Touhou75Vita-iteration07-armv7-dynarec-smoke-vpk` después de subir los cambios.

## Validación pendiente

La VPK debe compilarse en Actions, instalarse en la PS Vita y dejar `ux0:data/TH075Vita/iteration07.log`. Un resultado satisfactorio incluye:

```text
build_id=iteration07-winvita-armv7-smoke-r1
game_pe_load_result=loaded_by_winvita_peimage
game_guest_map_result=passed
dynarec_backend=Box86-derived-ARMv7
dynarec_smoke_result=passed
game_entrypoint=not_attempted
game_code_executed=no
```

El resultado de CI demuestra compilación cruzada; solo el log de la consola demuestra que el dynarec alcanzó la rutina de control en hardware. La prueba no demuestra que los imports de Touhou funcionen ni que el juego sea jugable. Los siguientes bloqueos son la auditoría del cargador/imports, las APIs Win32 que Touhou usa y un backend de Direct3D 8 para Vita.

## Identidad y archivos

- SHA-256 esperado de `TH075.exe`: `BD441E99075436E8DCAD26F86FFCF5E6AAC4F58B0ED3EE7442E4CB39D8E22C98`.
- Los archivos comerciales del juego y del parche deben seguir fuera de Git.
