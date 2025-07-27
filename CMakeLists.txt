cmake_minimum_required(VERSION 3.16)
project(lowlatencydancegamesdk)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")
endif()

# Add libusb from extern directory
add_subdirectory(extern/libusb)

# Create library
add_library(lowlatencydancegamesdk STATIC src/lowlatencydancegamesdk.cpp)

# Link libusb
target_link_libraries(lowlatencydancegamesdk usb-1.0)

# Include libusb headers
target_include_directories(lowlatencydancegamesdk PRIVATE extern/libusb/libusb/libusb)

# Make headers available to users of the library
target_include_directories(lowlatencydancegamesdk PUBLIC include)