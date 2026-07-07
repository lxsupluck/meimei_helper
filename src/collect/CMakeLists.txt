add_library(meimei_collect STATIC
    ModbusMaster.cpp
    CollectThread.cpp
)
target_include_directories(meimei_collect PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(meimei_collect PRIVATE ${LIBMODBUS_INCLUDE_DIRS})
target_link_libraries(meimei_collect PUBLIC meimei_common ${LIBMODBUS_LIBRARIES})