cmake_minimum_required(VERSION 3.20)

project(motionpad C RC)

if(NOT WIN32)
    message(FATAL_ERROR "motionpad is a Windows-only application.")
endif()

if(MSVC)
    set(MOTIONPAD_COMPILER_KIND "msvc")
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(MOTIONPAD_COMPILER_KIND "gcc")
elseif(CMAKE_C_COMPILER_ID STREQUAL "TinyCC")
    set(MOTIONPAD_COMPILER_KIND "tcc")
else()
    message(
        FATAL_ERROR
        "Unsupported C compiler '${CMAKE_C_COMPILER_ID}'. Supported compilers: MSVC, GNU, TinyCC."
    )
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(MOTIONPAD_SOURCES
    app.rc
    src/app_context.c
    src/device_manager.c
    src/input_actions.c
    src/logger.c
    src/main.c
    src/move_input.c
    src/navigator_input.c
    src/telemetry.c
    src/profile.c
    src/profile_watcher.c
    src/rumble_manager.c
    src/tray_ui.c
    src/vigem_manager.c
)

set(GENERATED_IMPORT_LIB_DIR "${CMAKE_CURRENT_BINARY_DIR}/import-libs")
file(MAKE_DIRECTORY "${GENERATED_IMPORT_LIB_DIR}")

function(motionpad_add_gnu_import_library target_name def_file output_name)
    if(NOT DLLTOOL_EXECUTABLE)
        message(FATAL_ERROR "dlltool was not found in PATH.")
    endif()

    set(output_path "${GENERATED_IMPORT_LIB_DIR}/${output_name}")
    add_custom_command(
        OUTPUT "${output_path}"
        COMMAND "${DLLTOOL_EXECUTABLE}" -d "${CMAKE_CURRENT_SOURCE_DIR}/${def_file}" -l "${output_path}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${def_file}"
        COMMENT "Generating import library ${output_name}"
        VERBATIM
    )

    add_custom_target("${target_name}" DEPENDS "${output_path}")
endfunction()

function(motionpad_add_msvc_import_library target_name def_file output_name)
    if(NOT MSVC_LIB_EXECUTABLE)
        message(FATAL_ERROR "MSVC lib.exe was not found in PATH.")
    endif()

    set(output_path "${GENERATED_IMPORT_LIB_DIR}/${output_name}")
    add_custom_command(
        OUTPUT "${output_path}"
        COMMAND "${MSVC_LIB_EXECUTABLE}"
            "/def:${CMAKE_CURRENT_SOURCE_DIR}/${def_file}"
            "/machine:x86"
            "/out:${output_path}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${def_file}"
        COMMENT "Generating import library ${output_name}"
        VERBATIM
    )

    add_custom_target("${target_name}" DEPENDS "${output_path}")
endfunction()

set(MOTIONPAD_LINK_LIBRARIES)
set(MOTIONPAD_LINK_ITEMS)
set(MOTIONPAD_COMPILE_DEFINITIONS)
set(MOTIONPAD_COMPILE_OPTIONS)
set(MOTIONPAD_LINK_OPTIONS)
set(MOTIONPAD_IMPORT_TARGETS)

if(MOTIONPAD_COMPILER_KIND STREQUAL "gcc")
    find_program(DLLTOOL_EXECUTABLE NAMES dlltool x86_64-w64-mingw32-dlltool i686-w64-mingw32-dlltool)
    motionpad_add_gnu_import_library(motionpad_psmoveapi_import lib/libpsmoveapi.def libpsmoveapi.a)
    motionpad_add_gnu_import_library(motionpad_navinput_import lib/libnavinput.def libnavinput.a)
    motionpad_add_gnu_import_library(motionpad_vigem_import lib/ViGEmClient.def libViGEmClient.a)

    list(APPEND MOTIONPAD_IMPORT_TARGETS
        motionpad_psmoveapi_import
        motionpad_navinput_import
        motionpad_vigem_import
    )
    list(APPEND MOTIONPAD_LINK_LIBRARIES psmoveapi ViGEmClient navinput user32 shell32 ole32 uuid)
    list(APPEND MOTIONPAD_COMPILE_OPTIONS -O2 -Wall -Wextra)
elseif(MOTIONPAD_COMPILER_KIND STREQUAL "msvc")
    find_program(MSVC_LIB_EXECUTABLE NAMES lib)
    if(NOT MSVC_LIB_EXECUTABLE AND CMAKE_AR)
        set(MSVC_LIB_EXECUTABLE "${CMAKE_AR}")
    endif()
    motionpad_add_msvc_import_library(motionpad_psmoveapi_import lib/libpsmoveapi.def psmoveapi.lib)
    motionpad_add_msvc_import_library(motionpad_navinput_import lib/libnavinput.def navinput.lib)
    motionpad_add_msvc_import_library(motionpad_vigem_import lib/ViGEmClient.def ViGEmClient.lib)

    list(APPEND MOTIONPAD_IMPORT_TARGETS
        motionpad_psmoveapi_import
        motionpad_navinput_import
        motionpad_vigem_import
    )
    list(APPEND MOTIONPAD_LINK_LIBRARIES psmoveapi ViGEmClient navinput user32 shell32 ole32 uuid)
    list(APPEND MOTIONPAD_COMPILE_DEFINITIONS _CRT_SECURE_NO_WARNINGS)
    list(APPEND MOTIONPAD_COMPILE_OPTIONS
        /W4
        "$<$<CONFIG:Release>:/O2>"
        "$<$<CONFIG:RelWithDebInfo>:/O2>"
        "$<$<CONFIG:MinSizeRel>:/O1>"
    )
elseif(MOTIONPAD_COMPILER_KIND STREQUAL "tcc")
    list(APPEND MOTIONPAD_LINK_OPTIONS
        "-L${CMAKE_CURRENT_SOURCE_DIR}/lib"
        -lpsmoveapi
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/libpsmoveapi.def"
        -lViGEmClient
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/ViGEmClient.def"
        -lnavinput
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/libnavinput.def"
        -luser32
        -lshell32
        -Wl,-subsystem=windows
    )
endif()

add_executable(motionpad WIN32 ${MOTIONPAD_SOURCES})

if(MOTIONPAD_IMPORT_TARGETS)
    add_dependencies(motionpad ${MOTIONPAD_IMPORT_TARGETS})
endif()

target_include_directories(
    motionpad
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/psmoveapi"
)

target_compile_definitions(motionpad PRIVATE ${MOTIONPAD_COMPILE_DEFINITIONS})
target_compile_options(motionpad PRIVATE ${MOTIONPAD_COMPILE_OPTIONS})
target_link_options(motionpad PRIVATE ${MOTIONPAD_LINK_OPTIONS})

if(MOTIONPAD_IMPORT_TARGETS)
    target_link_directories(motionpad PRIVATE "${GENERATED_IMPORT_LIB_DIR}")
elseif(NOT MOTIONPAD_COMPILER_KIND STREQUAL "tcc")
    target_link_directories(motionpad PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib")
endif()

target_link_libraries(motionpad PRIVATE ${MOTIONPAD_LINK_LIBRARIES} ${MOTIONPAD_LINK_ITEMS})

set_target_properties(
    motionpad
    PROPERTIES
        OUTPUT_NAME "motionpad"
)

if(CMAKE_CONFIGURATION_TYPES)
    foreach(config_name IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER "${config_name}" config_name_upper)
        set_target_properties(
            motionpad
            PROPERTIES
                "RUNTIME_OUTPUT_DIRECTORY_${config_name_upper}" "${CMAKE_CURRENT_BINARY_DIR}/bin/${config_name}"
        )
    endforeach()
else()
    set_target_properties(
        motionpad
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
    )
endif()

set(MOTIONPAD_RUNTIME_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/ViGEmClient.dll"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/libnavinput.dll"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/libpsmoveapi.dll"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/settings.ini"
	"${CMAKE_CURRENT_SOURCE_DIR}/runtime/psmove.exe"
)

foreach(runtime_file IN LISTS MOTIONPAD_RUNTIME_FILES)
    add_custom_command(
        TARGET motionpad
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${runtime_file}"
            "$<TARGET_FILE_DIR:motionpad>"
        VERBATIM
    )
endforeach()

add_custom_command(
    TARGET motionpad
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E rm -f
        "$<TARGET_FILE_DIR:motionpad>/profile.ini"
    COMMAND "${CMAKE_COMMAND}" -E rm -f
        "$<TARGET_FILE_DIR:motionpad>/profiles/profile.ini"
    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "$<TARGET_FILE_DIR:motionpad>/profiles"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/profiles/default.ini"
        "$<TARGET_FILE_DIR:motionpad>/profiles/default.ini"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/profiles/default - xinput only.ini"
        "$<TARGET_FILE_DIR:motionpad>/profiles/default - xinput only.ini"
    VERBATIM
)
