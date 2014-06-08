dvdreadlib="-ldvdread"
dvdreadmsg="[--minilibs]"
if test "$dvdread" = "external"; then
    dvdreadmsg="[--minicflags]"
    dvdreadcflags="-I$dvdreaddir"
    extracflags="-DDVDNAV_USES_EXTERNAL_DVDREAD"
fi

usage()
{
	cat <<EOF
Usage: dvdnav-config [OPTIONS] [LIBRARIES]
Options:
	[--prefix[=DIR]]
	[--version]
        [--libs]
	[--cflags]
        $dvdreadmsg
EOF
	exit $1
}

if test $# -eq 0; then
	usage 1 1>&2
fi

while test $# -gt 0; do
  case "$1" in
  -*=*) optarg=`echo "$1" | sed 's/[-_a-zA-Z0-9]*=//'` ;;
  *) optarg= ;;
  esac

  case $1 in
    --prefix)
      echo_prefix=yes
      ;;
    --version)
      echo $version
      ;;
    --cflags)
      echo_cflags=yes
      ;;
    --minicflags)
      if test "$dvdread" = "external"; then
          echo_minicflags=yes
      else
          usage 1 1>&2
      fi
      ;;
    --libs)
      echo_libs=yes
      ;;
    --minilibs)
          echo_minilibs=yes
      ;;
    *)
      usage 1 1>&2
      ;;
  esac
  shift
done

if test "$echo_prefix" = "yes"; then
	echo $prefix
fi

if test "$echo_cflags" = "yes"; then
      echo -I$prefix/include $dvdreadcflags $extracflags $threadcflags
fi

if test "$echo_minicflags" = "yes"; then
      echo -I$prefix/include -I$prefix/include/dvdnav $extracflags $threadcflags
fi

if test "$echo_libs" = "yes"; then
      echo -L$libdir -ldvdnav $dvdreadlib $threadlib
fi      

if test "$echo_minilibs" = "yes"; then
      echo -L$libdir -ldvdnavmini $threadlib
fi
