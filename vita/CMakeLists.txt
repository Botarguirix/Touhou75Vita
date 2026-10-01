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

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(VITA_APP_NAME "Touhou 7.5 Vita - Iteration 12 Startup Services")
set(VITA_TITLEID "T075VITA1")
set(VITA_VERSION "01.15")

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
  src/startup_services.cpp
  src/sha256.cpp
  src/diagnostic_screen.cpp
)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
# VitaSDK's linker script reserves this gap before the next load segment.
# vita-elf-create appends SCE module/import metadata to the code segment;
# page alignment alone can leave too little space as the executable grows.
target_link_options(${PROJECT_NAME} PRIVATE
  "LINKER:--defsym=__sce_headroom=0x10000"
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
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME "${VITA_APP_NAME}"
)
