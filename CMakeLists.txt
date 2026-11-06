cmake_minimum_required(VERSION 3.21)

project(libster LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

function(libster_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /wd4121) # wegen gepackten structs auf ungeradem offset (BookRecord)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

# 1) Qt-freie Core-Lib
add_library(libster_core STATIC
    src/core/Version.cpp
    src/core/model/Borrower.cpp
    src/core/model/Medium.cpp
    src/core/model/Book.cpp
    src/core/model/CD.cpp
    src/core/model/LoanTransaction.cpp
    src/core/persistence/BinaryFile.cpp
)
target_include_directories(libster_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
libster_enable_warnings(libster_core)

# 2) Qt-GUI
qt_add_executable(libster
    src/gui/main.cpp
)
target_link_libraries(libster PRIVATE
    libster_core
    Qt6::Widgets
)
libster_enable_warnings(libster)

# 3) Tests
enable_testing()

add_executable(libster_tests
    tests/test_main.cpp
    tests/test_persistence.cpp
    tests/test_borrower.cpp
    tests/test_binaryfile.cpp
    tests/test_models.cpp
    tests/test_loan_transaction.cpp
)
target_link_libraries(libster_tests PRIVATE libster_core)
libster_enable_warnings(libster_tests)

add_test(NAME libster_tests COMMAND libster_tests)
