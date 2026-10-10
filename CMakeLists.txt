cmake_minimum_required(VERSION 3.14...3.25)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

file(READ "src/okjversion.h" vercontents)
string(REGEX MATCH "OKJ_VERSION_MAJOR ([0-9]*)" _ ${vercontents})
set(ver_major ${CMAKE_MATCH_1})
string(REGEX MATCH "OKJ_VERSION_MINOR ([0-9]*)" _ ${vercontents})
set(ver_minor ${CMAKE_MATCH_1})
string(REGEX MATCH "OKJ_VERSION_BUILD ([0-9]*)" _ ${vercontents})
set(ver_build ${CMAKE_MATCH_1})
set(sem_ver "${ver_major}.${ver_minor}.${ver_build}")

message(STATUS "Building OpenKJ ${sem_ver}")

project(openkj
        VERSION ${sem_ver}
        LANGUAGES CXX C
        )

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6 Configuration
find_package(Qt6 COMPONENTS Core Gui Sql Network NetworkAuth Widgets Concurrent Svg PrintSupport Multimedia WebSockets REQUIRED)

include_directories(
        src/
)

# CPM (CMake Package Manager) Setup
include(cmake/CPM.cmake)

# CPM Packages
CPMAddPackage("gh:gabime/spdlog@1.8.2")

# libzip static build via CPM
CPMAddPackage(
    NAME libzip
    GITHUB_REPOSITORY nih-at/libzip
    VERSION 1.10.1
    OPTIONS
      "ENABLE_COMMONCRYPTO OFF"
      "ENABLE_GNUTLS OFF"
      "ENABLE_MBEDTLS OFF"
      "ENABLE_OPENSSL OFF"
      "ENABLE_BZIP2 OFF"
      "ENABLE_LZMA OFF"
      "ENABLE_ZSTD OFF"
      "BUILD_TOOLS OFF"
      "BUILD_REGRESS OFF"
      "BUILD_EXAMPLES OFF"
      "BUILD_DOC OFF"
      "BUILD_SHARED_LIBS OFF"
)

# taglib static build via CPM
CPMAddPackage(
    NAME taglib
    GITHUB_REPOSITORY taglib/taglib
    VERSION 1.13.1
    OPTIONS
      "BUILD_SHARED_LIBS OFF"
      "WITH_ASF ON"
      "WITH_MP4 ON"
)
if(taglib_ADDED)
    target_include_directories(tag INTERFACE
        ${taglib_SOURCE_DIR}
        ${taglib_SOURCE_DIR}/taglib
        ${taglib_SOURCE_DIR}/taglib/toolkit
        ${taglib_BINARY_DIR}
    )
endif()



# GStreamer Configuration
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-video-1.0 gstreamer-audio-1.0 gstreamer-app-1.0)
include_directories(${GSTREAMER_INCLUDE_DIRS})
link_directories(${GSTREAMER_LIBRARY_DIRS})
set(GSTREAMER_LIBRARIES ${GSTREAMER_LIBRARIES})

set(SOURCE_FILES
        src/main.cpp
        src/dlgaddsong.cpp
        src/mediabackend.cpp
        src/mzarchive.cpp
        src/okjutil.h
        src/okjtypes.cpp
        src/dlgvideopreview.cpp
        src/mainwindow.cpp
        src/dbupdater.cpp
        src/directorymonitor.cpp
        src/dlgkeychange.cpp
        src/dlgdatabase.cpp
        src/dlgrequests.cpp
        src/dlgregularexport.cpp
        src/dlgregularimport.cpp
        src/dlgregularsingers.cpp
        src/dlgsettings.cpp
        src/dlgcdg.cpp
        src/models/tablemodelbreaksongs.cpp
        src/models/tablemodelcustomnamingpatterns.cpp
        src/models/tablemodelhistorysingers.cpp
        src/models/tablemodelhistorysongs.cpp
        src/models/tablemodelkaraokesongs.cpp
        src/models/tablemodelkaraokesourcedirs.cpp
        src/models/tablemodelplaylistsongs.cpp
        src/models/tablemodelqueuesongs.cpp
        src/models/tablemodelrequests.cpp
        src/models/tablemodelrotation.cpp
        src/models/tablemodelsongshopsongs.cpp
        src/tagreader.cpp
        src/bmdbupdatethread.cpp
        src/settings.cpp
        src/bmdbdialog.cpp
        src/dlgcustompatterns.cpp
        src/audiorecorder.cpp
        src/okjsongbookapi.cpp
        src/dlgdbupdate.cpp
        src/dlgbookcreator.cpp
        src/dlgeq.cpp
        src/audiofader.cpp
        src/customlineedit.cpp
        src/tickernew.cpp
        src/updatechecker.cpp
        src/videodisplay.cpp
        src/xbrz.cpp
        src/xbrz.h
        src/volslider.cpp
        src/dlgaddsinger.cpp
        src/songshop.cpp
        src/dlgsongshop.cpp
        src/simplecrypt.cpp
        src/dlgsongshoppurchase.cpp
        src/dlgsetpassword.cpp
        src/dlgpassword.cpp
        src/dlgpurchaseprogress.cpp
        src/karaokefileinfo.cpp
        src/karaokefilepatternresolver.cpp
        src/custompattern.cpp
        src/dlgeditsong.cpp
        src/soundfxbutton.cpp
        src/runguard/runguard.cpp
        src/durationlazyupdater.cpp
        src/idledetect.cpp
        src/databaseworker.cpp
        src/mainwindow.h
        src/dlgaddsong.h
        src/dlgvideopreview.h
        src/mediabackend.h
        src/mzarchive.h
        src/okjutil.h
        src/okjtypes.h
        src/dbupdater.h
        src/directorymonitor.h
        src/dlgkeychange.h
        src/dlgdatabase.h
        src/dlgrequests.h
        src/dlgregularexport.h
        src/dlgregularimport.h
        src/dlgregularsingers.h
        src/dlgsettings.h
        src/dlgcdg.h
        src/models/tablemodelbreaksongs.h
        src/models/tablemodelcustomnamingpatterns.h
        src/models/tablemodelhistorysingers.h
        src/models/tablemodelhistorysongs.h
        src/models/tablemodelkaraokesongs.h
        src/models/tablemodelkaraokesourcedirs.h
        src/models/tablemodelplaylistsongs.h
        src/models/tablemodelqueuesongs.h
        src/models/tablemodelrequests.h
        src/models/tablemodelrotation.h
        src/models/tablemodelsongshopsongs.h
        src/tagreader.h
        src/bmdbupdatethread.h
        src/settings.h
        src/bmdbdialog.h
        src/dlgcustompatterns.h
        src/audiorecorder.h
        src/okjsongbookapi.h
        src/dlgdbupdate.h
        src/dlgbookcreator.h
        src/dlgeq.h
        src/audiofader.h
        src/customlineedit.h
        src/tickernew.h
        src/updatechecker.h
        src/videodisplay.h
        src/volslider.h
        src/okjversion.h
        src/dlgaddsinger.h
        src/songshop.h
        src/dlgsongshop.h
        src/simplecrypt.h
        src/dlgsongshoppurchase.h
        src/dlgsetpassword.h
        src/dlgpassword.h
        src/dlgpurchaseprogress.h
        src/karaokefileinfo.h
        src/karaokefilepatternresolver.h
        src/custompattern.h
        src/dlgeditsong.h
        src/soundfxbutton.h
        src/runguard/runguard.h
        src/models/tableviewtooltipfilter.h
        src/durationlazyupdater.h
        src/idledetect.h
        src/databaseworker.h
        src/mainwindow.ui
        src/dlgaddsong.ui
        src/dlgkeychange.ui
        src/dlgdatabase.ui
        src/dlgrequests.ui
        src/dlgregularexport.ui
        src/dlgregularimport.ui
        src/dlgregularsingers.ui
        src/dlgsettings.ui
        src/dlgcdg.ui
        src/bmdbdialog.ui
        src/dlgcustompatterns.ui
        src/dlgdbupdate.ui
        src/dlgbookcreator.ui
        src/dlgeq.ui
        src/dlgaddsinger.ui
        src/dlgsongshop.ui
        src/dlgsongshoppurchase.ui
        src/dlgsetpassword.ui
        src/dlgpassword.ui
        src/dlgpurchaseprogress.ui
        src/dlgeditsong.ui
        src/dlgvideopreview.ui
        src/resources.qrc
)

set(LIBRARIES
        spdlog::spdlog
        zip
        tag
        ${GSTREAMER_LIBRARIES}
        Qt6::Widgets
        Qt6::Core
        Qt6::Gui
        Qt6::Sql
        Qt6::Network
        Qt6::NetworkAuth
        Qt6::Svg
        Qt6::PrintSupport
        Qt6::Concurrent
        Qt6::Multimedia
        Qt6::WebSockets
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    add_executable(openkj
            ${SOURCE_FILES}
            )
    target_link_libraries(openkj PRIVATE ${LIBRARIES})

    install(
            TARGETS openkj
            DESTINATION bin
    )
    install(
            FILES src/Icons/org.openkj.OpenKJ.svg
            DESTINATION share/icons/hicolor/scalable/apps
    )
    install(
            FILES src/org.openkj.OpenKJ.desktop
            DESTINATION share/applications
    )
    install(
            FILES src/org.openkj.OpenKJ.metainfo.xml
            DESTINATION share/metainfo
    )

    # ---------------------------------------------------------------------------
    # Optional AppImage — produces a single self-contained executable.
    # Enable with:  cmake -DBUILD_APPIMAGE=ON ...
    # Requires linuxdeploy and linuxdeploy-plugin-qt on PATH or in the build dir.
    # ---------------------------------------------------------------------------
    option(BUILD_APPIMAGE "Build a self-contained AppImage (Linux only)" OFF)
    if (BUILD_APPIMAGE)
        find_program(LINUXDEPLOY linuxdeploy linuxdeploy-x86_64.AppImage
            HINTS ${CMAKE_BINARY_DIR} ENV PATH)
        find_program(LINUXDEPLOY_PLUGIN_QT linuxdeploy-plugin-qt linuxdeploy-plugin-qt-x86_64.AppImage
            HINTS ${CMAKE_BINARY_DIR} ENV PATH)

        if (LINUXDEPLOY AND LINUXDEPLOY_PLUGIN_QT)
            # Build the AppDir from the cmake install tree, then package it.
            add_custom_command(TARGET openkj POST_BUILD
                COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR}
                        --prefix ${CMAKE_BINARY_DIR}/AppDir/usr
                COMMAND ${CMAKE_COMMAND} -E env
                        QMAKE=${Qt6_DIR}/../../../bin/qmake
                        OUTPUT=${CMAKE_BINARY_DIR}/OpenKJ-${sem_ver}-x86_64.AppImage
                    "${LINUXDEPLOY}"
                        --appdir ${CMAKE_BINARY_DIR}/AppDir
                        --plugin qt
                        --output appimage
                COMMENT "Building AppImage..."
            )
            message(STATUS "AppImage build enabled: linuxdeploy found at ${LINUXDEPLOY}")
        else()
            message(WARNING "BUILD_APPIMAGE=ON but linuxdeploy or linuxdeploy-plugin-qt not found.\n"
                "Download from https://github.com/linuxdeploy/linuxdeploy/releases and place in the build directory.")
        endif()
    endif()
endif ()

option(OPENKJ_MACOS_DEPLOY "Bundle dependencies into the macOS application" ON)
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(openkj_ICON ${CMAKE_CURRENT_SOURCE_DIR}/src/Icons/OpenKJ.icns)

    set_source_files_properties(${openkj_ICON} PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
            )
    qt_add_executable(openkj
            MACOSX_BUNDLE
            ${openkj_ICON}
            ${SOURCE_FILES}
            )
    set_target_properties(openkj PROPERTIES
            BUNDLE True
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/src/Info.plist.in
            MACOSX_BUNDLE_GUI_IDENTIFIER org.openkj.openkj
            MACOSX_BUNDLE_BUNDLE_NAME OpenKJ
            MACOSX_BUNDLE_BUNDLE_VERSION ${sem_ver}
            MACOSX_BUNDLE_ICON_FILE OpenKJ.icns
            # Ensure rpath inside the bundle is set so the linker embeds it
            MACOSX_RPATH TRUE
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "@executable_path/../Frameworks"
            )
    target_link_libraries(openkj PRIVATE ${LIBRARIES})
    target_include_directories(openkj PRIVATE
            src
            ${taglib_SOURCE_DIR}/taglib/toolkit
            ${taglib_SOURCE_DIR}/taglib
    )

    qt_import_plugins(openkj
        INCLUDE_BY_TYPE permissions
    )

    # -------------------------------------------------------------------------
    # Step 1: Run macdeployqt to bundle Qt frameworks into the .app.
    # This always runs (no BUILDONLY guard) so every build is self-contained.
    # -------------------------------------------------------------------------
    get_target_property(_qt6_qmake_location Qt6::qmake IMPORTED_LOCATION)
    execute_process(
            COMMAND "${_qt6_qmake_location}" -query QT_INSTALL_PREFIX
            RESULT_VARIABLE _qt6_qmake_rc
            OUTPUT_VARIABLE qt6_install_prefix
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (qt6_install_prefix)
        find_program(MACDEPLOYQT macdeployqt
            HINTS "${qt6_install_prefix}"
            PATH_SUFFIXES bin)
        if (MACDEPLOYQT AND OPENKJ_MACOS_DEPLOY)
            add_custom_command(TARGET openkj POST_BUILD
                COMMAND "${MACDEPLOYQT}"
                        "$<TARGET_BUNDLE_DIR:openkj>"
                        -verbose=1
                COMMENT "Running macdeployqt to bundle Qt frameworks..."
            )
        else ()
            message(WARNING "macdeployqt not found — Qt frameworks will NOT be bundled.")
        endif ()
    else ()
        message(WARNING "QT_INSTALL_PREFIX not found — Qt frameworks will NOT be bundled.")
    endif ()

endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(WINDOWS_ICON_RC "src/appicon.rc")
    add_executable(openkj
            ${SOURCE_FILES}
            ${WINDOWS_ICON_RC}
            )
    target_link_libraries(openkj PRIVATE ${LIBRARIES})
    target_include_directories(openkj PRIVATE
            src
            ${taglib_SOURCE_DIR}/taglib/toolkit
            ${taglib_SOURCE_DIR}/taglib
    )
    set_property(
            TARGET openkj
            PROPERTY WIN32_EXECUTABLE true
    )

    # -------------------------------------------------------------------------
    # Step 1: Run windeployqt to copy Qt DLLs and plugins next to the .exe.
    # This always runs so every build (local and CI) is self-contained.
    # -------------------------------------------------------------------------
    get_target_property(_qt6_qmake_location Qt6::qmake IMPORTED_LOCATION)
    execute_process(
            COMMAND "${_qt6_qmake_location}" -query QT_INSTALL_PREFIX
            RESULT_VARIABLE _qt6_qmake_rc
            OUTPUT_VARIABLE qt6_install_prefix
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (qt6_install_prefix)
        find_program(WINDEPLOYQT windeployqt
            HINTS "${qt6_install_prefix}"
            PATH_SUFFIXES bin)
        if (WINDEPLOYQT)
            add_custom_command(TARGET openkj POST_BUILD
                COMMAND "${WINDEPLOYQT}"
                        --release
                        --compiler-runtime
                        --no-translations
                        "$<TARGET_FILE:openkj>"
                COMMENT "Running windeployqt to bundle Qt DLLs..."
            )
        else ()
            message(WARNING "windeployqt not found — Qt DLLs will NOT be bundled.")
        endif ()
    else ()
        message(WARNING "QT_INSTALL_PREFIX not found — Qt DLLs will NOT be bundled.")
    endif ()
endif ()


