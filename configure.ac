AC_INIT([ibus-xkb-layouts],
        [0.0.0],
        [http://github.com/suzhe/ibus-xkb-layouts])

AC_PREREQ([2.50])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.10])
AC_GNU_SOURCE

AM_DISABLE_STATIC
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_ISC_POSIX
AC_HEADER_STDC

PKG_PROG_PKG_CONFIG([0.20])

AC_ARG_WITH([xkb-rules-xml],
            AC_HELP_STRING([--with-xkb-rules-xml=${datadir}/X11/xkb/rules/evdev.xml],
                           [Specify the XKB rules xml file to be used.]),
            [XKB_RULES_XML_FILE=$withval],
            [XKB_RULES_XML_FILE="/usr/share/X11/xkb/rules/evdev.xml"])

AC_CHECK_FILE([$XKB_RULES_XML_FILE],[],[AC_MSG_ERROR([$XKB_RULES_XML_FILE is not available.])])

AC_SUBST([XKB_RULES_XML_FILE])
AC_SUBST([datarootdir])

AC_ARG_WITH([ranked-layouts],
            AC_HELP_STRING([--with-ranked-layouts=cz,ru,...],
                           [Specify the ranked layouts with comma separated.]),
            [RANKED_LAYOUTS=$withval],
            [RANKED_LAYOUTS="us,ara,bg,cz,dev,gr,gur,in,mkd,ru,ua,mal"])

PREDEFINED_MACROS=$(printf "%s %s" \
                    "-DXKB_RULES_XML_FILE=\\\"$XKB_RULES_XML_FILE\\\"" \
                    "-DRANKED_LAYOUTS=\\\"$RANKED_LAYOUTS\\\"")
AC_SUBST([RANKED_LAYOUTS])
AC_SUBST([PREDEFINED_MACROS])

# check ibus
PKG_CHECK_MODULES(IBUS, [ibus-1.0 >= 1.3.0])

# check x11
PKG_CHECK_MODULES(X11, [x11])
PKG_CHECK_MODULES(XKBFILE, [xkbfile], [], [
    XKBFILE_LIBS='-lxkbfile'
    AC_SUBST([XKBFILE_LIBS])
])

# OUTPUT files
AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/xkb-layouts.xml.in
                 ])

AC_OUTPUT
