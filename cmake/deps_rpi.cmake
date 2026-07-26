# deps_rpi.cmake — ARM64 交叉编译依赖
# 交叉编译器自动在 /usr/aarch64-linux-gnu/ 下找库

set(CURL_INCLUDE_DIRS    /usr/aarch64-linux-gnu/include)
set(CURL_LIBRARIES       curl)

set(OPENSSL_INCLUDE_DIR  /usr/aarch64-linux-gnu/include)
set(OPENSSL_LIBRARIES    ssl crypto)

set(ALSA_INCLUDE_DIRS    /usr/aarch64-linux-gnu/include)
set(ALSA_LIBRARIES       asound)
