# - Find libsdr
#
#  LIBSDR_INCLUDE_DIRS - where to find sdr.h
#  LIBSDR_LIBRARIES    - List of libraries when using libsdr.
#  LIBSDR_FOUND        - True if libsdr found.

if(LIBSDR_INCLUDE_DIRS)
  # Already in cache, be silent
  set(LIBSDR_FIND_QUIETLY TRUE)
endif(LIBSDR_INCLUDE_DIRS)

find_path(LIBSDR_INCLUDE_DIRS sdr.hh PREFIX "libsdr")
find_library(LIBSDR_LIBRARIES NAMES sdr)

# handle the QUIETLY and REQUIRED arguments and set LIBSDR_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libsdr DEFAULT_MSG LIBSDR_LIBRARIES LIBSDR_INCLUDE_DIRS)

mark_as_advanced(LIBSDR_LIBRARIES LIBSDR_INCLUDE_DIRS)
