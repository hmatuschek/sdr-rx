# - Find libsdr-gui
#
#  LIBSDR_GUI_INCLUDE_DIRS - where to find libsdr/gui/gui.h
#  LIBSDR_GUI_LIBRARIES    - List of libraries when using libsdr-gui.
#  LIBSDR_GUI_FOUND        - True if libsdr-gui found.

if(LIBSDR_GUI_INCLUDE_DIRS)
  # Already in cache, be silent
  set(LIBSDR_GUI_FIND_QUIETLY TRUE)
endif(LIBSDR_GUI_INCLUDE_DIRS)

find_path(LIBSDR_GUI_INCLUDE_DIRS gui.hh PATH_SUFFIXES libsdr/gui)
find_library(LIBSDR_GUI_LIBRARY NAMES sdr-gui)

set(LIBSDR_GUI_LIBRARIES ${LIBSDR_GUI_LIBRARY})

# handle the QUIETLY and REQUIRED arguments and set LIBSDR_GUI_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBSDR_GUI DEFAULT_MSG LIBSDR_GUI_LIBRARIES LIBSDR_GUI_INCLUDE_DIRS)

mark_as_advanced(LIBSDR_GUI_LIBRARIES LIBSDR_GUI_INCLUDE_DIRS)
