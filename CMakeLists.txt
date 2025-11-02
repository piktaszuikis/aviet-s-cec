cmake_minimum_required(VERSION 3.5)

project(aviete-cec LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig)
pkg_check_modules(DBUS dbus-1)
include_directories(${DBUS_INCLUDE_DIRS})
link_directories(${DBUS_LIBRARY_DIRS})

add_executable(aviete-cec main.cpp notify.cpp)
target_link_libraries(aviete-cec cec dl xdo ${DBUS_LIBRARIES})

set(CMAKE_INSTALL_PREFIX "$ENV{HOME}")
install(TARGETS aviete-cec RUNTIME DESTINATION .local/bin)
install(FILES aviete.service DESTINATION .config/systemd/user)
install(FILES notification-daemon.service DESTINATION .config/systemd/user)

install(CODE "execute_process(COMMAND \"/bin/systemctl\" \"--user\" \"enable\" \"--now\" \"notification-daemon.service\")")
install(CODE "execute_process(COMMAND \"/bin/systemctl\" \"--user\" \"enable\" \"--now\" \"aviete.service\")")
