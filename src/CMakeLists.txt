include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GLIB2_INCLUDE_DIRS}
    ${WEBOS_APPLICATION_INCLUDE_DIRS}
    ${LUNA_SERVICE2_INCLUDE_DIRS}
    ${LUNA_SYSMGR_COMMON_INCLUDE_DIRS}
    ${Qt5WebEngine_PRIVATE_INCLUDE_DIRS}
    ${Qt5Gui_PRIVATE_INCLUDE_DIRS})

set(SOURCES
    main.cpp
    lunaqmlapplication.cpp
    applicationdescription.cpp)

add_executable(luna-qml-launcher ${SOURCES} ${RESOURCES})
qt5_use_modules(luna-qml-launcher Core Gui Qml Quick WebEngine)
target_link_libraries(luna-qml-launcher
    ${GLIB2_LIBRARIES}
    ${WEBOS_APPLICATION_LIBRARIES}
    ${LUNA_SYSMGR_COMMON_LIBRARIES}
    -ljson-c
    ${LUNA_SERVICE2_LIBRARIES})

webos_build_program(ADMIN)
