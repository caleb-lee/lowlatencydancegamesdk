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

# Make headers available to users of the library
target_include_directories(lowlatencydancegamesdk PUBLIC include)

if(WIN32)
  target_compile_definitions(lowlatencydancegamesdk PRIVATE 
    $<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS=1>)
endif()

# Install commands
install(TARGETS lowlatencydancegamesdk
       ARCHIVE DESTINATION lib
       LIBRARY DESTINATION lib
       RUNTIME DESTINATION bin)

install(DIRECTORY include/ 
       DESTINATION include
       FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")