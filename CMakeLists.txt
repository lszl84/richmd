cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

project(richmd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- wxWidgets: try system first, fall back to FetchContent ----
find_package(wxWidgets QUIET COMPONENTS core base)

if(NOT wxWidgets_FOUND)
    message(STATUS "wxWidgets not found in system — building from source (FetchContent)")
    include(FetchContent)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
    set(wxBUILD_SHARED OFF)
    FetchContent_Declare(
        wxWidgets
        GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
        GIT_TAG v3.2.6
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(wxWidgets)
    set(WX_LIBS wxcore)
else()
    message(STATUS "Using system wxWidgets ${wxWidgets_VERSION_STRING}")
    include(${wxWidgets_USE_FILE})
    set(WX_LIBS ${wxWidgets_LIBRARIES})
endif()

# ---- Sources ----
set(SRCS
    src/main.cpp
    src/md_frame.cpp
)

if(APPLE)
    add_executable(richmd MACOSX_BUNDLE ${SRCS})
elseif(WIN32)
    add_executable(richmd WIN32 ${SRCS})
else()
    add_executable(richmd ${SRCS})
endif()

target_link_libraries(richmd PRIVATE ${WX_LIBS})
target_compile_options(richmd PRIVATE -Wall -Wextra -Wno-unused-parameter)

include(GNUInstallDirs)
install(TARGETS richmd
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .
)
