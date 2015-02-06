
AC_INIT([main.c])

AC_SUBST(VERSION) 

AC_CONFIG_SUBDIRS([libUseful-2.1])

AC_PROG_CC 
AC_LANG_C 
AC_PROG_MAKE_SET 
AC_PROG_INSTALL 
AC_HEADER_STDC 
AC_SYS_LARGEFILE

AC_ARG_ENABLE(xattr, [  --enable-xattr       enable extended file attributes support], cf_use_xattr=$enableval )


if test "$cf_use_xattr" = "yes"
then
AC_DEFINE([USE_XATTR])
fi


dnl read Makefile.in and write Makefile 
AC_OUTPUT(Makefile)


echo
echo
echo "############# Hashrat Build Config: #############"
if test "$ac_cv_sys_file_offset_bits" = "64"
then
echo "Largefiles ( > 2GB ) enabled"
else
echo "Largefiles ( > 2GB ) NOT enabled"
fi

if test "$cf_use_xattr" = "yes"
then
echo "File system attributes (xattr) enabled"
else
echo "File system attributes (xattr) NOT enabled"
fi


