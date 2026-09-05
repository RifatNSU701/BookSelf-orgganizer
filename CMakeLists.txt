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
find_package(SQLite3 REQUIRED)

set(BSO_CORE_SOURCES
    src/bookshelf_core.c
    src/bookshelf.c
    src/input.c
    src/inventory.c
    src/logger.c
    src/process_manager.c
    src/repository.c
    src/search.c
    src/sorting.c
    src/storage.c
    src/storage_sqlite.c
    src/synchronization.c
    src/thread_manager.c
    src/utils.c
    src/worker_pool.c
)

add_library(bookshelf_core STATIC ${BSO_CORE_SOURCES})
set_target_properties(bookshelf_core PROPERTIES OUTPUT_NAME "bookshelf_core")
target_include_directories(bookshelf_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(bookshelf_core PUBLIC Threads::Threads SQLite::SQLite3)
target_compile_options(bookshelf_core PRIVATE $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wshadow;-Wformat=2>)
target_compile_definitions(bookshelf_core PRIVATE $<$<CONFIG:Debug>:BSO_DEBUG_BUILD=1>)

aadd_executable(bookshelf-organizer src/main.c)
target_link_libraries(bookshelf-organizer PRIVATE bookshelf_core)
target_compile_options(bookshelf-organizer PRIVATE $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wshadow;-Wformat=2>)
set_target_properties(bookshelf-organizer PROPERTIES OUTPUT_NAME "BookShelfOrganizer")
target_compile_definitions(bookshelf-organizer PRIVATE $<$<CONFIG:Debug>:BSO_DEBUG_BUILD=1>)

add_executable(bookshelf-migrate src/migrate.c)
target_link_libraries(bookshelf-migrate PRIVATE bookshelf_core)
target_compile_options(bookshelf-migrate PRIVATE $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wshadow;-Wformat=2>)
set_target_properties(bookshelf-migrate PROPERTIES OUTPUT_NAME "BookShelfMigrate")

if(BSO_BUILD_TESTS AND BUILD_TESTING)
    enable_testing()
    add_executable(test_repository tests/test_repository.c)
    target_link_libraries(test_repository PRIVATE bookshelf_core)
    target_compile_options(test_repository PRIVATE $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wshadow;-Wformat=2>)
    add_test(NAME repository_persistence COMMAND test_repository)
    add_executable(test_worker_pool tests/test_worker_pool.c)
    target_link_libraries(test_worker_pool PRIVATE bookshelf_core)
    target_compile_options(test_worker_pool PRIVATE $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wshadow;-Wformat=2>)
    add_test(NAME worker_pool_stress COMMAND test_worker_pool)
endif()

if(BSO_ENABLE_SANITIZERS AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(BSO_SANITIZER_FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_compile_options(bookshelf_core PRIVATE ${BSO_SANITIZER_FLAGS})
    target_link_options(bookshelf_core PRIVATE -fsanitize=address,undefined)
    target_compile_options(bookshelf-organizer PRIVATE ${BSO_SANITIZER_FLAGS})
    target_link_options(bookshelf-organizer PRIVATE -fsanitize=address,undefined)
    target_compile_options(bookshelf-migrate PRIVATE ${BSO_SANITIZER_FLAGS})
    target_link_options(bookshelf-migrate PRIVATE -fsanitize=address,undefined)
    if(BSO_BUILD_TESTS AND BUILD_TESTING)
        target_compile_options(test_repository PRIVATE ${BSO_SANITIZER_FLAGS})
        target_link_options(test_repository PRIVATE -fsanitize=address,undefined)
        target_compile_options(test_worker_pool PRIVATE ${BSO_SANITIZER_FLAGS})
        target_link_options(test_worker_pool PRIVATE -fsanitize=address,undefined)
    endif()
endif()

install(TARGETS bookshelf-organizer bookshelf-migrate RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS bookshelf_core ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES include/bookshelf_core.h include/storage.h include/storage_sqlite.h include/repository.h include/worker_pool.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bookshelf)
