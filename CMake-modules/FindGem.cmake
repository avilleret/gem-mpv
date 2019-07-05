###############################################################################
# CMake module to search for the Gem library (Graphic Environment for Puredata
#
# This is quick and dirty, no version check, no REQUIRED flag check...
#
###############################################################################

### Global Configuration Section
#
SET(_GEM_REQUIRED_VARS GEM_INCLUDE_DIR GEM_LIBRARY)

#
### GEM uses pkgconfig.
#
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_GEM QUIET Gem)
endif(PKG_CONFIG_FOUND)

#
### Look for the include files.
#
find_path(
  MPV_INCLUDE_DIR
  NAMES Gem/State.h
  HINTS
      ${PC_GEM_INCLUDEDIR}
      ${PC_GEM_INCLUDE_DIRS} # Unused for MPV but anyway
  DOC "MPV include directory"
)

mark_as_advanced(MPV_INCLUDE_DIR)
