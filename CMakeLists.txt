cmake_minimum_required(VERSION 3.16)
project(FilterCam LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Aceita uma instalacao propria ou o pacote oficial preparado por configurar.ps1.
if(NOT OpenCV_DIR AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.deps/opencv/build/OpenCVConfig.cmake")
    set(OpenCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.deps/opencv/build")
endif()
find_package(OpenCV 4 REQUIRED COMPONENTS core imgproc imgcodecs highgui videoio)

add_library(processamento src/Processamento.cpp)
target_include_directories(processamento PUBLIC src ${OpenCV_INCLUDE_DIRS})
target_link_libraries(processamento PUBLIC ${OpenCV_LIBS})

add_executable(grau_b src/main.cpp src/Aplicativo.cpp)
target_link_libraries(grau_b PRIVATE processamento)
if(MSVC)
    target_compile_options(processamento PRIVATE /W4 /utf-8)
    target_compile_options(grau_b PRIVATE /W4 /utf-8)
endif()

# Como no Zelda, os recursos ficam ao lado do executavel.
get_property(MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(MULTI_CONFIG)
    set(PASTA_EXECUCAO "${CMAKE_BINARY_DIR}/$<CONFIG>")
else()
    set(PASTA_EXECUCAO "${CMAKE_BINARY_DIR}")
endif()
add_custom_target(recursos ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets" "${PASTA_EXECUCAO}/assets")
add_dependencies(grau_b recursos)
set_target_properties(grau_b PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PASTA_EXECUCAO}"
    VS_DEBUGGER_WORKING_DIRECTORY "${PASTA_EXECUCAO}")

if(WIN32)
    # O pacote oficial MSVC distribui o OpenCV em uma DLL compartilhada.
    get_filename_component(OPENCV_CONFIG_ROOT "${OpenCV_DIR}" ABSOLUTE)
    file(GLOB OPENCV_RUNTIME_DLLS "${OPENCV_CONFIG_ROOT}/x64/vc*/bin/*.dll")
    foreach(DLL IN LISTS OPENCV_RUNTIME_DLLS)
        add_custom_command(TARGET grau_b POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DLL}" "$<TARGET_FILE_DIR:grau_b>")
    endforeach()
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(teste_processamento tests/teste_processamento.cpp)
    target_link_libraries(teste_processamento PRIVATE processamento)
    if(MSVC)
        target_compile_options(teste_processamento PRIVATE /W4 /utf-8)
    endif()
    add_test(NAME processamento COMMAND teste_processamento "${CMAKE_CURRENT_SOURCE_DIR}/assets")
endif()
