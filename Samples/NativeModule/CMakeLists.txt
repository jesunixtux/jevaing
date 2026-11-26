add_library(JevaingBaseModule SHARED Module.c)
add_library(JevaingSmokeModule SHARED Module.c)
target_compile_features(JevaingBaseModule PRIVATE c_std_11)
target_compile_features(JevaingSmokeModule PRIVATE c_std_11)
target_compile_definitions(JevaingBaseModule PRIVATE MODULE_ID=1)
target_compile_definitions(JevaingSmokeModule PRIVATE MODULE_ID=2)
target_link_libraries(JevaingBaseModule PRIVATE JevaingApi)
target_link_libraries(JevaingSmokeModule PRIVATE JevaingApi)
set_target_properties(JevaingBaseModule JevaingSmokeModule PROPERTIES DEBUG_POSTFIX "")

add_executable(JevaingModuleSmokeTest ModuleSmokeTest.cpp)
target_link_libraries(JevaingModuleSmokeTest PRIVATE JevaingModules)
add_dependencies(JevaingModuleSmokeTest JevaingBaseModule JevaingSmokeModule)

foreach(target JevaingBaseModule JevaingSmokeModule JevaingModuleSmokeTest)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/bin/Debug"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/bin/Release")
endforeach()

foreach(manifest BaseModule SmokeModule BadAbi BadPath CycleA CycleB)
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/bin/$<CONFIG>/${manifest}.jevmod"
        INPUT "${CMAKE_CURRENT_SOURCE_DIR}/${manifest}.jevmod")
endforeach()
