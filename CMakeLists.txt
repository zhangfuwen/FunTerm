cmake_minimum_required(VERSION 3.1)
project(funterm)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_C_FLAGS "-O0 -fexceptions -fPIC -MMD -pg -g -ggdb")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fpermissive")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-narrowing")

set(CMAKE_INSTALL_RPATH "/usr/share/funterm/data/")
set(CMAKE_BUILD_RPATH_USE_ORIGIN)

include(ExternalProject)

externalproject_add(
    configor
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

externalproject_add(
    handycpp
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

message("c flag is ${CMAKE_C_FLAGS}")
message("os is ${CMAKE_SYSTEM}")

message("PROFECT_SOURCE_DIR " ${PROJECT_SOURCE_DIR})
message("CMAKE_SOURCE_DIR " ${CMAKE_SOURCE_DIR})
message("CMAKE_BUILD_TYPE " ${CMAKE_BUILD_TYPE})

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

file(GLOB_RECURSE SRCS ${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/src/*.h)
message("SRCS ${SRCS}")

add_custom_target(
    translation_text
    COMMENT "Generating translation files"
    COMMAND xgettext -j -k_ -o res/language/zh_CN/LC_MESSAGES/messages.po ${SRCS}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(
    translation_binary
    COMMENT "Generating translation files"
    COMMAND msgfmt -o res/language/zh_CN/LC_MESSAGES/messages.mo res/language/zh_CN/LC_MESSAGES/messages.po
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

pkg_check_modules(GTKMM3 REQUIRED gtkmm-3.0) #
# pkg_check_modules(VTE REQUIRED vte-2.91) #  apt install libvte-2.91-dev

configure_file(res/funterm.desktop.in ${CMAKE_BINARY_DIR}/res/funterm.desktop)
configure_file(src/funterm_config.h.in ${PROJECT_BINARY_DIR}/include/funterm_config.h)
include_directories(${PROJECT_BINARY_DIR}/include)
add_custom_command(
    OUTPUT gen_headers/preference_dialog.ui.h
    COMMENT "Generating headers"
    COMMAND
        mkdir ${CMAKE_SOURCE_DIR}/gen_headers || true && ${CMAKE_SOURCE_DIR}/src/glade_code_gen
        --filename=${CMAKE_SOURCE_DIR}/res/preference_dialog.glade --quiet=true >
        ${CMAKE_SOURCE_DIR}/gen_headers/preference_dialog.ui.h
)
add_executable(
    funterm src/FunTerm.cpp src/PanedContainer.cpp src/TerminalSession.cpp src/Preference.cpp
            gen_headers/preference_dialog.ui.h src/Tab.cpp
)

add_dependencies(funterm configor)
add_dependencies(funterm handycpp)
add_dependencies(funterm translation_binary)
target_include_directories(
    funterm
    PRIVATE
        gen_headers
        ${GTKMM3_INCLUDE_DIRS}
        # ${VTE_INCLUDE_DIRS} we don't use this one, we use the one we built in thirdparty
        ${CMAKE_BINARY_DIR}/configor/src/include
        ${CMAKE_BINARY_DIR}/handycpp/src/include
)
set_property(TARGET funterm PROPERTY CXX_STANDARD 17)
message("gtkmm ${GTKMM3_INCLUDE_DIRS}")
target_include_directories(funterm PRIVATE thirdparty/install/include/vte-2.91/)
target_link_directories(funterm PRIVATE thirdparty/install/lib/x86_64-linux-gnu)
target_link_libraries(
    funterm
    # ${VTE_LIBRARIES}
    libvte-2.91.so
    ${GTKMM3_LIBRARIES}
)
message(info "GTKMM3 libs:${GTKMM3_LIBRARIES}")
message(info "VTE LIBRARIES: ${VTE_LIBRARIES}")
if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(funterm PRIVATE NBUILD)
endif()
# target_compile_definitions(funterm PRIVATE _GLIBCXX_USE_CXX11_ABI=0)
if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(funterm PRIVATE RES_FILE_DIR="${CMAKE_SOURCE_DIR}/res")
else()
    target_compile_definitions(
        funterm PRIVATE RES_FILE_DIR="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/funterm/data/"
    )
    message("CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}")
endif()
# target_compile_options(funterm PRIVATE  -Wl,--no-enable-new-dtags
# -Wl,-rpath,${CMAKE_SOURCE_DIR}/thirdparty/install/lib/x86_64-linux-gnu/)

# set(CMAKE_BUILD_WITH_INSTALL_RPATH) set(CMAKE_INSTALL_RPATH "/usr/share/funterm/data/")
# set(CMAKE_BUILD_RPATH_USE_ORIGIN 1)

pkg_check_modules(libnautilus-extension REQUIRED libnautilus-extension-4 IMPORTED_TARGET)
add_library(nautilus_funterm SHARED src/nautilus_extension.cpp)
target_include_directories(nautilus_funterm PRIVATE ${GTKMM3_INCLUDE_DIRS})
target_link_libraries(nautilus_funterm ${GTKMM3_LIBRARIES} PkgConfig::libnautilus-extension)

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Dean") # required
set(CPACK_PACKAGE_NAME "funterm") # debian package name
set(CPACK_PACKAGE_VERSION "1.1")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA, "scripts/prerm;scripts/postrm;scripts/preinst;scripts/postinst")
# set(CPACK_DEBIAN_PACKAGE_DEPENDS "libvte")

message("CMAKE_INSTALL_DATADIR ${CMAKE_INSTALL_DATADIR}")
message("CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}")
message("-- Current compiler flags CMAKE_CXX_FLAGS are: ${CMAKE_CXX_FLAGS}")
message("-- Current compiler flags CMAKE_C_FLAGS are: ${CMAKE_C_FLAGS}")

get_target_property(TARGET_COMPILE_OPTIONS funterm COMPILE_OPTIONS)
message("Compile options for target 'funterm': ${TARGET_COMPILE_OPTIONS}")
message("Compile options for 'funterm': $<TARGET_PROPERTY:funterm,COMPILE_OPTIONS>")

get_target_property(TARGET_COMPILE_DEFINITIONS funterm COMPILE_DEFINITIONS)
message("Compile definitions : ${TARGET_COMPILE_DEFINITIONS}")
message("Compile definitions : $<TARGET_PROPERTY:funterm,COMPILE_DEFINITIONS>")

install(
    FILES ${CMAKE_BINARY_DIR}/res/funterm.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ WORLD_EXECUTE
)
install(
    FILES ${CMAKE_SOURCE_DIR}/thirdparty/install/etc/profile.d/vte.sh
    DESTINATION /etc/profile.d/vte-funterm.sh
    PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ WORLD_EXECUTE
)
install(
    FILES ${CMAKE_SOURCE_DIR}/thirdparty/install/lib/x86_64-linux-gnu/libvte-2.91.so
    DESTINATION ${CMAKE_INSTALL_DATADIR}/funterm/data
    PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ WORLD_EXECUTE
)
install(
    FILES ${CMAKE_SOURCE_DIR}/thirdparty/install/lib/x86_64-linux-gnu/libvte-2.91.so.0
    DESTINATION ${CMAKE_INSTALL_DATADIR}/funterm/data
    PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ WORLD_EXECUTE
)
install(
    FILES ${CMAKE_SOURCE_DIR}/thirdparty/install/lib/x86_64-linux-gnu/libvte-2.91.so.0.6900.0
    DESTINATION ${CMAKE_INSTALL_DATADIR}/funterm/data
    PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ WORLD_EXECUTE
)
install(
    TARGETS funterm
    DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ WORLD_EXECUTE
)
install(
    TARGETS nautilus_funterm
    DESTINATION /usr/lib/x86_64-linux-gnu/nautilus/extensions-3.0/
    PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ WORLD_EXECUTE
)
install(
    FILES res/funterm.glade
    DESTINATION ${CMAKE_INSTALL_DATADIR}/funterm/data
    PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ
)
install(
    FILES res/match_dialog.glade
    DESTINATION ${CMAKE_INSTALL_DATADIR}/funterm/data
    PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ
)
install(
    FILES res/preference_dialog.glade
    DESTINATION ${CMAKE_INSTALL_DATADIR}/funterm/data
    PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ
)
install(
    FILES res/headerbar.glade
    DESTINATION ${CMAKE_INSTALL_DATADIR}/funterm/data
    PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ
)
install(
    FILES res/style.css
    DESTINATION ${CMAKE_INSTALL_DATADIR}/funterm/data
    PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ
)
install(
    FILES res/funterm.png
    DESTINATION /usr/share/icons/hicolor/48x48/apps/
    PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ WORLD_EXECUTE
)
install(
    FILES res/funterm.conf
    DESTINATION /etc/rsyslog.d/
    PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ WORLD_EXECUTE
)
install(
    FILES res/language/zh_CN/LC_MESSAGES/messages.mo
    DESTINATION ${CMAKE_INSTALL_DATADIR}/funterm/data/language/zh_CN/LC_MESSAGES/
    PERMISSIONS OWNER_READ OWNER_WRITE WORLD_READ
)
install(FILES res/fun.xjbcode.funterm.gschema.xml DESTINATION /usr/share/glib-2.0/schemas)

include(CPack)
# xgettext -k_ -o language/zh_CN/LC_MESSAGES/messages.po ./src/Engine.cpp msgfmt -o
# language/zh_CN/LC_MESSAGES/messages.mo language/zh_CN/LC_MESSAGES/messages.po

configure_file(scripts/git_hooks/pre-commit ${CMAKE_CURRENT_SOURCE_DIR}/.git/hooks/pre-commit COPYONLY)
execute_process(
    COMMAND ${GIT_EXECUTABLE} config commit.template scripts/git_hooks/project_commit_msg_template
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
)
