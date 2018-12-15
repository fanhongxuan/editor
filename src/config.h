/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define this label if your system uses case-insensitive file names */
/* #undef CASE_INSENSITIVE_FILENAMES */

/* Define this label if you wish to check the regcomp() function at run time
   for correct behavior. This function is currently broken on Cygwin. */
/* #undef CHECK_REGCOMP */

/* You can define this label to be a string containing the name of a
   site-specific configuration file containing site-wide default options. The
   files /etc/ctags.conf and /usr/local/etc/ctags.conf are already checked, so
   only define one here if you need a file somewhere else. */
/* #undef CUSTOM_CONFIGURATION_FILE */


/* Define this as desired.
 * 1:  Original ctags format
 * 2:  Extended ctags format with extension flags in EX-style comment.
 */
#define DEFAULT_FILE_FORMAT 2


/* Define to 1 if gcov is instrumented. */
/* #undef ENABLE_GCOV */

/* Define the string to check (in executable name) for etags mode */
#define ETAGS "etags"


/* Define this label to use the system sort utility (which is probably more
*  efficient) over the internal sorting algorithm.
*/
#ifndef INTERNAL_SORT
# define EXTERNAL_SORT 1
#endif


/* Define this value if aspell is available. */
/* #undef HAVE_ASPELL */

/* Define to 1 if you have the `asprintf' function. */
#define HAVE_ASPRINTF 1

/* Define to 1 if you have the `chmod' function. */
/* #undef HAVE_CHMOD */

/* Define to 1 if you have the `chsize' function. */
/* #undef HAVE_CHSIZE */

/* Define to 1 if you have the `clock' function. */
#define HAVE_CLOCK 1

/* Define to 1 if you have the declaration of `_NSGetEnviron', and to 0 if you
   don't. */
#define HAVE_DECL__NSGETENVIRON 0

/* Define to 1 if you have the declaration of `__environ', and to 0 if you
   don't. */
#define HAVE_DECL___ENVIRON 1

/* Define to 1 if you have the <dirent.h> header file. */
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the <errno.h> header file. */
#define HAVE_ERRNO_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `fgetpos' function. */
#define HAVE_FGETPOS 1

/* Define to 1 if you have the `findfirst' function. */
/* #undef HAVE_FINDFIRST */

/* Define to 1 if you have the `fnmatch' function. */
#define HAVE_FNMATCH 1

/* Define to 1 if you have the <fnmatch.h> header file. */
#define HAVE_FNMATCH_H 1

/* Define to 1 if you have the `ftruncate' function. */
/* #undef HAVE_FTRUNCATE */

/* Define this value if support multibyte character encoding. */
#define HAVE_ICONV 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <io.h> header file. */
/* #undef HAVE_IO_H */

/* Define this value if jansson is available. */
/* #undef HAVE_JANSSON */

/* Define this value if libxml is available. */
/* #undef HAVE_LIBXML */

/* Define this value if libyaml is available. */
/* #undef HAVE_LIBYAML */

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the `mblen' function. */
#define HAVE_MBLEN 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mkstemp' function. */
#define HAVE_MKSTEMP 1

/* Define to 1 if you have the `opendir' function. */
#define HAVE_OPENDIR 1

/* Define to 1 if you have the `putenv' function. */
/* #undef HAVE_PUTENV */

/* Define to 1 if you have the `regcomp' function. */
#define HAVE_REGCOMP 1

/* Define to 1 if you have the `remove' function. */
#define HAVE_REMOVE 1

/* Define to 1 if you have the `scandir' function. */
#define HAVE_SCANDIR 1

/* Define this value if libseccomp is available. */
/* #undef HAVE_SECCOMP */

/* Define to 1 if you have the `setenv' function. */
#define HAVE_SETENV 1

/* Define to 1 if you have the <stat.h> header file. */
/* #undef HAVE_STAT_H */

/* Define this macro if the field "st_ino" exists in struct stat in
   <sys/stat.h>. */
#define HAVE_STAT_ST_INO 1

/* Define to 1 if you have the <stdbool.h> header file. */
#define HAVE_STDBOOL_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#define HAVE_STRCASECMP 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the `stricmp' function. */
/* #undef HAVE_STRICMP */

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strncasecmp' function. */
#define HAVE_STRNCASECMP 1

/* Define to 1 if you have the `strnicmp' function. */
/* #undef HAVE_STRNICMP */

/* Define to 1 if you have the `strstr' function. */
#define HAVE_STRSTR 1

/* Define to 1 if you have the <sys/dir.h> header file. */
#define HAVE_SYS_DIR_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/times.h> header file. */
#define HAVE_SYS_TIMES_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/wait.h> header file. */
#define HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the `tempnam' function. */
/* #undef HAVE_TEMPNAM */

/* Define to 1 if you have the `times' function. */
/* #undef HAVE_TIMES */

/* Define to 1 if you have the <time.h> header file. */
#define HAVE_TIME_H 1

/* Define to 1 if you have the `truncate' function. */
#define HAVE_TRUNCATE 1

/* Define to 1 if you have the <types.h> header file. */
/* #undef HAVE_TYPES_H */

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if the system has the type `_Bool'. */
/* #undef HAVE__BOOL */

/* Define to 1 if you have the `_findfirst' function. */
/* #undef HAVE__FINDFIRST */

/* Define this value if the platform uses "lib" as prefix for iconv functions.
   */
/* #undef ICONV_USE_LIB_PREFIX */

/* Define as the maximum integer on your system if not defined <limits.h>. */
/* #undef INT_MAX */

/* Define to the appropriate size for tmpnam() if <stdio.h> does not define
   this. */
/* #undef L_tmpnam */

/* Define this label if you want macro tags (defined lables) to use patterns
   in the EX command by default (original ctags behavior is to use line
   numbers). */
/* #undef MACROS_USE_PATTERNS */

/* Define to 1 if your system uses MS-DOS style path. */
/* #undef MSDOS_STYLE_PATH */

/* If you receive error or warning messages indicating that you are missing a
   prototype for, or a type mismatch using, the following function, define
   this label and remake. */
/* #undef NEED_PROTO_FGETPOS */

/* If you receive error or warning messages indicating that you are missing a
   prototype for, or a type mismatch using, the following function, define
   this label and remake. */
/* #undef NEED_PROTO_FTRUNCATE */

/* If you receive error or warning messages indicating that you are missing a
   prototype for, or a type mismatch using, the following function, define
   this label and remake. */
/* #undef NEED_PROTO_GETENV */

/* If you receive error or warning messages indicating that you are missing a
   prototype for, or a type mismatch using, the following function, define
   this label and remake. */
/* #undef NEED_PROTO_LSTAT */

/* If you receive error or warning messages indicating that you are missing a
   prototype for, or a type mismatch using, the following function, define
   this label and remake. */
/* #undef NEED_PROTO_MALLOC */

/* If you receive error or warning messages indicating that you are missing a
   prototype for, or a type mismatch using, the following function, define
   this label and remake. */
/* #undef NEED_PROTO_REMOVE */

/* If you receive error or warning messages indicating that you are missing a
   prototype for, or a type mismatch using, the following function, define
   this label and remake. */
/* #undef NEED_PROTO_STAT */

/* If you receive error or warning messages indicating that you are missing a
   prototype for, or a type mismatch using, the following function, define
   this label and remake. */
/* #undef NEED_PROTO_TRUNCATE */

/* If you receive error or warning messages indicating that you are missing a
   prototype for, or a type mismatch using, the following function, define
   this label and remake. */
/* #undef NEED_PROTO_UNLINK */

/* Define this is you have a prototype for putenv() in <stdlib.h>, but doesn't
   declare its argument as "const char *". */
/* #undef NON_CONST_PUTENV_PROTOTYPE */

/* Package name. */
#define PACKAGE "universal-ctags"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "universal-ctags"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "universal-ctags 0.0.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "universal-ctags"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.0.0"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* If you wish to change the directory in which temporary files are stored,
   define this label to the directory desired. */
#define TMPDIR "/tmp"

/* whether or not to use <stdbool.h>. */
#define USE_STDBOOL_H 1

/* Package version. */
#define VERSION "0.0.0"

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

/* type for 'bool' if <stdbool.h> is missing or broken. */
/* #undef bool */

/* Define to the appropriate type if <time.h> does not define this. */
/* #undef clock_t */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* value of 'false' if <stdbool.h> is missing or broken. */
/* #undef false */

/* Define to long if <stdio.h> does not define this. */
/* #undef fpos_t */

/* Define to `long int' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define remove to unlink if you have unlink(), but not remove(). */
/* #undef remove */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* value of 'true' if <stdbool.h> is missing or broken. */
/* #undef true */
