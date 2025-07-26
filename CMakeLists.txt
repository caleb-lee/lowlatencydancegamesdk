cmake_minimum_required(VERSION 3.16)
project(lowlatencydancegamesdk)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build libusb as an object library (not a target that gets installed/linked)
add_subdirectory(extern/libusb)
get_target_property(LIBUSB_SOURCES usb-1.0 SOURCES)
get_target_property(LIBUSB_INCLUDE_DIRS usb-1.0 INTERFACE_INCLUDE_DIRECTORIES)

# Create your library and embed libusb directly into it
add_library(lowlatencydancegamesdk STATIC 
    src/lowlatencydancegamesdk.cpp
    $<TARGET_OBJECTS:usb-1.0>)  # Embed libusb object files

# Include libusb headers privately
target_include_directories(lowlatencydancegamesdk PRIVATE extern/libusb/libusb/libusb)

# Platform-specific settings (all PRIVATE)
if(WIN32)
    target_link_libraries(lowlatencydancegamesdk PRIVATE setupapi)
elseif(APPLE)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(IOKIT_LIBRARY IOKit)
    target_link_libraries(lowlatencydancegamesdk PRIVATE ${COREFOUNDATION_LIBRARY} ${IOKIT_LIBRARY})
elseif(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(UDEV REQUIRED libudev)
    target_link_libraries(lowlatencydancegamesdk PRIVATE ${UDEV_LIBRARIES})
    target_include_directories(lowlatencydancegamesdk PRIVATE ${UDEV_INCLUDE_DIRS})
endif()

# Only expose your own headers
target_include_directories(lowlatencydancegamesdk PUBLIC include)