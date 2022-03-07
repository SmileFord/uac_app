cmake_minimum_required(VERSION 2.8.0 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 11)
add_definitions(-fno-rtti)
PROJECT(uac_app)

include_directories(src/include)

set(LIB_SOURCE
    src/uevent.cpp
    src/uac_control.cpp
    src/graph/graph_control.cpp
    src/graph/uac_control_graph.cpp
    src/mpi/uac_control_mpi.cpp
)

set(SOURCE
    src/main.cpp
    ${LIB_SOURCE}
)

set(UAC_APP_DEPENDENT_LIBS
    pthread
    rockit
)

set(UAC_AUDIO_ALGORITHM
    ${CMAKE_CURRENT_LIST_DIR}/libs/libaec_wake_float.so
    ${CMAKE_CURRENT_LIST_DIR}/libs/libbeamform.so
    ${CMAKE_CURRENT_LIST_DIR}/libs/libagc.so
    ${CMAKE_CURRENT_LIST_DIR}/libs/libanr.so)
install(FILES ${UAC_AUDIO_ALGORITHM} DESTINATION lib)

ADD_EXECUTABLE(uac_app ${SOURCE})
target_link_libraries(uac_app ${UAC_APP_DEPENDENT_LIBS})

install(TARGETS uac_app DESTINATION bin)
install(DIRECTORY configs/ DESTINATION share/uac_app FILES_MATCHING PATTERN "*.json")
install(DIRECTORY . DESTINATION bin
        FILES_MATCHING PATTERN "*.sh"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_WRITE GROUP_EXECUTE
        WORLD_READ WORLD_WRITE WORLD_EXECUTE)
