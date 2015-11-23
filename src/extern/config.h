/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* The canonical host libsigrok will run on. */
#define CONF_HOST "x86_64-unknown-linux-gnu"

/* Build-time version of libftdi. */
#define CONF_LIBFTDI_VERSION "0.20"

/* Build-time version of libgpib. */
/* #undef CONF_LIBGPIB_VERSION */

/* Build-time version of librevisa. */
/* #undef CONF_LIBREVISA_VERSION */

/* Build-time version of libserialport. */
#define CONF_LIBSERIALPORT_VERSION "0.1.1"

/* Build-time version of libusb. */
#define CONF_LIBUSB_1_0_VERSION "1.0.20"

/* Build-time version of libzip. */
#define CONF_LIBZIP_VERSION "0.11.2"

/* define if the compiler supports basic C++11 syntax */
#define HAVE_CXX11 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Whether to support Saleae Logic16 device. */
#define HAVE_HW_SALEAE_LOGIC16 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Whether libftdi is available. */
#define HAVE_LIBFTDI 1

/* Whether libgpib is available. */
/* #undef HAVE_LIBGPIB */

/* Whether libieee1284 is available. */
/* #undef HAVE_LIBIEEE1284 */

/* Whether librevisa is available. */
/* #undef HAVE_LIBREVISA */

/* Whether libusb is available. */
#define HAVE_LIBUSB_1_0 1

/* Define to 1 if the system has the type `libusb_os_handle'. */
/* #undef HAVE_LIBUSB_OS_HANDLE */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Specifies whether we have RPC support. */
#define HAVE_RPC 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Specifies whether we have the stoi and stod functions. */
/* #undef HAVE_STOI_STOD */

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/mman.h> header file. */
#define HAVE_SYS_MMAN_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/timerfd.h> header file. */
#define HAVE_SYS_TIMERFD_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `zip_discard' function. */
#define HAVE_ZIP_DISCARD 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "sigrok-devel@lists.sourceforge.net"

/* Define to the full name of this package. */
#define PACKAGE_NAME "libsigrok"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "libsigrok 0.4.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libsigrok"

/* Define to the home page for this package. */
#define PACKAGE_URL "http://www.sigrok.org"

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.4.0"

/* Whether last argument to pyg_flags_get_value() is signed. */
/* #undef PYGOBJECT_FLAGS_SIGNED */

/* Binary age of libsigrok. */
#define SR_LIB_VERSION_AGE 0

/* Binary version of libsigrok. */
#define SR_LIB_VERSION_CURRENT 2

/* Binary revision of libsigrok. */
#define SR_LIB_VERSION_REVISION 0

/* Binary version triple of libsigrok. */
#define SR_LIB_VERSION_STRING "2:0:0"

/* Major version number of libsigrok. */
#define SR_PACKAGE_VERSION_MAJOR 0

/* Micro version number of libsigrok. */
#define SR_PACKAGE_VERSION_MICRO 0

/* Minor version number of libsigrok. */
#define SR_PACKAGE_VERSION_MINOR 4

/* Version of libsigrok. */
#define SR_PACKAGE_VERSION_STRING "0.4.0-git-67f890d"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Enable large inode numbers on Mac OS X 10.5.  */
#ifndef _DARWIN_USE_64_BIT_INODE
# define _DARWIN_USE_64_BIT_INODE 1
#endif

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* The targeted POSIX standard. */
#ifndef _POSIX_C_SOURCE
# define _POSIX_C_SOURCE 200112L
#endif
