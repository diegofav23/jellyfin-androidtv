
# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

#include(ExternalProject)

project("androidtv")

# TODO try to compile all integrated with cmake

#ExternalProject_Add(
#        dav1d_lib
#        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/dav1d
#        CONFIGURE_COMMAND meson setup ${CMAKE_CURRENT_SOURCE_DIR}/libs/dav1d -Denable_tests=false -Db_lto=true -Dstack_alignment=16
#            --cross-file=${CMAKE_CURRENT_SOURCE_DIR}/libs/prefix/${ANDROID_ABI}/crossfile.txt
#            --prefix=${CMAKE_CURRENT_SOURCE_DIR}/libs/prefix/${ANDROID_ABI} --libdir=lib
#        BUILD_COMMAND meson compile
#        INSTALL_COMMAND meson install
#        BUILD_BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/libs/prefix/${ANDROID_ABI}/lib/libdav1d.so
#)
#add_library(dav1d SHARED IMPORTED)
#add_dependencies(dav1d dav1d_lib)
#set_target_properties(
#        dav1d
#        PROPERTIES
#        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/libs/prefix/${ANDROID_ABI}/lib/libdav1d.so
#        IMPORTED_NO_SONAME TRUE
#)

add_library(${CMAKE_PROJECT_NAME} SHARED
    # List C/C++ source files with relative paths to this CMakeLists.txt.
    androidtv.cpp)

#add_subdirectory(libs/mbedtls EXCLUDE_FROM_ALL)

add_library(avcodec SHARED IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/third_party/${ANDROID_ABI}/libavcodec.so)

add_library(avdevice SHARED IMPORTED)
set_target_properties(avdevice PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/third_party/${ANDROID_ABI}/libavdevice.so)

add_library(avfilter SHARED IMPORTED)
set_target_properties(avfilter PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/third_party/${ANDROID_ABI}/libavfilter.so)

add_library(avformat SHARED IMPORTED)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/third_party/${ANDROID_ABI}/libavformat.so)

add_library(avutil SHARED IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/third_party/${ANDROID_ABI}/libavutil.so)

add_library(c++_shared SHARED IMPORTED)
set_target_properties(c++_shared PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/third_party/${ANDROID_ABI}/libc++_shared.so)

add_library(mpv SHARED IMPORTED)
set_target_properties(mpv PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/third_party/${ANDROID_ABI}/libmpv.so)

add_library(player SHARED IMPORTED)
set_target_properties(player PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/third_party/${ANDROID_ABI}/libplayer.so)

add_library(swresample SHARED IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/third_party/${ANDROID_ABI}/libswresample.so)

add_library(swscale SHARED IMPORTED)
set_target_properties(swscale PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/third_party/${ANDROID_ABI}/libswscale.so)

target_link_libraries(${CMAKE_PROJECT_NAME}
        android
        log
        EGL
        GLESv2
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        c++_shared
        mpv
        player
        swresample
        swscale
)

#add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy
#        ${CMAKE_CURRENT_SOURCE_DIR}/libs/prefix/${ANDROID_ABI}/lib/libdav1d.so
#        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libdav1d.so
#)
