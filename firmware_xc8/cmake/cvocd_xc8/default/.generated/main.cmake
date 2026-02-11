include("${CMAKE_CURRENT_LIST_DIR}/rule.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/file.cmake")

set(cvocd_xc8_default_library_list )

# Handle files with suffix (s|as|asm|AS|ASM|As|aS|Asm), for group default-XC8
if(cvocd_xc8_default_default_XC8_FILE_TYPE_assemble)
add_library(cvocd_xc8_default_default_XC8_assemble OBJECT ${cvocd_xc8_default_default_XC8_FILE_TYPE_assemble})
    cvocd_xc8_default_default_XC8_assemble_rule(cvocd_xc8_default_default_XC8_assemble)
    list(APPEND cvocd_xc8_default_library_list "$<TARGET_OBJECTS:cvocd_xc8_default_default_XC8_assemble>")

endif()

# Handle files with suffix S, for group default-XC8
if(cvocd_xc8_default_default_XC8_FILE_TYPE_assemblePreprocess)
add_library(cvocd_xc8_default_default_XC8_assemblePreprocess OBJECT ${cvocd_xc8_default_default_XC8_FILE_TYPE_assemblePreprocess})
    cvocd_xc8_default_default_XC8_assemblePreprocess_rule(cvocd_xc8_default_default_XC8_assemblePreprocess)
    list(APPEND cvocd_xc8_default_library_list "$<TARGET_OBJECTS:cvocd_xc8_default_default_XC8_assemblePreprocess>")

endif()

# Handle files with suffix [cC], for group default-XC8
if(cvocd_xc8_default_default_XC8_FILE_TYPE_compile)
add_library(cvocd_xc8_default_default_XC8_compile OBJECT ${cvocd_xc8_default_default_XC8_FILE_TYPE_compile})
    cvocd_xc8_default_default_XC8_compile_rule(cvocd_xc8_default_default_XC8_compile)
    list(APPEND cvocd_xc8_default_library_list "$<TARGET_OBJECTS:cvocd_xc8_default_default_XC8_compile>")

endif()


# Main target for this project
add_executable(cvocd_xc8_default_image_lunv0ofc ${cvocd_xc8_default_library_list})

set_target_properties(cvocd_xc8_default_image_lunv0ofc PROPERTIES
    OUTPUT_NAME "default"
    SUFFIX ".elf"
    ADDITIONAL_CLEAN_FILES "${output_extensions}"
    RUNTIME_OUTPUT_DIRECTORY "${cvocd_xc8_default_output_dir}")
target_link_libraries(cvocd_xc8_default_image_lunv0ofc PRIVATE ${cvocd_xc8_default_default_XC8_FILE_TYPE_link})

# Add the link options from the rule file.
cvocd_xc8_default_link_rule( cvocd_xc8_default_image_lunv0ofc)


