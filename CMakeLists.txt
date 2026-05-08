cmake_minimum_required(VERSION 3.27 FATAL_ERROR)
project(richmd VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# wxWidgets configuration
# REQUIRED_VERSION: minimum acceptable system install (matches Ubuntu 24.04 LTS).
# FETCH_VERSION:    version FetchContent pulls when no system install is found.
#                   3.2.8+ is required for libc++ (Windows mingw-clang/CLANG64);
#                   3.2.4's wx/ustring.h hits an undefined std::char_traits<unsigned int>.
set(wxWidgets_REQUIRED_VERSION "3.2.4")
set(wxWidgets_FETCH_VERSION    "3.2.8")
set(wxWidgets_REQUIRED_COMPONENTS core base)

find_package(wxWidgets ${wxWidgets_REQUIRED_VERSION} QUIET COMPONENTS ${wxWidgets_REQUIRED_COMPONENTS})

if(wxWidgets_FOUND)
    message(STATUS "Found pre-installed wxWidgets")
    include(${wxWidgets_USE_FILE})
    message(STATUS "-- libraries: ${wxWidgets_LIBRARIES}")
else()
    message(STATUS "wxWidgets ${wxWidgets_REQUIRED_VERSION}+ not found, fetching v${wxWidgets_FETCH_VERSION} from GitHub...")

    include(FetchContent)
    set(wxBUILD_SHARED OFF)

    # wx 3.2.x's top-level CMakeLists declares cmake_minimum_required < 3.5,
    # which CMake 4+ rejects. This shim re-establishes the old policy floor
    # for the FetchContent sub-build only.
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

    FetchContent_Declare(
        wxWidgets
        GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
        GIT_TAG v${wxWidgets_FETCH_VERSION}
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

    # Custom plist: enables Retina/HiDPI rendering and sets bundle metadata.
    set_target_properties(richmd PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/src/Info.plist)
elseif(WIN32)
    # WIN32 makes the linker look for WinMain instead of main.
    # The .manifest declares Per-Monitor V2 DPI awareness; resources.rc pulls
    # in wx's standard resource block (with wxUSE_DPI_AWARE_MANIFEST=2) and
    # embeds richmd.ico as resource ID 1 (Explorer's exe icon).
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/resources.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/resources.rc
        @ONLY)
    add_executable(richmd WIN32 ${SRCS}
        src/main.exe.manifest
        ${CMAKE_CURRENT_BINARY_DIR}/resources.rc)

    # lld is faster than the default linker; we link wx and the C++ runtime
    # dynamically — runtime DLLs are copied into the install tree below via
    # install(... RUNTIME_DEPENDENCIES).
    target_link_options(richmd PRIVATE -fuse-ld=lld)
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

if(WIN32)
    # Walk the installed .exe's PE imports and copy non-system runtime DLLs
    # (wx, libc++, libunwind, zlib, etc.) into bin/ alongside the executable.
    # PRE excludes Windows API sets, POST drops anything resolved from System32.
    install(CODE "
        file(GET_RUNTIME_DEPENDENCIES
            EXECUTABLES \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/$<TARGET_FILE_NAME:richmd>\"
            RESOLVED_DEPENDENCIES_VAR _resolved
            UNRESOLVED_DEPENDENCIES_VAR _unresolved
            PRE_EXCLUDE_REGEXES \"api-ms-.*\" \"ext-ms-.*\"
            POST_EXCLUDE_REGEXES \".*[Ss]ystem32.*\" \".*WINDOWS.*\"
        )
        foreach(_dep \${_resolved})
            file(INSTALL \"\${_dep}\" DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\")
        endforeach()
        if(_unresolved)
            message(WARNING \"Unresolved runtime deps: \${_unresolved}\")
        endif()
    ")
endif()

if(LINUX OR CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    install(FILES richmd.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES richmd.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
endif()

# ---- CPack ----
set(CPACK_PACKAGE_NAME "richmd")
set(CPACK_PACKAGE_VENDOR "onlyfastcode")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Live-styling markdown editor")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/lszl84/richmd")
set(CPACK_PACKAGE_CONTACT "lukasz.sromek@gmail.com")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "richmd")
set(CPACK_STRIP_FILES ON)

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    # HiDPI: tell the NSIS-generated installer .exe to use Per-Monitor DPI
    # awareness so the wizard isn't bitmap-upscaled (= pixelated) on
    # high-resolution displays. Requires CMake 3.27+.
    set(CPACK_NSIS_MANIFEST_DPI_AWARE TRUE)
    set(CPACK_NSIS_MUI_ICON   "${CMAKE_CURRENT_SOURCE_DIR}/richmd.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/richmd.ico")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\richmd.exe")
    set(CPACK_NSIS_DISPLAY_NAME "richmd")
    set(CPACK_NSIS_PACKAGE_NAME "richmd")
    set(CPACK_NSIS_URL_INFO_ABOUT "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_NSIS_CONTACT "${CPACK_PACKAGE_CONTACT}")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\richmd.lnk' '$INSTDIR\\\\bin\\\\richmd.exe'")
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
        "Delete '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\richmd.lnk'")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "richmd")
else()
    set(CPACK_GENERATOR "DEB;TGZ")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Lukasz Sromek <lukasz.sromek@gmail.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "editors")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
endif()

include(CPack)
