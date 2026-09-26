cmake_minimum_required(VERSION 3.20)
project(iamp_vita C)

if(NOT DEFINED ENV{VITASDK})
  message(FATAL_ERROR "VITASDK is not set. Install VitaSDK and export VITASDK=/path/to/vitasdk before configuring.")
endif()

set(VITASDK "$ENV{VITASDK}")
set(CMAKE_TOOLCHAIN_FILE "${VITASDK}/share/vita.toolchain.cmake" CACHE FILEPATH "VitaSDK toolchain")

add_executable(iamp_vita.elf
  src/main.c
  ../runtime/iamp_semantic_runtime.c
  ../generated/guest_semantic.c
)
target_include_directories(iamp_vita.elf PRIVATE ../runtime)
target_link_libraries(iamp_vita.elf m)

add_custom_command(TARGET iamp_vita.elf POST_BUILD
  COMMAND ${VITASDK}/bin/vita-make-fself $<TARGET_FILE:iamp_vita.elf> ${CMAKE_CURRENT_BINARY_DIR}/iamp_vita.self
  COMMAND ${VITASDK}/bin/vita-pack-vpk -s PARAM.SFO -b ${CMAKE_CURRENT_BINARY_DIR}/iamp_vita.self -a ${CMAKE_CURRENT_BINARY_DIR}/iamp_vita.vpk=iamp_vita.self
  VERBATIM)
