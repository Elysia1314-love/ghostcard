cmake_minimum_required(VERSION 3.28)
project(GhostCard LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 使用 FetchContent 自动获取 SFML 3.0.1
include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.1
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

# 创建游戏库目标
add_library(GameLib 
    src/Card.cpp
    src/Game.cpp
    src/Player.cpp
)

# 设置 GameLib 的包含目录
target_include_directories(GameLib PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 链接 SFML 到 GameLib
target_link_libraries(GameLib
    PUBLIC
        SFML::Graphics
        SFML::Window
        SFML::System
)

# 创建可执行文件
add_executable(${PROJECT_NAME} src/main.cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

# 链接库
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        GameLib
)

# 在 Windows 系统中（使用 MinGW-w64）进行静态链接
if(WIN32 AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
  target_link_libraries(${PROJECT_NAME} PRIVATE
      -static-libgcc
      -static-libstdc++
  )  
endif()

# 复制 assets 目录到输出目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets
    COMMENT "Copying assets directory to output directory"
)

# 测试 & 打包
enable_testing()
add_test(NAME SmokeTest
         COMMAND ${PROJECT_NAME}
         WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION .)
install(DIRECTORY "assets/"
        DESTINATION "assets"
        FILES_MATCHING PATTERN "*"
)

set(CPACK_PACKAGE_NAME "GhostCard")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VENDOR "Developer")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "CMake-SFML-GhostCard")
set(CPACK_PACKAGE_CONTACT "contact@example.com")
include(CPack)