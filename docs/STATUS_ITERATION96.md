# Iteración 96 — zona horaria Win32 desde el RTC de Vita

## Punto de partida físico 95

La selección de personajes ya se muestra en Practice y Story. Ambas capturas
terminan al confirmar en KERNEL32!GetTimeZoneInformation. El único juego de
logs disponible contiene 188 Present, 189 esperas, cuatro Cross/Z y 339 draws
de área cero aceptados. No hay rechazo de memoria ni geometría, ni límite CPU;
el watchdog quedó desarmado. Combate todavía no observado.

El buffer solicitado es 0068E498, IAT 006571E8, trap 00B006E0. El nuevo
contrato atiende esa consulta sin cambiar el EXE original en disco.

## Implementación 96

- GetTimeZoneInformation se incorpora al router KERNEL32 de StartupServices
  con un argumento y limpieza stdcall de 8 bytes. El epílogo compartido con
  las pruebas conserva registros no volátiles y verifica retorno/stack.
- Se toma un tick UTC, se convierte ese mismo tick a hora local con el RTC
  de Vita y se consulta su resolución. El bias se calcula en minutos con
  el signo Win32: UTC = local + bias. Se rechazan diferencias no integrales
  en minutos y offsets fuera del perfil acotado de ±24 horas, usando
  diferencias unsigned para evitar overflow/narrowing a partir de ticks.
- TIME_ZONE_INFORMATION se serializa explícitamente como 172 bytes PE32;
  LONG=32 bits, WCHAR=16 bits. Los nombres pueden estar vacíos. Las fechas
  de transición y biases adicionales quedan en cero, y el éxito retorna
  TIME_ZONE_ID_UNKNOWN=0: política de **offset actual fijo sin reglas DST**.
  No se representa una tabla anual ni histórica de horario de verano.
- Se valida el buffer guest completo, se escribe y se comprueban sus 172
  bytes por readback. LastError se conserva al tener éxito. Fallos RTC o
  de offset retornan TIME_ZONE_ID_INVALID con ERROR_GEN_FAILURE y salida
  intacta; un fallo de memoria/readback/publicación de LastError es una
  frontera de contrato antes de continuar el EXE.
- El log identifica rtc_rc, estado, buffer, bias, retorno y política, además
  de comprobar el ABI. No usa la zona horaria del PC ni una constante UTC
  inventada cuando falla el reloj.

[Contrato Win32](https://learn.microsoft.com/en-us/windows/win32/api/timezoneapi/nf-timezoneapi-gettimezoneinformation)
· [Layout](https://learn.microsoft.com/en-us/windows/win32/api/timezoneapi/ns-timezoneapi-time_zone_information)
· [RTC de VitaSDK](https://docs.vitasdk.org/group__SceRtcUser.html).

Se agregan sólo dos funciones oficiales de la biblioteca user SceRtcUser:
sceRtcConvertUtcToLocalTime (NID 1282C436) y sceRtcGetTickResolution (811313B3),
verificadas en la base 3.60 SceDriverUser.yml del SDK instalado. Sus otras
funciones y las otras catorce bibliotecas conservan sus imports. La biblioteca
sigue siendo 6704D3C6; no se usa SceRtcForDriver/kernel.

Se conservan 96 MiB gráficos/128 MiB heap, raster serial 320×240, audio/input,
menú directo sin logo/fade y soporte de área cero de la 95. LiveArea/icono
con Alice vuelven a salir del DAT original, con el mismo centrado. No se
activa el pool raw ni se modifica el renderer, clocks, JIT o preset del EXE.

## Comprobaciones locales

168 grupos C++ y 11 pruebas Python, ASan/UBSan y comparaciones O0/O2. Diez
grupos nuevos prueban layout/endian/UTF-16, offsets positivos/negativos/cero
y cuartos de hora, límites/ticks extremos, fallos RTC, output de la traza
física, epílogo CPU real, registros/flags/LastError, punteros inválidos, write
y readback fallidos, publicación de error y último span completo del guest.

Build Vita y auditoría de ZIP/SFO/SELF, segmentos/headroom, memoria, PNG8 y
NIDs de imports comprobados. Los diez fingerprints del preset siguen iguales.
No se ejecutó el EXE completo en PC ni se verificó la continuación física 96.

## Paquete

- Title ID **T075VITA1**, versión **02.00**.
- Nombre **Touhou 7.5 Vita - Iteration 96 Time Zone**.
- Build **iteration96-time-zone-r1**.
- VPK local artifacts/iteration96/Touhou75Vita-iteration96.vpk.
- SHA-256: fae7be0a029832d13e2e3039d320e1d00578ce9b56f6c26c6cc04f713ef21377.

## Prueba física siguiente

1. Instalar el VPK y comprobar nombre/diagnóstico **ITERATION 96**.
2. Entrar a Practice, confirmar personajes/escenario y observar si carga
   combate; si aparece, probar movimiento y un ataque.
3. Exportar iteration96.log, iteration96-runtime.log e
   iteration96-watchdog.log; guardar esa corrida en una carpeta Practice.
4. Repetir Story y guardar sus logs por separado, incluyendo log.txt,
   captura, personajes, escenario y acciones. Los nombres se reutilizan.

Se mantiene la sesión acotada a 1800 Present/300 s y watchdog 330 s. Resolver
zona horaria permite observar la siguiente solicitud; no garantiza que no
haya más fronteras de carga/render. Combate y 60 FPS siguen pendientes.

[Roadmap](ROADMAP_AFTER_SELECTION95.md) · [Métricas](PERFORMANCE_60FPS.md).
