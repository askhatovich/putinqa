cmake_minimum_required(VERSION 3.16)
project(putinqa LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Version: from APP_VERSION env (CI tag), or git short hash
if(DEFINED ENV{APP_VERSION})
    set(APP_VERSION "$ENV{APP_VERSION}")
else()
    execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_SHORT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(GIT_SHORT_HASH)
        set(APP_VERSION "${GIT_SHORT_HASH}")
    else()
        set(APP_VERSION "dev")
    endif()
endif()
message(STATUS "App version: ${APP_VERSION}")
add_compile_definitions(APP_VERSION="${APP_VERSION}")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Network WebSockets Widgets)

# Find libsodium: try vcpkg first (Windows), then pkg-config (Linux/macOS)
find_package(unofficial-sodium CONFIG QUIET)
if(unofficial-sodium_FOUND)
    set(SODIUM_TARGET unofficial-sodium::sodium)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SODIUM REQUIRED IMPORTED_TARGET libsodium)
    set(SODIUM_TARGET PkgConfig::SODIUM)
endif()

# Find libqrencode: try pkg-config (Linux/macOS), fall back to find_path (vcpkg/Windows)
if(PkgConfig_FOUND OR NOT WIN32)
    if(NOT PkgConfig_FOUND)
        find_package(PkgConfig REQUIRED)
    endif()
    pkg_check_modules(QRENCODE REQUIRED IMPORTED_TARGET libqrencode)
    set(QRENCODE_TARGET PkgConfig::QRENCODE)
else()
    find_path(QRENCODE_INCLUDE_DIR NAMES qrencode.h REQUIRED)
    find_library(QRENCODE_LIBRARY NAMES qrencode REQUIRED)
    add_library(qrencode::qrencode UNKNOWN IMPORTED)
    set_target_properties(qrencode::qrencode PROPERTIES
        IMPORTED_LOCATION "${QRENCODE_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${QRENCODE_INCLUDE_DIR}"
    )
    set(QRENCODE_TARGET qrencode::qrencode)
endif()

set(SOURCES
    src/main.cpp
    src/appcontroller.cpp
    src/crypto/crypto.cpp
    src/client/authorization.cpp
    src/client/serverworkload.cpp
    src/client/session/actions.cpp
    src/client/session/session.cpp
    src/client/session/sessionstate.cpp
    src/client/session/websocketconnection.cpp
)

set(HEADERS
    src/appcontroller.h
    src/crypto/crypto.h
    src/client/authorization.h
    src/client/serverworkload.h
    src/client/session/actions.h
    src/client/session/session.h
    src/client/session/sessionstate.h
    src/client/session/websocketconnection.h
)

qt6_add_resources(QML_RESOURCES src/resources.qrc)

# Windows icon resource
if(WIN32)
    set(WIN_ICON_RC src/icons/putinqa.rc)
endif()

# macOS bundle icon
if(APPLE)
    set(MACOSX_BUNDLE_ICON_FILE putinqa.icns)
endif()

add_executable(putinqa WIN32 ${SOURCES} ${HEADERS} ${QML_RESOURCES} ${WIN_ICON_RC})

if(APPLE)
    set_target_properties(putinqa PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "PutinQA"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.askhatovich.putinqa"
    )
endif()

target_include_directories(putinqa PRIVATE
    src
    src/client
    src/client/session
    src/crypto
)

target_link_libraries(putinqa PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::WebSockets
    Qt6::Widgets
    ${SODIUM_TARGET}
    ${QRENCODE_TARGET}
)
