cmake_minimum_required(VERSION 3.16)

project(Ongaku VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For VSCode C++ configuration

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia Sql)

# Find TagLib for audio metadata
find_package(PkgConfig REQUIRED)
pkg_check_modules(TAGLIB REQUIRED taglib)

# Enable automatic MOC and RCC (no UIC needed)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/databasemanager.cpp
    src/musicscanner.cpp
    src/musiclibrarymodel.cpp
    src/musiclibraryflat.cpp
    src/musicplayer.cpp
)

# Header files
set(HEADERS
    src/mainwindow.h
    src/databasemanager.h
    src/musicscanner.h
    src/musiclibrarymodel.h
    src/musiclibraryflat.h
    src/musicplayer.h
)

# Create executable
add_executable(Ongaku ${SOURCES} ${HEADERS})

# Link Qt libraries
target_link_libraries(Ongaku Qt6::Core Qt6::Widgets Qt6::Multimedia Qt6::Sql ${TAGLIB_LIBRARIES})

# Include directories
target_include_directories(Ongaku PRIVATE src ${TAGLIB_INCLUDE_DIRS})
