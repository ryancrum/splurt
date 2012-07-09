#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([splurt], [0.0.1], [ryan.j.crum@gmail.com])
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB(jpeg, [jpeg_read_header, jpeg_std_error, jpeg_stdio_src, jpeg_start_decompress, jpeg_read_scanlines, jpeg_finish_decompress, jpeg_destroy_decompress])
AC_CHECK_LIB(ncurses, [has_colors, start_color, init_pair, mvaddch])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h ncurses.h assert.h jpeglib.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
