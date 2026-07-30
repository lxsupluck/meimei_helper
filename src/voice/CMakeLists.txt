add_library(meimei_voice STATIC
    VoiceRecorder.cpp
    SpeechToText.cpp
    VoiceThread.cpp
)

target_include_directories(meimei_voice PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/config
)

target_include_directories(meimei_voice PUBLIC
    ${CURL_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${ALSA_INCLUDE_DIRS}
)

target_link_libraries(meimei_voice PUBLIC
    meimei_common
    ${CURL_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${ALSA_LIBRARIES}
)
