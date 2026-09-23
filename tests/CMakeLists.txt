add_executable(abc3d_tests
    test_mesh.cpp
    test_fuzz.cpp
    test_io.cpp
    test_math.cpp
    test_selection.cpp
)

target_link_libraries(abc3d_tests PRIVATE
    abc3d_core
    abc3d_flags
    GTest::gtest_main
)

#Grab path at compile time so tests don't depend on directory
target_compile_definitions(abc3d_tests PRIVATE
    ABC3D_MODELS_DIR="${CMAKE_SOURCE_DIR}/assets/models"
)

add_test(NAME unit COMMAND abc3d_tests)

# The ASan runtime DLL lives in the MSVC bin folder, which is only on PATH inside a
# developer prompt. Copy it next to the exe so the tests also run from VS and plain shells.
if(ABC3D_ASAN AND MSVC)
    get_filename_component(MSVC_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    add_custom_command(TARGET abc3d_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MSVC_BIN_DIR}/clang_rt.asan_dynamic-x86_64.dll"
            "${MSVC_BIN_DIR}/clang_rt.asan_dbg_dynamic-x86_64.dll"
            "$<TARGET_FILE_DIR:abc3d_tests>"
    )

    # MSVC 14.38 (VS 17.8) ASan fails to hook some newer Windows 11 DLLs and aborts
    # before main(). Updating Visual Studio should remove the need for this.
    set(ASAN_ENV "ASAN_WIN_CONTINUE_ON_INTERCEPTION_FAILURE=1")
    set_tests_properties(unit PROPERTIES ENVIRONMENT "${ASAN_ENV}")
    set_property(TARGET abc3d_tests PROPERTY VS_DEBUGGER_ENVIRONMENT "${ASAN_ENV}")
endif()