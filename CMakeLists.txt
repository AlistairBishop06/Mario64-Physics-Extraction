cmake_minimum_required(VERSION 3.20)

project(SM64PhysicsSandbox
    VERSION 0.1.0
    DESCRIPTION "SM64 Physics Sandbox: movement physics analysis and debugging tool"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SM64PS_BUILD_APP "Build the SDL2/OpenGL/ImGui sandbox application" ON)
option(SM64PS_BUILD_TESTS "Build unit tests" ON)
option(SM64PS_BUILD_EXTRACTOR "Build the ROM asset extractor CLI" ON)
option(SM64PS_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

include(FetchContent)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

FetchContent_MakeAvailable(glm nlohmann_json)

add_library(sm64ps_project_warnings INTERFACE)
if(MSVC)
    target_compile_options(sm64ps_project_warnings INTERFACE /W4 /permissive-)
    if(SM64PS_WARNINGS_AS_ERRORS)
        target_compile_options(sm64ps_project_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(sm64ps_project_warnings INTERFACE -Wall -Wextra -Wpedantic)
    if(SM64PS_WARNINGS_AS_ERRORS)
        target_compile_options(sm64ps_project_warnings INTERFACE -Werror)
    endif()
endif()

add_library(sm64ps_core STATIC
    src/assets/RomExtractor.cpp
    src/assets/RuntimeAssets.cpp
    src/collision/CollisionWorld.cpp
    src/debug/Console.cpp
    src/debug/TweakVars.cpp
    src/mario/ActionStateMachine.cpp
    src/mario/MarioController.cpp
    src/mario/LibSm64Backend.cpp
    src/mario/MarioMath.cpp
    src/physics/FixedTimestep.cpp
    src/physics/PhysicsWorld.cpp
    src/replay/Replay.cpp
    src/util/Config.cpp
    src/util/Log.cpp
    src/util/Sha1.cpp
)

target_include_directories(sm64ps_core PUBLIC src)
target_link_libraries(sm64ps_core
    PUBLIC
        glm::glm
        nlohmann_json::nlohmann_json
        ${CMAKE_DL_LIBS}
    PRIVATE
        sm64ps_project_warnings
)

if(SM64PS_BUILD_APP)
    find_package(OpenGL REQUIRED)

    FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.30.10
    )

    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.91.6
    )

    FetchContent_MakeAvailable(SDL2 imgui)

    if(TARGET SDL2-static)
        set(SM64PS_SDL_TARGET SDL2-static)
    elseif(TARGET SDL2::SDL2-static)
        set(SM64PS_SDL_TARGET SDL2::SDL2-static)
    elseif(TARGET SDL2::SDL2)
        set(SM64PS_SDL_TARGET SDL2::SDL2)
    elseif(TARGET SDL2)
        set(SM64PS_SDL_TARGET SDL2)
    else()
        message(FATAL_ERROR "SDL2 target was not created by FetchContent")
    endif()

    add_library(sm64ps_imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    )
    target_include_directories(sm64ps_imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    target_link_libraries(sm64ps_imgui PUBLIC ${SM64PS_SDL_TARGET} OpenGL::GL)

    add_executable(sm64_physics_sandbox
        src/main.cpp
        src/SandboxApp.cpp
        src/rendering/DebugRenderer.cpp
        src/rendering/Renderer.cpp
        src/debug/DebugUI.cpp
    )
    target_link_libraries(sm64_physics_sandbox
        PRIVATE
            sm64ps_core
            sm64ps_imgui
            ${SM64PS_SDL_TARGET}
            OpenGL::GL
            sm64ps_project_warnings
    )
    target_compile_definitions(sm64_physics_sandbox PRIVATE SDL_MAIN_HANDLED)
    add_custom_command(TARGET sm64_physics_sandbox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/configs
            $<TARGET_FILE_DIR:sm64_physics_sandbox>/configs
        COMMAND ${CMAKE_COMMAND}
            -D SRC=${CMAKE_SOURCE_DIR}/third_party/libsm64/dist/sm64.dll
            -D DST=$<TARGET_FILE_DIR:sm64_physics_sandbox>/sm64.dll
            -P ${CMAKE_SOURCE_DIR}/cmake/CopyIfExists.cmake
    )
endif()

if(SM64PS_BUILD_EXTRACTOR)
    add_executable(sm64ps_asset_extractor
        assets_extractor/main.cpp
    )
    target_link_libraries(sm64ps_asset_extractor PRIVATE sm64ps_core sm64ps_project_warnings)
endif()

if(SM64PS_BUILD_TESTS)
    enable_testing()

    add_executable(sm64ps_movement_math_tests
        tests/movement_math_tests.cpp
    )
    target_link_libraries(sm64ps_movement_math_tests PRIVATE sm64ps_core sm64ps_project_warnings)
    add_test(NAME movement_math COMMAND sm64ps_movement_math_tests)

    add_executable(sm64ps_replay_tests
        tests/replay_tests.cpp
    )
    target_link_libraries(sm64ps_replay_tests PRIVATE sm64ps_core sm64ps_project_warnings)
    add_test(NAME replay_serialization COMMAND sm64ps_replay_tests)

    add_executable(sm64ps_collision_tests
        tests/collision_tests.cpp
    )
    target_link_libraries(sm64ps_collision_tests PRIVATE sm64ps_core sm64ps_project_warnings)
    add_test(NAME collision COMMAND sm64ps_collision_tests)
endif()
