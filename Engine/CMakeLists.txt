cmake_minimum_required(VERSION 3.16)

project(Tempest)

set(CMAKE_CXX_STANDARD 20)

option(TEMPEST_BUILD_SHARED "Build shared Tempest." ON)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
set(CMAKE_OSX_DEPLOYMENT_TARGET 12.0)

if(MSVC)
  option(TEMPEST_BUILD_DIRECTX12   "Build directx12 support" ON )
else()
  option(TEMPEST_BUILD_DIRECTX12   "Build directx12 support" OFF)
endif()

if(APPLE)
  option(TEMPEST_BUILD_VULKAN      "Build vulkan support"    OFF)
  option(TEMPEST_BUILD_METAL       "Build metal support"     ON )
else()
  option(TEMPEST_BUILD_VULKAN      "Build vulkan support"    ON )
  option(TEMPEST_BUILD_METAL       "Build metal support"     OFF)
endif()

if(IOS)
  set(TEMPEST_BUILD_SHARED OFF)
endif()

# This warning only occurs in CMake 3.24 or greater.
# https://cmake.org/cmake/help/latest/policy/CMP0135.html#cmp0135
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24")
  cmake_policy(SET CMP0135 NEW)
endif()

# Utils
function(target_append_redist_file target_name src_path)
  add_custom_command(
      TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND}
      -E copy_if_different "${src_path}"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${target_name}")
endfunction()

option(TEMPEST_BUILD_AUDIO "Build openal sound support" ON)

### The Library
if(TEMPEST_BUILD_SHARED)
  add_library(${PROJECT_NAME} SHARED)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
else()
  add_library(${PROJECT_NAME} STATIC)
endif()

### Compilers
if(MSVC)
  add_definitions(-D_USE_MATH_DEFINES)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
  add_definitions(-DNOMINMAX)
  add_compile_options(/FS)
endif()

if(APPLE)
  add_definitions(-D_LARGEFILE64_SOURCE)
endif()

### zlib
set(ZLIB_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(ZLIB_LIBRARY        zlibstatic)
set(ZLIB_INCLUDE_DIR    "thirdparty/zlib")
add_subdirectory("thirdparty/zlib" EXCLUDE_FROM_ALL)
target_include_directories(${PROJECT_NAME} PRIVATE "thirdparty/zlib")
if(NOT MSVC)
  target_compile_options(zlibstatic PRIVATE -Wno-deprecated-non-prototype)
endif()

### libpng16
set(PNG_SHARED                 OFF CACHE INTERNAL "")
set(PNG_STATIC                 ON  CACHE INTERNAL "")
set(PNG_TESTS                  OFF CACHE INTERNAL "")
set(PNG_BUILD_ZLIB             OFF CACHE INTERNAL "")
set(SKIP_INSTALL_ALL           ON  CACHE INTERNAL "")
set(PNG_HARDWARE_OPTIMIZATIONS OFF CACHE INTERNAL "")
add_definitions(-DPNG_ARM_NEON_OPT=0)
add_subdirectory("thirdparty/libpng" EXCLUDE_FROM_ALL)
target_include_directories(png_static PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(png_static PRIVATE "thirdparty/zlib")
target_include_directories(${PROJECT_NAME} PRIVATE "thirdparty/libpng")
target_link_libraries(${PROJECT_NAME} PRIVATE png_static)

### stb
add_definitions(-DSTB_IMAGE_IMPLEMENTATION)
add_definitions(-DSTB_TRUETYPE_IMPLEMENTATION)
add_definitions(-DSTB_IMAGE_WRITE_IMPLEMENTATION)

### OpenAl
if(TEMPEST_BUILD_AUDIO)
  add_definitions(-DTEMPEST_BUILD_AUDIO)
  if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
      set(ALSOFT_BACKEND_WAVE     ON  CACHE INTERNAL "")
      set(ALSOFT_BACKEND_OSS      ON  CACHE INTERNAL "")
      set(ALSOFT_REQUIRE_OSS      ON  CACHE INTERNAL "")
      set(ALSOFT_REQUIRE_QSA      OFF CACHE INTERNAL "")
      set(ALSOFT_REQUIRE_SOLARIS  OFF CACHE INTERNAL "")
      set(ALSOFT_BACKEND_ALSA     OFF CACHE INTERNAL "")
      set(ALSOFT_DLOPEN           OFF CACHE INTERNAL "")
      set(ALSOFT_NO_CONFIG_UTIL   OFF CACHE INTERNAL "")
  endif()
  set(AL_LIBTYPE_STATIC   ON  CACHE INTERNAL "")
  set(ALSOFT_EXAMPLES     OFF CACHE INTERNAL "")
  set(ALSOFT_UTILS        OFF CACHE INTERNAL "")
  set(ALSOFT_BUILD_ROUTER OFF CACHE INTERNAL "")
  set(LIBTYPE "STATIC") # TODO: any proper way to do this?!
  add_definitions(-DAL_ALEXT_PROTOTYPES)
  add_subdirectory("thirdparty/openal-soft" EXCLUDE_FROM_ALL)
  target_include_directories(${PROJECT_NAME} PRIVATE "thirdparty/openal-soft/include")
  target_link_libraries(${PROJECT_NAME} PRIVATE OpenAL)
  if(IOS)
    target_compile_options(OpenAL PRIVATE -Wno-nullability-completeness)
  endif()
  if(NOT MSVC)
    target_compile_options(OpenAL PRIVATE -Wno-attributes -Wno-conversion -Wno-switch -Wno-unused-but-set-variable)
  endif()
  if(NOT MSVC AND NOT APPLE AND NOT ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    target_compile_options(OpenAL PRIVATE -Wno-stringop-overflow)
  endif()
  if(MSVC)
    target_compile_options(OpenAL PRIVATE /wd4060)
  endif()
endif()

### Vulkan
if(TEMPEST_BUILD_VULKAN)
  add_definitions(-DTEMPEST_BUILD_VULKAN)
  add_definitions(-DVULKAN_HPP_NO_EXCEPTIONS)
  add_definitions(-DVULKAN_HPP_NO_SMART_HANDLE)

  if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    target_link_directories(${PROJECT_NAME} PRIVATE "$ENV{VULKAN_SDK}/lib")
  else()
    target_link_directories(${PROJECT_NAME} PRIVATE "$ENV{VULKAN_SDK}/Lib32")
  endif()

  target_include_directories(${PROJECT_NAME} PRIVATE "$ENV{VULKAN_SDK}/include")
  if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE vulkan-1)
  else()
    target_link_libraries(${PROJECT_NAME} PRIVATE vulkan)
  endif()
endif()

### Directx12
if(WIN32 AND TEMPEST_BUILD_DIRECTX12)
  add_definitions(-DTEMPEST_BUILD_DIRECTX12)

  include(FetchContent)
  FetchContent_Declare(DirectXShaderCompiler
      URL "https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.7.2212.1/dxc_2023_03_01.zip"
      URL_HASH SHA256=E4E8CB7326FF7E8A791ACDA6DFB0CB78CC96309098BFDB0AB1E465DC29162422)
  FetchContent_MakeAvailable(DirectXShaderCompiler)

  if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    set(TARGET_ARCH "x64")
  else()
    set(TARGET_ARCH "x86")
  endif()

  #message("directxshadercompiler_SOURCE_DIR = ${directxshadercompiler_SOURCE_DIR}")
  target_include_directories(${PROJECT_NAME} PRIVATE "${directxshadercompiler_SOURCE_DIR}/inc")
  target_link_directories(${PROJECT_NAME} PRIVATE "${directxshadercompiler_SOURCE_DIR}/lib/${TARGET_ARCH}")
  target_link_libraries(${PROJECT_NAME} PRIVATE dxcompiler dxgi)
  target_append_redist_file("dxcompiler.dll" "${directxshadercompiler_SOURCE_DIR}/bin/${TARGET_ARCH}/dxcompiler.dll")
  target_append_redist_file("dxil.dll"       "${directxshadercompiler_SOURCE_DIR}/bin/${TARGET_ARCH}/dxil.dll")
endif()

### Metal
if(TEMPEST_BUILD_METAL)
  add_definitions(-DTEMPEST_BUILD_METAL)
  add_subdirectory("thirdparty/metal-cpp" EXCLUDE_FROM_ALL)
  target_include_directories(${PROJECT_NAME} PRIVATE "thirdparty/metal-cpp")
endif()

### Spirv-cross
set(SPIRV_CROSS_CLI            OFF CACHE INTERNAL "")
set(SPIRV_CROSS_ENABLE_UTIL    OFF CACHE INTERNAL "")
set(SPIRV_CROSS_ENABLE_TESTS   OFF CACHE INTERNAL "")
set(SPIRV_CROSS_ENABLE_REFLECT OFF CACHE INTERNAL "")
set(SPIRV_CROSS_STATIC         ON  CACHE INTERNAL "")
set(SPIRV_CROSS_ENABLE_MSL     ON  CACHE INTERNAL "")
add_subdirectory("thirdparty/spirv_cross" EXCLUDE_FROM_ALL)
target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-core)
target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-glsl)
target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-hlsl)
target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-msl)

### squish
add_subdirectory("thirdparty/squish" EXCLUDE_FROM_ALL)
target_link_libraries(${PROJECT_NAME} PRIVATE squish-tempest)
set_target_properties(squish-tempest PROPERTIES CXX_VISIBILITY_PRESET hidden)

### Font
set(GEN_FONTS_HEADER "${PROJECT_BINARY_DIR}/builtin_fonts.h")
add_subdirectory("fonts" EXCLUDE_FROM_ALL)
target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_BINARY_DIR}/fonts")
target_link_libraries(${PROJECT_NAME} PRIVATE fonts-tempest)

### Shaders
set(GEN_SHADERS_HEADER "${PROJECT_BINARY_DIR}/sprv/builtin_shader.h")

file(GLOB SHADER_SOURCES
  "${PROJECT_SOURCE_DIR}/shaders/*.vert"
  "${PROJECT_SOURCE_DIR}/shaders/*.frag"
  "${PROJECT_SOURCE_DIR}/shaders/*.comp")

# GLSL to SPIRV compiler
find_program(GLSLANGVALIDATOR glslangValidator "/opt/homebrew/bin")
if(NOT GLSLANGVALIDATOR)
  message(FATAL_ERROR "glslangValidator required")
endif()

function(add_shader OUTPUT SOURCE)
  set(OUTPUT_FILE "${PROJECT_BINARY_DIR}/sprv/${OUTPUT}")
  set(SOURCE_FILE "${PROJECT_SOURCE_DIR}/shaders/${SOURCE}")
  set(SHADERS      ${SHADERS}      ${SOURCE_FILE} PARENT_SCOPE)
  set(SHADERS_SPRV ${SHADERS_SPRV} ${OUTPUT_FILE} PARENT_SCOPE)

  list(SUBLIST ARGV 2 -1 VARS)
  add_custom_command(
    OUTPUT     ${OUTPUT_FILE}
    DEPENDS    ${SOURCE_FILE}
    COMMAND    ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/sprv/"
    COMMAND    ${GLSLANGVALIDATOR} -V ${VARS} "${SOURCE_FILE}" -o ${OUTPUT_FILE}
    )
endfunction(add_shader)

add_shader(blit.vert.sprv      blit.vert "")
add_shader(blit.frag.sprv      blit.frag "")

add_shader(empty.vert.sprv     brush.vert "")
add_shader(empty.frag.sprv     brush.frag "")
add_shader(tex_brush.vert.sprv brush.vert -DTEXTURE)
add_shader(tex_brush.frag.sprv brush.frag -DTEXTURE)

add_shader(copy.comp.sprv      copy.comp  "")
add_shader(copy.s.comp.sprv    copy.comp  -DFRM_SMALL)

add_shader(mesh_init.comp.sprv         mesh_init.comp  "")
add_shader(task_post_pass.comp.sprv    task_post_pass.comp  "")
add_shader(task_lut_pass.comp.sprv     task_lut_pass.comp  "")
add_shader(mesh_prefix_pass.comp.sprv  mesh_prefix_pass.comp  "")
add_shader(mesh_compactage.comp.sprv   mesh_compactage.comp   "")

add_custom_command(
  OUTPUT     ${GEN_SHADERS_HEADER}
  DEPENDS    ${SHADERS_SPRV}
  COMMAND    ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/sprv/"
  COMMAND    ${CMAKE_COMMAND} -P "${PROJECT_SOURCE_DIR}/shaders/link_shaders.cmake"
  WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
  )

add_custom_target(shaders
  DEPENDS ${GEN_SHADERS_HEADER}
  SOURCES ${SHADER_SOURCES} "${PROJECT_SOURCE_DIR}/shaders/link_shaders.cmake")
target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_BINARY_DIR}/sprv")
add_dependencies(${PROJECT_NAME} shaders)

file(GLOB_RECURSE SOURCES
  "2d/*.h"
  "2d/*.cpp"
  "exceptions/*.h"
  "exceptions/*.cpp"
  "formats/*.h"
  "formats/*.cpp"
  "gapi/*.h"
  "gapi/*.cpp"
  "graphics/*.h"
  "graphics/*.cpp"
  "io/*.h"
  "io/*.cpp"
  "libspirv/*.h"
  "libspirv/*.cpp"
  "math/*.h"
  "math/*.cpp"
  "sound/*.h"
  "sound/*.cpp"
  "system/*.h"
  "system/*.cpp"
  "ui/*.h"
  "ui/*.cpp"
  "utility/*.h"
  "utility/*.cpp"
  )

if(APPLE OR IOS)
  set(CMAKE_OSX_DEPLOYMENT_TARGET 12.0)
  enable_language(OBJCXX)
  file(GLOB_RECURSE ObjCSOURCES
    "*.mm"
    "**/*.mm")
endif()

# FIXME: the headers include some SOURCES files so installing them is useless
# the files in include should be the headers themselves!
file(GLOB_RECURSE PUB_HEADERS "include/Tempest/**")

### Sources
target_sources(${PROJECT_NAME} PRIVATE ${GEN_SHADERS_HEADER} ${SOURCES} ${ObjCSOURCES})
include_directories("." "include")

set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    PUBLIC_HEADER "${PUB_HEADERS}"
    )

if(WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE shlwapi Kernel32)
elseif(IOS)
  #NOTE: use -DCMAKE_SYSTEM_NAME=iOS on first configure
  target_link_libraries(${PROJECT_NAME} PRIVATE "-framework UiKit" "-framework Foundation" "-framework QuartzCore" "-framework Metal")
elseif(APPLE)
  target_link_libraries(${PROJECT_NAME} PRIVATE "-framework AppKit" "-framework QuartzCore" "-framework Metal")
elseif(UNIX)
  target_link_libraries(${PROJECT_NAME} PRIVATE X11 Xcursor)
endif()

install(
    TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include/${PROJECT_NAME}
    )
