cmake_minimum_required(VERSION 3.1.0)
include(CMakeDependentOption)
find_package(Qt5Widgets CONFIG)

cmake_dependent_option(EUDAQ_BUILD_ELOG_RUN_GUI "Compile GUI executables (requires QT5)" ON
  "Qt5Widgets_FOUND" OFF)

if(NOT EUDAQ_BUILD_ELOG_RUN_GUI)
    message(STATUS "GUI of executable (euElogRun) is NOT to be built (EUDAQ_BUILD_ELOG_RUN_GUI=OFF)")
  return()
endif()
message(STATUS "GUI of executable (euElogRun) is to be built (EUDAQ_BUILD_ELOG_RUN_GUI=ON)")

find_package(Qt5Widgets REQUIRED)
message(STATUS "Qt5Widgets: ${Qt5Widgets_DIR}")
set(EXTERNAL_LIB_QT Qt5::Widgets)
include_directories(${Qt5Widgets_INCLUDE_DIRS})
add_definitions(${Qt5Widgets_DEFINITIONS})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(include)
aux_source_directory(src ELOG_RUN_GUI_SRC)

qt5_wrap_ui(elogRun_UISrcs ui/elogrunctrl.ui)
qt5_wrap_cpp(elogRun_MOCSrcs include/elogrunctrl.h include/elog.h)
list(APPEND ELOG_RUN_GUI_SRC ${elogRun_UISrcs} ${elogRun_MOCSrcs})
add_library(elogRC SHARED ${ELOG_RUN_GUI_SRC})
target_link_libraries(elogRC ${EUDAQ_CORE_LIBRARY} ${EXTERNAL_LIB_QT})
#list(APPEND INSTALL_TARGETS euElogRun)

install(TARGETS ${INSTALL_TARGETS}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
