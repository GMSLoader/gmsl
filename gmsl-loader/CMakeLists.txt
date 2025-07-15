add_library(gmsl-loader SHARED
    "src/dllmain.cpp"
    "res/version.def"
)

set_target_properties(gmsl-loader PROPERTIES OUTPUT_NAME "version")

add_subdirectory(vendor/minhook)
target_link_libraries(gmsl-loader PRIVATE minhook "${CMAKE_CURRENT_SOURCE_DIR}/vendor/nethost/nethost.lib")

add_custom_command(TARGET gmsl-loader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${OutDir}/gmsl/mods"
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:gmsl-loader> ${OutDir}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/vendor/nethost/nethost.dll "${OutDir}"
)