cmake_minimum_required(VERSION 3.5)
project(study-cli-tests)

add_executable(${PROJECT_NAME}
        main.cxx
        )
target_include_directories(${PROJECT_NAME} PRIVATE
        $<BUILD_INTERFACE:${CMAKE_GENERATED_DIR}>
        )
#target_link_libraries(${PROJECT_NAME}
#        PRIVATE
#        cmdr11::cmdr11)
#
#target_link_libraries(${PROJECT_NAME}
#        PRIVATE
#        libs::sm-lib
#        )

add_test(NAME study-cli COMMAND $<TARGET_FILE:study-cli-tests>)
# add_test(study-cli study-cli-tests)

set_property(TEST study-cli PROPERTY LABELS monaco)
set_property(TEST study-cli APPEND PROPERTY LABELS ../../include/core11.hh)
set_property(TEST study-cli APPEND PROPERTY LABELS main.cxx)
set_tests_properties(study-cli PROPERTIES LABELS monaco)
