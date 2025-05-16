cmake_minimum_required(VERSION 3.10)
project(Insomnia)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
set(SOURCE_FILES
    main.cpp
    resource.rc
)

# Create executable
add_executable(Insomnia WIN32 ${SOURCE_FILES})

# Link required libraries
target_link_libraries(Insomnia PRIVATE
    comctl32
    shell32
)

# Set Windows subsystem
set_target_properties(Insomnia PROPERTIES
    WIN32_EXECUTABLE TRUE
)

