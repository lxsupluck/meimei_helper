add_library(meimei_logger STATIC
    Logger.cpp
)
target_include_directories(meimei_logger PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(meimei_logger PUBLIC meimei_common)
