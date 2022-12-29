cmake_minimum_required(VERSION 3.1.0)
#include(CMakeDependentOption)
find_package(Qt5Widgets REQUIRED)
message(STATUS "Qt5Widgets: ${Qt5Widgets_DIR}")
set(EXTERNAL_LIB_QT Qt5::Widgets)
include_directories(${Qt5Widgets_INCLUDE_DIRS})
add_definitions(${Qt5Widgets_DEFINITIONS})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(include)
aux_source_directory(src MODULE_SRC)

qt5_wrap_ui(elogRun_UISrcs ui/elogrunctrl.ui)
qt5_wrap_cpp(elogRun_MOCSrcs include/elogrunctrl.h include/elog.h)
list(APPEND ELOG_RUN_GUI_SRC ${elogRun_UISrcs} ${elogRun_MOCSrcs})
add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC} ${ELOG_RUN_GUI_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY} ${EXTERNAL_LIB_QT})

install(TARGETS ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
