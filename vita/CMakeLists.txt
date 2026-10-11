cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake"
        CACHE PATH "VitaSDK toolchain")
  else()
    message(FATAL_ERROR "Set VITASDK to the VitaSDK installation directory.")
  endif()
endif()

project(touhou75_vita C CXX)
include("${VITASDK}/share/vita.cmake" REQUIRED)

# A blank build type left the software pixel loops unoptimized in 75.
# Keep explicit Debug/Release choices; use strict -O2 with symbols by default.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build configuration" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(VITA_APP_NAME "Touhou 7.5 Vita - Iteration 91 Half Resolution")
set(VITA_TITLEID "T075VITA1")
set(VITA_VERSION "01.95")
# vita-mksfoex also writes NAME into the 52-byte STITLE field.
# Reserve one byte for its NUL terminator before generating the package.
string(LENGTH "${VITA_APP_NAME}" VITA_APP_NAME_BYTES)
if(VITA_APP_NAME_BYTES GREATER 51)
  message(FATAL_ERROR "VITA_APP_NAME exceeds the 51-byte STITLE limit")
endif()

set(WINVITA_ARCHIVE
  "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/winvita/build-vita/libwinx86_vita.a")
if(NOT EXISTS "${WINVITA_ARCHIVE}")
  message(FATAL_ERROR
    "WinVita ARMv7 runtime archive is missing. Run TARGET=vita third_party/winvita/build.sh first.")
endif()
add_library(winx86_vita STATIC IMPORTED GLOBAL)
set_target_properties(winx86_vita PROPERTIES IMPORTED_LOCATION "${WINVITA_ARCHIVE}")

add_executable(${PROJECT_NAME}
  src/main.cpp
  src/import_smoke.cpp
  src/service_smoke.cpp
  src/thread_smoke.cpp
  src/startup_probe.cpp
  src/worker_probe.cpp
  src/startup_services.cpp
  src/sha256.cpp
  src/diagnostic_screen.cpp
)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -fno-fast-math -ffp-contract=off)
# VitaSDK's linker script reserves this gap before the next load segment.
# vita-elf-create appends SCE module/import metadata to the code segment;
# page alignment alone can leave too little space as the executable grows.
target_link_options(${PROJECT_NAME} PRIVATE
  "LINKER:--defsym=__sce_headroom=0x10000"
  # GCC's Vita gthr-posix backend tests a weak pthread_cancel reference to
  # enable C++ mutex initialization/locking. Native sceKernelCreateThread
  # does not pull this object from libpthread.a. Force its inclusion: without
  # it, std::mutex is inert while condition_variable still calls pthread.
  "LINKER:--undefined=pthread_cancel"
  # libgcc emutls uses weak pthread_once to initialize its mutex/TLS key.
  # Retain it too, plus condition-variable cleanup; all referenced pthread
  # entry points must exist when the activation marker is present.
  "LINKER:--undefined=pthread_once"
  "LINKER:--undefined=pthread_cond_destroy"
)
target_include_directories(${PROJECT_NAME} PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/winvita/src"
)
target_link_libraries(${PROJECT_NAME}
  winx86_vita
  pthread
  m
  SceIofilemgr_stub
  SceLibKernel_stub
  SceKernelThreadMgr_stub
  SceProcessmgr_stub
  SceSysmem_stub
  SceDisplay_stub
  SceCtrl_stub
  SceAudio_stub
  ScePower_stub
  SceRtc_stub
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})
set(VITA_LIVEAREA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sce_sys" CACHE PATH
  "Local generated LiveArea assets; game artwork stays outside Git")
set(VITA_LIVEAREA_FILES)
if(EXISTS "${VITA_LIVEAREA_DIR}/livearea/contents/template.xml")
  # DAT artwork may contain visible iteration text; generated art may not.
  # Refuse stale asset metadata even when SFO/executable identity is current.
  string(REGEX MATCH "Iteration ([0-9]+)" VITA_ITERATION_MATCH "${VITA_APP_NAME}")
  set(VITA_ITERATION "${CMAKE_MATCH_1}")
  file(READ "${VITA_LIVEAREA_DIR}/livearea/contents/template.xml" LIVEAREA_TEMPLATE)
  if(NOT VITA_ITERATION OR NOT LIVEAREA_TEMPLATE MATCHES "content-rev=\"${VITA_ITERATION}\"")
    message(FATAL_ERROR "LiveArea iteration does not match ${VITA_APP_NAME}; prepare artwork for this build")
  endif()
  if(NOT EXISTS "${VITA_LIVEAREA_DIR}/provenance.json")
    message(FATAL_ERROR "Local LiveArea provenance is missing; prepare artwork for this build")
  endif()
  file(READ "${VITA_LIVEAREA_DIR}/provenance.json" LIVEAREA_PROVENANCE)
  string(REPLACE "." "\\." VITA_VERSION_REGEX "${VITA_VERSION}")
  if(NOT LIVEAREA_PROVENANCE MATCHES "\"iteration\"[ \t\r\n]*:[ \t\r\n]*${VITA_ITERATION}[, \t\r\n]"
     OR NOT LIVEAREA_PROVENANCE MATCHES "\"version\"[ \t\r\n]*:[ \t\r\n]*\"${VITA_VERSION_REGEX}\"")
    message(FATAL_ERROR "LiveArea provenance iteration/version mismatch; prepare artwork for this build")
  endif()
  foreach(ASSET icon0.png pic0.png livearea/contents/bg0.png
                livearea/contents/startup.png livearea/contents/template.xml)
    if(NOT EXISTS "${VITA_LIVEAREA_DIR}/${ASSET}")
      message(FATAL_ERROR "LiveArea asset missing: ${ASSET}")
    endif()
    list(APPEND VITA_LIVEAREA_FILES FILE "${VITA_LIVEAREA_DIR}/${ASSET}" "sce_sys/${ASSET}")
  endforeach()
else()
  message(STATUS "Local LiveArea art absent: building diagnostic package without artwork")
endif()
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME "${VITA_APP_NAME}"
  ${VITA_LIVEAREA_FILES}
)
