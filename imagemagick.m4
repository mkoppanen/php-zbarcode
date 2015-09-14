#########################################################
# Locate ImageMagick configuration program
# ImageMagick has the config program:
# bin/Wand-config
# bin/MagickWand-config
#
# Sets
#  IM_WAND_BINARY
#  IM_IMAGEMAGICK_PREFIX
#  IM_IMAGEMAGICK_VERSION
#  IM_IMAGEMAGICK_VERSION_MASK
#

#########################################################

AC_DEFUN([IM_FIND_IMAGEMAGICK],[

  AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
  if test "x$PKG_CONFIG" = "xno"; then
    AC_MSG_RESULT([pkg-config not found])
    AC_MSG_ERROR([Please reinstall the pkg-config distribution])
  fi

  if test -z "$AWK"; then
    AC_MSG_ERROR([awk not found])
  fi

  AC_MSG_CHECKING(ImageMagick MagickWand API configuration program)

  LMW_ARRAY=$(find /usr/lib/ -name *Wand-config -exec dirname {} \;)

  if test "$2" != "yes"; then
    for i in "$2/bin" /usr/local/bin /usr/bin /opt/bin /opt/local/bin $LMW_ARRAY;
    do
      if test -r "${i}/MagickWand-config"; then
        IM_WAND_BINARY="${i}/MagickWand-config"
        IM_IMAGEMAGICK_PREFIX=$i
        break
      fi

      if test -r "${i}/Wand-config"; then
        IM_WAND_BINARY="${i}/Wand-config"
        IM_IMAGEMAGICK_PREFIX=$i
        break
      fi
    done
  else
    for i in /usr/local /usr /opt /opt/local $LMW_ARRAY;
    do
      if test -r "${i}/MagickWand-config"; then
        IM_WAND_BINARY="${i}/MagickWand-config"
        IM_IMAGEMAGICK_PREFIX=$i
        break
      fi

      if test -r "${i}/Wand-config"; then
        IM_WAND_BINARY="${i}/Wand-config"
        IM_IMAGEMAGICK_PREFIX=$i
        break
      fi
    done
  fi

  if test "x" = "x$IM_WAND_BINARY"; then
    AC_MSG_ERROR(not found. Please provide a path to MagickWand-config or Wand-config program.)
  fi
  AC_MSG_RESULT([found in $IM_WAND_BINARY])

# This is used later for cflags and libs
  export PKG_CONFIG_PATH="${IM_IMAGEMAGICK_PREFIX}/${PHP_LIBDIR}/pkgconfig"
  
# Check version
#
  IM_IMAGEMAGICK_VERSION=`$IM_WAND_BINARY --version`
  IM_IMAGEMAGICK_VERSION_MASK=`echo $IM_IMAGEMAGICK_VERSION | $AWK 'BEGIN { FS = "."; } { printf "%d", ($[1] * 1000 + $[2]) * 1000 + $[3];}'`

  AC_MSG_CHECKING(if ImageMagick version is at least $1)
  if test "$IM_IMAGEMAGICK_VERSION_MASK" -ge $1; then
    AC_MSG_RESULT(found version $IM_IMAGEMAGICK_VERSION)
  else
    AC_MSG_ERROR(no. You need at least Imagemagick version $1 to use this extension.)
  fi

# Potential locations for the header
# include/wand/magick-wand.h
# include/ImageMagick/wand/MagickWand.h
# include/ImageMagick-6/wand/MagickWand.h
# include/ImageMagick-7/MagickWand/MagickWand.h

  AC_MSG_CHECKING(for MagickWand.h or magick-wand.h header)

  IM_PREFIX=`$IM_WAND_BINARY --prefix`
  IM_MAJOR_VERSION=`echo $IM_IMAGEMAGICK_VERSION | $AWK 'BEGIN { FS = "."; } {print $[1]}'`

  # Try the header formats from newest to oldest
  if test -r "${IM_PREFIX}/include/ImageMagick-${IM_MAJOR_VERSION}/MagickWand/MagickWand.h"; then

    IM_INCLUDE_FORMAT="MagickWand/MagickWand.h"
    IM_HEADER_STYLE="SEVEN"
    AC_DEFINE([IM_MAGICKWAND_HEADER_STYLE_SEVEN], [1], [ImageMagick 7.x style header])

    AC_MSG_RESULT([${IM_PREFIX}/include/ImageMagick-${IM_MAJOR_VERSION}/MagickWand/MagickWand.h])

  elif test -r "${IM_PREFIX}/include/ImageMagick-${IM_MAJOR_VERSION}/wand/MagickWand.h"; then

    IM_INCLUDE_FORMAT="wand/MagickWand.h"
    IM_HEADER_STYLE="SIX"
    AC_DEFINE([IM_MAGICKWAND_HEADER_STYLE_SIX], [1], [ImageMagick 6.x style header])

    AC_MSG_RESULT([${IM_PREFIX}/include/ImageMagick-${IM_MAJOR_VERSION}/wand/MagickWand.h])

  elif test -r "${IM_PREFIX}/include/ImageMagick/wand/MagickWand.h"; then

    IM_INCLUDE_FORMAT="wand/MagickWand.h"
    IM_HEADER_STYLE="SIX"
    AC_DEFINE([IM_MAGICKWAND_HEADER_STYLE_SIX], [1], [ImageMagick 6.x style header])

    AC_MSG_RESULT([${IM_PREFIX}/include/ImageMagick/wand/MagickWand.h])

  elif test -r "${IM_PREFIX}/include/ImageMagick/wand/magick-wand.h"; then

    IM_INCLUDE_FORMAT="wand/magick-wand.h"
    IM_HEADER_STYLE="OLD"
    AC_DEFINE([IM_MAGICKWAND_HEADER_STYLE_OLD], [1], [ImageMagick old style header])

    AC_MSG_RESULT([${IM_PREFIX}/include/wand/magick-wand.h])

  else
    AC_MSG_ERROR([Unable to find MagickWand.h or magick-wand.h header])
  fi
  
#
# The cflags and libs
#
  IM_IMAGEMAGICK_LIBS=`$IM_WAND_BINARY --libs`
  IM_IMAGEMAGICK_CFLAGS=`$IM_WAND_BINARY --cflags`

  export IM_IMAGEMAGICK_PREFIX
  export IM_WAND_BINARY
  export IM_IMAGEMAGICK_VERSION
  export IM_IMAGEMAGICK_VERSION_MASK
  export IM_INCLUDE_FORMAT
  export IM_HEADER_STYLE

  export IM_IMAGEMAGICK_LIBS
  export IM_IMAGEMAGICK_CFLAGS
])
