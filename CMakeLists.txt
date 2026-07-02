cmake_minimum_required (VERSION 3.13)

project(EspiAnalyzer)

include(CTest)

add_definitions( -DLOGIC2 )

option(ESPI_DEBUG_TRANSACTION_DETAILS "Emit verbose transaction metadata in analyzer text output" OFF)
if( ESPI_DEBUG_TRANSACTION_DETAILS )
	add_definitions( -DESPI_DEBUG_TRANSACTION_DETAILS )
endif()

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "Minimum supported MacOS version" FORCE)

# enable generation of compile_commands.json, helpful for IDEs to locate include files.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# custom CMake Modules are located in the cmake directory.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(ExternalAnalyzerSDK)

set(SOURCES 
src/EspiAnalyzer.cpp
src/EspiAnalyzer.h
src/EspiAnalyzerResults.cpp
src/EspiAnalyzerResults.h
src/EspiAnalyzerSettings.cpp
src/EspiAnalyzerSettings.h
src/EspiSimulationDataGenerator.cpp
src/EspiSimulationDataGenerator.h
)

add_analyzer_plugin(${PROJECT_NAME} SOURCES ${SOURCES})

if( BUILD_TESTING )
    add_executable(EspiCommandTests tests/EspiCommandTests.cpp)
    target_include_directories(EspiCommandTests PRIVATE ${PROJECT_SOURCE_DIR}/src)
    add_test(NAME EspiCommandTests COMMAND EspiCommandTests)
endif()
