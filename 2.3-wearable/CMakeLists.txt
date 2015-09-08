CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(cbhm C)

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(LIBDIR "${PREFIX}/lib")
SET(DATADIR "${PREFIX}/share")
SET(LOCALEDIR "${DATADIR}/locale")
SET(VERSION 0.1.0)

SET(SRCS src/main.c
	src/item_manager.c
	src/xconverter.c
	src/xhandler.c
	src/clipdrawer.c
	src/storage.c
)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

IF ( MDM_ENABLE )
	 ADD_DEFINITIONS( -DMDM_ENABLE )
	 SET(MDM_REQUIRED_PKGS "mdm")
ENDIF ( MDM_ENABLE )

INCLUDE(FindPkgConfig)
	pkg_check_modules(pkgs REQUIRED elementary eet appcore-efl appcore-common x11 ecore-x utilX eina evas ecore ecore-file ecore-evas edje ecore-input notification vconf vconf-internal-keys libsystemd-daemon efl-assist ${MDM_REQUIED_PKGS})

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -fvisibility=hidden -Wall -fPIC -g")

SET(CMAKE_CXX_FLAGS_PROFILING  "-O0 -std=c++0x -g -pg -fPIE")
SET(CMAKE_C_FLAGS_DEBUG        "-O0 -g -fPIE")
SET(CMAKE_CXX_FLAGS_DEBUG      "-O0 -std=c++0x -g -fPIE")
SET(CMAKE_C_FLAGS_RELEASE      "-O2 -g -fPIE")
SET(CMAKE_CXX_FLAGS_RELEASE    "-O2 -std=c++0x -g -fPIE")
SET(CMAKE_C_FLAGS_CCOV         "-O2 -g --coverage -fPIE")
SET(CMAKE_CXX_FLAGS_CCOV       "-O2 -std=c++0x -g --coverage -fPIE")

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -pie")

ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")

ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
#for XV extension screenshot
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS})

ADD_CUSTOM_TARGET(cbhmdrawer.edj
	   COMMAND edje_cc
	   -id ${CMAKE_CURRENT_SOURCE_DIR}/data/images
	   -sd ${CMAKE_CURRENT_SOURCE_DIR}/data/sounds
	   ${CMAKE_SOURCE_DIR}/data/themes/cbhmdrawer.edc ${CMAKE_BINARY_DIR}/data/themes/cbhmdrawer.edj
	   DEPENDS ${CMAKE_SOURCE_DIR}/data/themes/cbhmdrawer.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} cbhmdrawer.edj)

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin)
INSTALL(FILES ${CMAKE_BINARY_DIR}/data/themes/cbhmdrawer.edj DESTINATION ${DATADIR}/edje)
ADD_SUBDIRECTORY(po)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../systemd/cbhm.service DESTINATION ${LIBDIR}/systemd/user)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../systemd/cbhm.service DESTINATION ${LIBDIR}/systemd/system)
