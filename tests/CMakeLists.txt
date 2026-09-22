add_executable(abc3d_tests
    test_mesh.cpp
    test_fuzz.cpp
    test_io.cpp
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