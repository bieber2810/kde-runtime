set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )

########### next target ###############

set(testtrash_SRCS 
    testtrash.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/../trashimpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../trashsizecache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../discspaceutil.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../kinterprocesslock.cpp
)

kde4_add_unit_test(testtrash ${testtrash_SRCS})

set(kio_trash_LIBS ${KDE4_KIO_LIBS} ${KDE4_SOLID_LIBS})

target_link_libraries(testtrash ${kio_trash_LIBS} ${QT_QTTEST_LIBRARY})
if(APPLE)
    target_link_libraries(testtrash "-framework DiskArbitration")
endif(APPLE)

### next target ###

set(lockingtest_SRCS lockingtest.cpp ../kinterprocesslock.cpp )

kde4_add_executable(lockingtest NOGUI ${lockingtest_SRCS})

target_link_libraries(lockingtest ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY})
