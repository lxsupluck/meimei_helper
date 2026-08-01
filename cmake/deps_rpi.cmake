# deps_rpi.cmake — ARM64 交叉编译依赖（Ubuntu 22.04 多架构）

set(LIBMODBUS_INCLUDE_DIRS /usr/include/modbus)
set(LIBMODBUS_LIBRARIES    /usr/lib/aarch64-linux-gnu/libmodbus.so)

set(CURL_INCLUDE_DIRS      /usr/include/aarch64-linux-gnu)
set(CURL_LIBRARIES         /usr/lib/aarch64-linux-gnu/libcurl.so)

set(OPENSSL_INCLUDE_DIR    /usr/include)
set(OPENSSL_LIBRARIES      /usr/lib/aarch64-linux-gnu/libssl.so
                           /usr/lib/aarch64-linux-gnu/libcrypto.so)

set(ALSA_INCLUDE_DIRS      /usr/include)
set(ALSA_LIBRARIES         /usr/lib/aarch64-linux-gnu/libasound.so)
