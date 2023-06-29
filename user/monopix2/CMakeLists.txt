get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})
set(USER_HARDWARE_LIBRARY ${EUDAQ_USERNAME}_static)

option(USER_MONOPIX2_BUILD "build MONOPIX2 support" OFF)

if(NOT USER_MONOPIX2_BUILD)
  message(STATUS "user/monopix2 is NOT to be built (USER_MONOPIX2_BUILD=OFF)")
  return()
else()
  message(STATUS "user/monopix2 is to be built (USER_MONOPIX2_BUILD=ON)")
endif()

include_directories(${EUDAQ_INCLUDE_DIRS})

add_subdirectory(module)
