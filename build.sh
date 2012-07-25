#!/bin/sh

TOOLCHAIN_PATH="/usr/arm-linux-gnueabi"
OMX_INSTALL_PATH="/home/kamil/praca/openmax/sprc/dst7"
OMXCORE_INCLUDE_PATH=$OMX_INSTALL_PATH"/usr/local/include"


cflags="-I$OMX_INSTALL_PATH/include -I$TOOLCHAIN_PATH/include \
	-I$OMXCORE_INCLUDE_PATH \
	-L$OMX_INSTALL_PATH/usr/local/lib"

make distclean

autoreconf -i

ac_cv_func_memset_0_nonnull=yes ac_cv_func_realloc_0_nonnull=yes  ac_cv_func_malloc_0_nonnull=yes ./configure \
 --host=arm-linux-gnueabi \
 --prefix=/usr/local \
 --enable-shared \
 CFLAGS="$cflags"


mkdir -p $OMX_INSTALL_PATH

make uninstall DESTDIR=$OMX_INSTALL_PATH
make
make install DESTDIR=$OMX_INSTALL_PATH
