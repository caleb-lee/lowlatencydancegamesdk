cmake_minimum_required(VERSION 3.16)
project(lowlatencydancegamesdk)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(ExternalProject)

# Build libusb as a completely separate project
ExternalProject_Add(
    libusb_external
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/libusb
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/libusb_install
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
    BUILD_BYPRODUCTS 
        ${CMAKE_CURRENT_BINARY_DIR}/libusb_install/lib/libusb-1.0.a
        ${CMAKE_CURRENT_BINARY_DIR}/libusb_install/lib/usb-1.0.lib
)

# Create your library
add_library(lowlatencydancegamesdk STATIC src/lowlatencydancegamesdk.cpp)

# Link to the externally built libusb
add_dependencies(lowlatencydancegamesdk libusb_external)

if(WIN32)
    target_link_libraries(lowlatencydancegamesdk PRIVATE 
        ${CMAKE_CURRENT_BINARY_DIR}/libusb_install/lib/usb-1.0.lib
        setupapi)
else()
    target_link_libraries(lowlatencydancegamesdk PRIVATE 
        ${CMAKE_CURRENT_BINARY_DIR}/libusb_install/lib/libusb-1.0.a)
endif()

target_include_directories(lowlatencydancegamesdk PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/libusb_install/include/libusb-1.0)

# Platform-specific settings
if(APPLE)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(IOKIT_LIBRARY IOKit)
    target_link_libraries(lowlatencydancegamesdk PRIVATE ${COREFOUNDATION_LIBRARY} ${IOKIT_LIBRARY})
elseif(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(UDEV REQUIRED libudev)
    target_link_libraries(lowlatencydancegamesdk PRIVATE ${UDEV_LIBRARIES})
    target_include_directories(lowlatencydancegamesdk PRIVATE ${UDEV_INCLUDE_DIRS})
endif()

# Only expose your headers
target_include_directories(lowlatencydancegamesdk PUBLIC include)