CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(cbhm C)

SET(SRCS src/main.c
	src/item_manager.c
	src/xconverter.c
	src/xhandler.c
	src/clipdrawer.c
	src/storage.c
)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED elementary eet appcore-efl appcore-common x11 ecore-x utilX eina evas ecore ecore-file ecore-evas edje ecore-input xext xcomposite svi pixman-1 enotify)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)
SET(EXTRA_CFLAGS "${EXTRA_CFLAGS}")
#SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -finstrument-functions")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")

ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
ADD_DEFINITIONS("-DFACTORYFS=\"$ENV{FACTORYFS}\"")
ADD_DEFINITIONS("-DTARGET")
ADD_DEFINITIONS("-DSLP_DEBUG")

ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
#for XV extension screenshot
SET(pkgs_LDFLAGS "${pkgs_LDFLAGS} -L/usr/lib -lsvi")
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS})

ADD_CUSTOM_TARGET(cbhmdrawer.edj 
		COMMAND edje_cc 
		-id ${CMAKE_CURRENT_SOURCE_DIR}/data/images		
		${CMAKE_SOURCE_DIR}/data/themes/cbhmdrawer.edc ${CMAKE_BINARY_DIR}/data/themes/cbhmdrawer.edj
		DEPENDS ${CMAKE_SOURCE_DIR}/data/themes/cbhmdrawer.edc
		DEPENDS ${CMAKE_SOURCE_DIR}/data/themes/extstyles.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} cbhmdrawer.edj)

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin)
INSTALL(FILES ${CMAKE_BINARY_DIR}/data/themes/cbhmdrawer.edj DESTINATION share/edje)
INSTALL(FILES ${CMAKE_BINARY_DIR}/data/images/cbhm_default_img.png DESTINATION share/cbhm/icons)
INSTALL(FILES ${CMAKE_BINARY_DIR}/data/init_script/cbhm DESTINATION /etc/init.d PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
