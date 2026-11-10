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
    src/core/TimeUtil.cpp
    src/core/Validation.cpp
    src/core/model/Borrower.cpp
    src/core/model/Medium.cpp
    src/core/model/Book.cpp
    src/core/model/CD.cpp
    src/core/model/LoanTransaction.cpp
    src/core/services/LibraryService.cpp
    src/core/persistence/BinaryFile.cpp
)
target_include_directories(libster_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
libster_enable_warnings(libster_core)

# 2) Qt-GUI
qt_add_executable(libster
    src/gui/main.cpp
    src/gui/GuiUtil.cpp
    src/gui/GuiUtil.h
    src/gui/MainWindow.cpp
    src/gui/MainWindow.h
    src/gui/StartupDialog.cpp
    src/gui/StartupDialog.h
    src/gui/BookTableModel.cpp
    src/gui/BookTableModel.h
    src/gui/CDTableModel.cpp
    src/gui/CDTableModel.h
    src/gui/BookTab.cpp
    src/gui/BookTab.h
    src/gui/CDTab.cpp
    src/gui/CDTab.h
    src/gui/BookDialog.cpp
    src/gui/BookDialog.h
    src/gui/CDDialog.cpp
    src/gui/CDDialog.h
    src/gui/BorrowerTableModel.cpp
    src/gui/BorrowerTableModel.h
    src/gui/BorrowerTab.cpp
    src/gui/BorrowerTab.h
    src/gui/BorrowerDialog.cpp
    src/gui/BorrowerDialog.h
    src/gui/LoanTableModel.cpp
    src/gui/LoanTableModel.h
    src/gui/LoanSidebar.cpp
    src/gui/LoanSidebar.h
    src/gui/ReportTab.cpp
    src/gui/ReportTab.h
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
    tests/test_timeutil.cpp
    tests/test_persistence.cpp
    tests/test_borrower.cpp
    tests/test_binaryfile.cpp
    tests/test_models.cpp
    tests/test_loan_transaction.cpp
    tests/test_library_service.cpp
    tests/test_validation.cpp
)
target_link_libraries(libster_tests PRIVATE libster_core)
libster_enable_warnings(libster_tests)

add_test(NAME libster_tests COMMAND libster_tests)

# 4) Testdaten-Generator
add_executable(libster_generate
    tools/generate_test_data.cpp
)
target_link_libraries(libster_generate PRIVATE libster_core)
libster_enable_warnings(libster_generate)

# 5) Performance-Benchmark
add_executable(libster_benchmark
    tools/benchmark_search.cpp
)
target_link_libraries(libster_benchmark PRIVATE libster_core)
libster_enable_warnings(libster_benchmark)
