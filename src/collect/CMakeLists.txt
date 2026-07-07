add_library(meimei_collect STATIC
    ModbusMaster.cpp
    CollectThread.cpp
    CsvRecorder.cpp
)
target_include_directories(meimei_collect PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/config
)
target_include_directories(meimei_collect PRIVATE ${LIBMODBUS_INCLUDE_DIRS})
target_link_libraries(meimei_collect PUBLIC meimei_common meimei_logger ${LIBMODBUS_LIBRARIES})