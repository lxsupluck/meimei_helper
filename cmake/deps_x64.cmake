# deps_x64.cmake — 本机 x86_64 依赖

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBMODBUS REQUIRED libmodbus)

find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ALSA REQUIRED)
