cmake_minimum_required(VERSION 3.20)

project(BookShelfOrganizer VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(BSO_BUILD_TESTS "Build test targets" ON)
option(BSO_ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)

include(GNUInstallDirs)
include(CTest)

find_package(Threads REQUIRED)

set(BSO_CORE_SOURCES
    src/bookshelf.c
    src/input.c
    src/inventory.c
    src/logger.c
    src/process_manager.c
    src/search.c
    src/sorting.c
    src/synchronization.c
    src/thread_manager.c
    src/utils.c
)

add_library(bookshelf_core STATIC ${BSO_CORE_SOURCES})

target_include_directories(bookshelf_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(bookshelf_core PUBLIC Threads::Threads)

target_compile_options(bookshelf_core PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wshadow;-Wformat=2>
)

if(BSO_ENABLE_SANITIZERS AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(bookshelf_core PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(bookshelf_core PRIVATE -fsanitize=address,undefined)
endif()

add_executable(bookshelf-organizer src/main.c)
target_link_libraries(bookshelf-organizer PRIVATE bookshelf_core Threads::Threads)

target_compile_options(bookshelf-organizer PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wshadow;-Wformat=2>
)

if(BSO_ENABLE_SANITIZERS AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(bookshelf-organizer PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(bookshelf-organizer PRIVATE -fsanitize=address,undefined)
endif()

set_target_properties(bookshelf-organizer PROPERTIES OUTPUT_NAME "BookShelfOrganizer")

if(BSO_BUILD_TESTS AND BUILD_TESTING)
    enable_testing()
endif()

install(TARGETS bookshelf-organizer
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
