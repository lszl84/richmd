cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(richmd LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# wxWidgets configuration
set(wxWidgets_REQUIRED_VERSION "3.2.8")
set(wxWidgets_REQUIRED_COMPONENTS core base)

find_package(wxWidgets ${wxWidgets_REQUIRED_VERSION} QUIET COMPONENTS ${wxWidgets_REQUIRED_COMPONENTS})

if(wxWidgets_FOUND)
    message(STATUS "Found pre-installed wxWidgets")
    include(${wxWidgets_USE_FILE})
    message(STATUS "-- libraries: ${wxWidgets_LIBRARIES}")
else()
    message(STATUS "wxWidgets ${wxWidgets_REQUIRED_VERSION}+ not found, fetching from GitHub...")

    include(FetchContent)
    set(wxBUILD_SHARED OFF)

    FetchContent_Declare(
        wxWidgets
        GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
        GIT_TAG v${wxWidgets_REQUIRED_VERSION}
    )
    FetchContent_MakeAvailable(wxWidgets)

    foreach(component ${wxWidgets_REQUIRED_COMPONENTS})
        list(APPEND wxWidgets_LIBRARIES wx${component})
    endforeach()
endif()

set(SRCS
    src/main.cpp
    src/md_frame.cpp
)

if(APPLE)
    add_executable(richmd MACOSX_BUNDLE ${SRCS})
elseif(WIN32)
    # WIN32 makes the linker look for WinMain instead of main.
    add_executable(richmd WIN32 ${SRCS})

    # static linking for a self-contained Windows binary (no DLL dependencies)
    target_link_options(richmd PRIVATE -static -fuse-ld=lld)
    target_link_libraries(richmd PRIVATE c++ c++abi unwind)
else()
    add_executable(richmd ${SRCS})
endif()

target_link_libraries(richmd PRIVATE ${wxWidgets_LIBRARIES})
target_compile_options(richmd PRIVATE -Wall -Wextra -Wno-unused-parameter)

include(GNUInstallDirs)
install(TARGETS richmd
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .
)
if(LINUX OR CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    install(FILES richmd.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES richmd.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
endif()
