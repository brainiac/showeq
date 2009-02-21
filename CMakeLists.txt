# Required cmake version
cmake_minimum_required(VERSION 2.4)

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# Let me see the command line
set(CMAKE_VERBOSE_MAKEFILE off)

# Debug build please
set(CMAKE_BUILD_TYPE Debug)

# Put the include dirs which are in the source or build tree
# before all other include dirs, so the headers in the sources
# are prefered over the already installed ones
# since cmake 2.4.1
set(CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE ON)

project(showeq)
set(VERSION "5.13.1.0")
set(PKGDATADIR ${DATA_INSTALL_DIR})

message(STATUS "Version: ${VERSION}")
message(STATUS "DATA_INSTALL_DIR: ${DATA_INSTALL_DIR}")
message(STATUS "PKGDATADIR: ${PKGDATADIR}")

option(WITH_ANSI "enforce strict ISO C++ conforming" ON)

# Our own modules
set(CMAKE_MODULE_PATH
  ${CMAKE_SOURCE_DIR}/cmake/Modules
)

# Include used modules
include(DefineInstallationPaths)
include(CheckIncludeFileCXX)
include(CheckIncludeFilesCXX)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckTypeSizeCXX)
include(CheckCXXCompilerFlag)
include(CheckMkdirArgs)
include(FindQt4)
include(KDE4Macros)

# Setup short variable path names
set(BINDIR ${BIN_INSTALL_DIR})
set(SBINDIR ${SBIN_INSTALL_DIR})
set(SYSCONFDIR ${SYSCONF_INSTALL_DIR})
set(LOCALSTATEDIR ${LOCALSTATE_INSTALL_DIR})
set(MANDIR ${MAN_INSTALL_DIR})
set(DATADIR ${DATA_INSTALL_DIR})

# library checks
find_package(Qt4 REQUIRED)
find_package(ZLIB REQUIRED)
check_library_exists(pcap pcap_open_offline "" HAVE_LIBPCAP)

check_include_file_cxx(netinet/in.h HAVE_NETINET_IN_H)
check_include_file_cxx(netinet/ip.h HAVE_NETINET_IP_H)
check_include_file_cxx(arpa/inet.h HAVE_ARPA_INET_H)
check_include_file_cxx(sys/socket.h HAVE_SYS_SOCKET_H)
check_include_file_cxx(pcap.h HAVE_PCAP_H)

check_function_exists(strnlen HAVE_STRNLEN)

# Defines and includes
add_definitions(
    ${PCAP_DEFINITIONS}
    ${ZLIB_DEFINITIONS}
    ${QT_DEFINITIONS}
)

include_directories(
    ${PCAP_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    "${QT_INCLUDE_DIR}/Qt3Support"
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

subdirs(src doc conf)
