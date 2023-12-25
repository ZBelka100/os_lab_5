cmake_minimum_required(VERSION 3.10)

project(YourProjectName)

# Укажите исходные файлы вашего проекта
file(GLOB SOURCES "src/*.cpp")

# Установка переменной CMAKE_PREFIX_PATH
set(CMAKE_PREFIX_PATH "/usr/local;/usr")

find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)

# Поиск библиотеки ZeroMQ
find_package(ZMQ REQUIRED)

# Укажите, в какие исполняемые файлы должны быть скомпилированы ваши исходные файлы
add_executable(calculation src/calculation_node.cpp)
add_executable(control src/control.cpp)

# Линковка библиотек к исполняемым файлам
target_link_libraries(calculation PRIVATE ${ZMQ_LIBRARIES})
target_link_libraries(control PRIVATE ${ZMQ_LIBRARIES})
