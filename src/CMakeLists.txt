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
target_link_libraries(luna-qml-launcher
    ${GLIB2_LIBRARIES}
    ${WEBOS_APPLICATION_LIBRARIES}
    ${LUNA_SYSMGR_COMMON_LIBRARIES}
    ${JSON_LIBRARIES}
    Qt5::Core Qt5::Gui Qt5::Qml Qt5::Quick Qt5::WebEngine
    ${LUNA_SERVICE2_LIBRARIES})

webos_build_program(ADMIN)
