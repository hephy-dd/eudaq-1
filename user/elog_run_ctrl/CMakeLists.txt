get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

option(USER_ELOG_RC_BUILD "build user/example" ON)

if(NOT USER_ELOG_RC_BUILD)
    message(STATUS "user/elog_run_control is NOT to be built (USER_ELOG_RC_BUILD=OFF)")
  return()
endif()

message(STATUS "user/elog_run_control is to be built (USER_ELOG_RC_BUILD=ON)")

add_subdirectory(module)

