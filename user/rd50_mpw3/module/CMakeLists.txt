aux_source_directory(src MODULE_SRC)
INCLUDE_DIRECTORIES(include)
aux_source_directory(tools TOOLS_SRC)
#INCLUDE_DIRECTORIES(tools/include)

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC} ${TOOLS_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY})

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
