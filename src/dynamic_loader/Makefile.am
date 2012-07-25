omxdynamicloaderdir = $(libdir)/omxloaders

omxdynamicloader_LTLIBRARIES = libomxdynamicloader.la
libomxdynamicloader_la_SOURCES = samsung_dynamic_component_loader.c samsung_dynamic_component_loader.h

libomxdynamicloader_la_LDFLAGS =
libomxdynamicloader_la_CFLAGS = -I$(top_srcdir)/include \
			-I$(top_srcdir)/src \
			-I$(top_srcdir)/src/base \
			-DOMX_COMPONENT_PATH=\"$(plugindir)\/\"
