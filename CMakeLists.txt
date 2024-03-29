cmake_minimum_required(VERSION 3.20.4)

project(mini_light_player VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_compile_definitions(NO_ZLIB)

include(FetchContent)

#set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

configure_file(src/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/src/config.h)

file( GLOB_RECURSE BASE_SRC src/*cpp src/*h)

add_executable(${PROJECT_NAME} ${BASE_SRC} )

# add dependencies
include(cmake/CPM.cmake)

find_package(Threads REQUIRED)

CPMAddPackage("gh:nlohmann/json@3.10.5")
CPMAddPackage("gh:gabime/spdlog@1.11.0")
CPMAddPackage("gh:chriskohlhoff/asio#asio-1-28-0@1.28.0")
CPMAddPackage("gh:leethomason/tinyxml2#e05956094c27117f989d22f25b75633123d72a83")
CPMAddPackage("gh:andreacasalino/Minimal-Socket#master")

cpmaddpackage(
  NAME
  zstd
  GITHUB_REPOSITORY
  facebook/zstd
  VERSION
  1.5.2
  SOURCE_SUBDIR
  build/cmake
  OPTIONS
  "ZSTD_LEGACY_SUPPORT OFF")

cpmaddpackage(NAME zlib VERSION 1.2.12.2 GITHUB_REPOSITORY horta/zlib)

if(zstd_ADDED)
  if(NOT TARGET ZSTD::zstd)
    add_library(ZSTD::zstd ALIAS libzstd_static)
    set_target_properties(
      libzstd_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                "$<BUILD_INTERFACE:${zstd_SOURCE_DIR}/lib>")
  endif()
endif()

if(asio_ADDED)
    message(STATUS "asio ADDED")
  add_library(asio INTERFACE)

  target_include_directories(asio SYSTEM INTERFACE ${asio_SOURCE_DIR}/asio/include)

  target_compile_definitions(asio INTERFACE ASIO_STANDALONE ASIO_NO_DEPRECATED)

  target_link_libraries(asio INTERFACE Threads::Threads)

  if(WIN32)
    # macro see @ https://stackoverflow.com/a/40217291/1746503
    macro(get_win32_winnt version)
      if(CMAKE_SYSTEM_VERSION)
        set(ver ${CMAKE_SYSTEM_VERSION})
        string(REGEX MATCH "^([0-9]+).([0-9])" ver ${ver})
        string(REGEX MATCH "^([0-9]+)" verMajor ${ver})
        # Check for Windows 10, b/c we'll need to convert to hex 'A'.
        if("${verMajor}" MATCHES "10")
          set(verMajor "A")
          string(REGEX REPLACE "^([0-9]+)" ${verMajor} ver ${ver})
        endif("${verMajor}" MATCHES "10")
        # Remove all remaining '.' characters.
        string(REPLACE "." "" ver ${ver})
        # Prepend each digit with a zero.
        string(REGEX REPLACE "([0-9A-Z])" "0\\1" ver ${ver})
        set(${version} "0x${ver}")
      endif()
    endmacro()

    if(NOT DEFINED _WIN32_WINNT)
      get_win32_winnt(ver)
      set(_WIN32_WINNT ${ver})
    endif()

    message(STATUS "Set _WIN32_WINNET=${_WIN32_WINNT}")

    target_compile_definitions(asio INTERFACE _WIN32_WINNT=${_WIN32_WINNT} WIN32_LEAN_AND_MEAN)
  endif()
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE spdlog nlohmann_json::nlohmann_json asio tinyxml2::tinyxml2 MinimalSocket)

target_link_libraries(${PROJECT_NAME} PUBLIC ZLIB::zlibstatic)
target_link_libraries(${PROJECT_NAME} PUBLIC ZSTD::zstd)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
)



if(WIN32)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${BASE_SRC})

