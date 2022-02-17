get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})
set(USER_HARDWARE_LIBRARY ${EUDAQ_USERNAME}_static)

option(USER_MPW3_BUILD "build RD50_MPW3 Support" OFF)

if(NOT USER_MPW3_BUILD)
  message(STATUS "user/rd50_mpw3 is NOT to be built (USER_MPW3_BUILD=OFF)")
  return()
else()
  message(STATUS "user/rd50_mpw3 is to be built (USER_MPW3_BUILD=ON)")
endif()

include_directories(${EUDAQ_INCLUDE_DIRS})

add_subdirectory(module)
