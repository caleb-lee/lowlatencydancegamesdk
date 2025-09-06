cmake_minimum_required(VERSION 3.16)
project(lowlatencydancegamesdk)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Check if external libusb variables are defined
if(DEFINED LIBUSB_INCLUDE_DIR AND DEFINED LIBUSB_LIBRARY)
    # Use external libusb
    message(STATUS "Using external libusb: ${LIBUSB_LIBRARY}")
    
    # Create library
    add_library(lowlatencydancegamesdk STATIC 
        src/lowlatencydancegamesdk.cpp
        src/adapters/AdapterBase.c
        src/adapters/SMXStage/SMXStageAdapter.c
        src/adapters/FoamPad/FoamPadAdapter.c)
    
    # Include external libusb headers
    target_include_directories(lowlatencydancegamesdk PRIVATE ${LIBUSB_INCLUDE_DIR})
else()
    # Build our own libusb
    message(STATUS "Building own libusb")
    
    # Add libusb from extern directory
    add_subdirectory(extern/libusb)
    
    # Create library
    add_library(lowlatencydancegamesdk STATIC 
        src/lowlatencydancegamesdk.cpp
        src/adapters/AdapterBase.c
        src/adapters/SMXStage/SMXStageAdapter.c
        src/adapters/FoamPad/FoamPadAdapter.c)
    
    # Link libusb
    target_link_libraries(lowlatencydancegamesdk PRIVATE usb-1.0)
    
    # Include libusb headers
    target_include_directories(lowlatencydancegamesdk PRIVATE extern/libusb/libusb/libusb)
endif()

# Make headers available to users of the library
target_include_directories(lowlatencydancegamesdk PUBLIC include)

if(WIN32)
  target_compile_definitions(lowlatencydancegamesdk PRIVATE 
    $<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS=1>)
  
  # SMX.dll wrapper target (Windows only)
  # This creates a DLL with the stepmaniax-sdk interface that uses lldgsdk as backend
  if(DEFINED LIBUSB_INCLUDE_DIR AND DEFINED LIBUSB_LIBRARY)
      # Use external libusb
      add_library(SMX SHARED
          src/smx-dll-wrapper/SMX.cpp
          src/lowlatencydancegamesdk.cpp
          src/adapters/AdapterBase.c
          src/adapters/SMXStage/SMXStageAdapter.c
          src/adapters/FoamPad/FoamPadAdapter.c)
      
      target_include_directories(SMX PRIVATE ${LIBUSB_INCLUDE_DIR})
      target_link_libraries(SMX PRIVATE ${LIBUSB_LIBRARY})
  else()
      # Use built-in libusb
      add_library(SMX SHARED
          src/smx-dll-wrapper/SMX.cpp
          src/lowlatencydancegamesdk.cpp
          src/adapters/AdapterBase.c
          src/adapters/SMXStage/SMXStageAdapter.c
          src/adapters/FoamPad/FoamPadAdapter.c)
      
      target_include_directories(SMX PRIVATE extern/libusb/libusb/libusb)
      target_link_libraries(SMX PRIVATE usb-1.0)
  endif()
  
  # SMX.dll specific settings
  target_include_directories(SMX PUBLIC src/smx-dll-wrapper include)
  target_compile_definitions(SMX PRIVATE SMX_EXPORTS
                                 PRIVATE $<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS=1>)
  
  # Install commands (Windows only)
  install(TARGETS lowlatencydancegamesdk SMX
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib
         RUNTIME DESTINATION bin)

  install(DIRECTORY include/ 
         DESTINATION include
         FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
         
  install(FILES src/smx-dll-wrapper/SMX.h
         DESTINATION include)
endif()