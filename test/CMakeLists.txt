
cmake_minimum_required(VERSION 3.2)

set(TEST_NAMES
    chain
)

add_custom_target(check COMMAND echo "Running all")

foreach(case ${TEST_NAMES})
  add_executable("test_${case}" "test_${case}.cpp")
  target_link_libraries("test_${case}" matrixChain)
  target_link_libraries("test_${case}" gtest_main)

  add_custom_target("check-${case}" COMMAND "test_${case}")
  add_dependencies(check "check-${case}")
endforeach()
