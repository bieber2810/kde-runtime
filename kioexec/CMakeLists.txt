set(kioexec_SRCS main.cpp )


kde4_add_executable(kioexec NOGUI ${kioexec_SRCS})

target_link_libraries(kioexec ${KDE4_KIO_LIBS})

install(TARGETS kioexec DESTINATION ${LIBEXEC_INSTALL_DIR} )

