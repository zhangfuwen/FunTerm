cmake_minimum_required(VERSION 3.1)
project(funterm)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_C_FLAGS "-O0 -fexceptions -fPIC -MMD -pg -g -ggdb")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fpermissive")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-narrowing")

set(CMAKE_INSTALL_RPATH "/usr/share/funterm/data/")
set(CMAKE_BUILD_RPATH_USE_ORIGIN)

include(ExternalProject)

ExternalProject_Add(configor
        GIT_REPOSITORY "https://github.com/Nomango/configor"
        GIT_TAG "v0.9.15"
        SOURCE_DIR "${CMAKE_BINARY_DIR}/configor/src"
        BINARY_DIR "${CMAKE_BINARY_DIR}/configor/build"
        CONFIGURE_COMMAND ""
        UPDATE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        TEST_COMMAND ""
        )


ExternalProject_Add(handycpp
        GIT_REPOSITORY "https://github.com/zhangfuwen/handycpp"
        GIT_TAG "main"
        SOURCE_DIR "${CMAKE_BINARY_DIR}/handycpp/src"
        BINARY_DIR "${CMAKE_BINARY_DIR}/handycpp/build"
        CONFIGURE_COMMAND ""
        UPDATE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        TEST_COMMAND ""
        )


MESSAGE("c flag is ${CMAKE_C_FLAGS}")
MESSAGE("os is ${CMAKE_SYSTEM}")

MESSAGE("PROFECT_SOURCE_DIR " ${PROJECT_SOURCE_DIR})
MESSAGE("CMAKE_SOURCE_DIR " ${CMAKE_SOURCE_DIR})
MESSAGE("CMAKE_BUILD_TYPE " ${CMAKE_BUILD_TYPE})

include(GNUInstallDirs)
message("CMAKE_INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR}")
message("CMAKE_INSTALL_DATADIR ${CMAKE_INSTALL_DATADIR}")
message("CMAKE_INSTALL_DATAROOTDIR ${CMAKE_INSTALL_DATAROOTDIR}")
message("CMAKE_INSTALL_SHAREDSTATEDIR ${CMAKE_INSTALL_SHAREDSTATEDIR}")
message("CMAKE_INSTALL_LIBEXECDIR ${CMAKE_INSTALL_LIBEXECDIR}")
message("CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR}")

message("CMAKE_INSTALL_FULL_BINDIR ${CMAKE_INSTALL_FULL_BINDIR}")
message("CMAKE_INSTALL_FULL_DATADIR ${CMAKE_INSTALL_FULL_DATADIR}")
message("CMAKE_INSTALL_FULL_DATAROOTDIR ${CMAKE_INSTALL_FULL_DATAROOTDIR}")
message("CMAKE_INSTALL_FULL_SHAREDSTATEDIR ${CMAKE_INSTALL_FULL_SHAREDSTATEDIR}")
message("CMAKE_INSTALL_FULL_LIBEXECDIR ${CMAKE_INSTALL_FULL_LIBEXECDIR}")
message("CMAKE_INSTALL_FULL_LIBDIR ${CMAKE_INSTALL_FULL_LIBDIR}")


find_package(PkgConfig REQUIRED)


pkg_check_modules(GTKMM3 REQUIRED gtkmm-3.0) #
#pkg_check_modules(VTE REQUIRED vte-2.91) #  apt install libvte-2.91-dev

add_custom_command(OUTPUT gen_headers/preference_dialog.ui.h
        COMMAND mkdir ${CMAKE_SOURCE_DIR}/gen_headers ; ${CMAKE_SOURCE_DIR}/src/glade_code_gen --filename=${CMAKE_SOURCE_DIR}/res/preference_dialog.glade --quiet=true > ${CMAKE_SOURCE_DIR}/gen_headers/preference_dialog.ui.h
        )
add_executable(funterm src/FunTerm.cpp
        src/PanedContainer.cpp
        src/TerminalSession.cpp
        src/Preference.cpp
        gen_headers/preference_dialog.ui.h
        src/Tab.cpp
        )
add_dependencies(funterm configor)
add_dependencies(funterm handycpp)
target_include_directories(funterm
        PRIVATE
        gen_headers
        ${GTKMM3_INCLUDE_DIRS}
#        ${VTE_INCLUDE_DIRS} we don't use this one, we use the one we built in thirdparty
        ${CMAKE_BINARY_DIR}/configor/src/include
        ${CMAKE_BINARY_DIR}/handycpp/src/include)
set_property(TARGET funterm PROPERTY CXX_STANDARD 17)
message("gtkmm ${GTKMM3_INCLUDE_DIRS}")
target_include_directories(funterm PRIVATE thirdparty/install/include/vte-2.91/)
target_link_directories(funterm PRIVATE thirdparty/install/lib/x86_64-linux-gnu)
target_link_libraries(funterm
#        ${VTE_LIBRARIES}
        libvte-2.91.so
        ${GTKMM3_LIBRARIES}
        )
message(info "GTKMM3 libs:${GTKMM3_LIBRARIES}")
message(info "VTE LIBRARIES: ${VTE_LIBRARIES}")
if (CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(funterm PRIVATE NBUILD)
endif ()
#target_compile_definitions(funterm PRIVATE _GLIBCXX_USE_CXX11_ABI=0)
if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(funterm PRIVATE RES_FILE_DIR="${CMAKE_SOURCE_DIR}/res")
else()
    target_compile_definitions(funterm PRIVATE RES_FILE_DIR="/usr/share/funterm/data/")
endif()
#target_compile_options(funterm PRIVATE  -Wl,--no-enable-new-dtags -Wl,-rpath,${CMAKE_SOURCE_DIR}/thirdparty/install/lib/x86_64-linux-gnu/)

#set(CMAKE_BUILD_WITH_INSTALL_RPATH)
#set(CMAKE_INSTALL_RPATH "/usr/share/funterm/data/")
#set(CMAKE_BUILD_RPATH_USE_ORIGIN 1)

SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Dean") #required
set(CPACK_PACKAGE_NAME "funterm") # debian package name
set(CPACK_PACKAGE_VERSION "1.1")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA, "scripts/prerm;scripts/postrm;scripts/preinst;scripts/postinst")
#set(CPACK_DEBIAN_PACKAGE_DEPENDS "libvte")

INSTALL(FILES res/funterm.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ WORLD_EXECUTE)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/thirdparty/install/etc/profile.d/vte.sh DESTINATION /etc/profile.d/vte-funterm.sh PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ WORLD_EXECUTE)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/thirdparty/install/lib/x86_64-linux-gnu/libvte-2.91.so DESTINATION ${CMAKE_INSTALL_DATADIR}/funterm/data PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ WORLD_EXECUTE)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/thirdparty/install/lib/x86_64-linux-gnu/libvte-2.91.so.0 DESTINATION ${CMAKE_INSTALL_DATADIR}/funterm/data PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ WORLD_EXECUTE)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/thirdparty/install/lib/x86_64-linux-gnu/libvte-2.91.so.0.6900.0 DESTINATION ${CMAKE_INSTALL_DATADIR}/funterm/data PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ WORLD_EXECUTE)
INSTALL(TARGETS funterm DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ WORLD_EXECUTE)
INSTALL(FILES res/funterm.glade DESTINATION ${CMAKE_INSTALL_DATADIR}/funterm/data PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ)
INSTALL(FILES res/match_dialog.glade DESTINATION ${CMAKE_INSTALL_DATADIR}/funterm/data PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ)
INSTALL(FILES res/preference_dialog.glade DESTINATION ${CMAKE_INSTALL_DATADIR}/funterm/data PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ)
INSTALL(FILES res/headerbar.glade DESTINATION ${CMAKE_INSTALL_DATADIR}/funterm/data PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ)
INSTALL(FILES res/style.css DESTINATION ${CMAKE_INSTALL_DATADIR}/funterm/data PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ)
INSTALL(FILES res/funterm.png DESTINATION /usr/share/icons/hicolor/48x48/apps/ PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ WORLD_EXECUTE)
INSTALL(FILES res/funterm.conf DESTINATION /etc/rsyslog.d/ PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ WORLD_EXECUTE)
INSTALL(FILES language/zh_CN/LC_MESSAGES/messages.mo DESTINATION ${CMAKE_INSTALL_DATADIR}/funterm/data/language/zh_CN/LC_MESSAGES/ PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ)

INCLUDE(CPack)
# xgettext -k_ -o language/zh_CN/LC_MESSAGES/messages.po ./src/Engine.cpp
#  msgfmt -o language/zh_CN/LC_MESSAGES/messages.mo language/zh_CN/LC_MESSAGES/messages.po
