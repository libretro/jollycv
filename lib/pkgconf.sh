#!/bin/sh

set -eu

prefix="$1"
libdir="$2"

case "$libdir" in
  "$prefix"* )
    lib_path="\${exec_prefix}${libdir#$prefix}"
  ;;
  * )
    lib_path="$libdir"
  ;;
esac

printf %s\\n "$lib_path"

exit 0
