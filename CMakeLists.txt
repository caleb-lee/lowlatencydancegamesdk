cmake_minimum_required(VERSION 3.16)
project(lowlatencydancegamesdk)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add libusb from extern directory
add_subdirectory(extern/libusb)

# Create library
add_library(lowlatencydancegamesdk STATIC src/lowlatencydancegamesdk.cpp)

# Link libusb
target_link_libraries(lowlatencydancegamesdk usb-1.0)

# Include libusb headers
target_include_directories(lowlatencydancegamesdk PRIVATE extern/libusb/libusb/libusb)

# Platform-specific settings
if(WIN32)
    # Windows specific libraries for libusb
    target_link_libraries(lowlatencydancegamesdk setupapi)
elseif(APPLE)
    # macOS requires CoreFoundation and IOKit for libusb
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(IOKIT_LIBRARY IOKit)
    target_link_libraries(lowlatencydancegamesdk ${COREFOUNDATION_LIBRARY} ${IOKIT_LIBRARY})
elseif(UNIX)
    # Linux specific settings
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(UDEV REQUIRED libudev)
    target_link_libraries(lowlatencydancegamesdk ${UDEV_LIBRARIES})
    target_include_directories(lowlatencydancegamesdk PRIVATE ${UDEV_INCLUDE_DIRS})
endif()

# Make headers available to users of the library
target_include_directories(lowlatencydancegamesdk PUBLIC include)