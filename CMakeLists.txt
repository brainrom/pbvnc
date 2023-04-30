project(pbvnc)

set(COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libvnc/common)
set(LIBVNCCLIENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libvnc/libvncclient)

find_package(JPEG)
find_package(ZLIB)

set(LIBVNCCLIENT_SOURCES
    ${LIBVNCCLIENT_DIR}/cursor.c
    ${LIBVNCCLIENT_DIR}/listen.c
    ${LIBVNCCLIENT_DIR}/rfbproto.c
    ${LIBVNCCLIENT_DIR}/sockets.c
    ${LIBVNCCLIENT_DIR}/vncviewer.c
    ${COMMON_DIR}/sockets.c

#Crypto
    ${COMMON_DIR}/crypto_included.c
	${COMMON_DIR}/sha1.c
	${COMMON_DIR}/d3des.c

    ${COMMON_DIR}/turbojpeg.c
    ${LIBVNCCLIENT_DIR}/tls_none.c
    ${COMMON_DIR}/minilzo.c
)


add_library(vncclient ${LIBVNCCLIENT_SOURCES})
target_include_directories(vncclient PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/libvnc ${COMMON_DIR})
target_link_libraries(vncclient
                      ${ADDITIONAL_LIBS}
                      ${ZLIB_LIBRARIES}
                      ${LZO_LIBRARIES}
                      ${JPEG_LIBRARIES}
		      ${CRYPTO_LIBRARIES}
                      ${GNUTLS_LIBRARIES}
                      ${OPENSSL_LIBRARIES}
)

add_executable(pbvnc.app main.c utils.c)
target_include_directories(pbvnc.app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libvnc)
target_link_libraries(pbvnc.app vncclient ${CMAKE_THREAD_LIBS_INIT} m)

set_property(TARGET pbvnc.app PROPERTY C_STANDARD 99)

target_link_libraries(pbvnc.app -lpthread -linkview)