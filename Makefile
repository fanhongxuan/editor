ROOT_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
PLATFORM := linux
CPUCOUNT := $(shell cat /proc/cpuinfo | grep -c processor)
#CPUCOUNT := 1

PLATFORM := linux
CROSS_COMPILE :=

# note:fanhongxuan@gmail.com
# define the dir which contain the temp file. this dir will be entie removed when make clean
db_inc_dir := import/libdb/include/
db_lib_idr := import/libdb/lib/linux/
db_lib_flags := -lgloutil -lglodb
WXROOT := /home/ubuntu/fanhongxuan/wxWidgets/linux

OUT_DIR := $(ROOT_DIR)/build/$(PLATFORM)/out
INC_DIR := $(ROOT_DIR)/inc
BIN_DIR := $(OUT_DIR)/bin
LIB_DIR := $(OUT_DIR)/lib

CPPFLAGS := -I$(db_inc_dir) -I. -DNDEBUG -D_FILE_OFFSET_BITS=64 -DwxDEBUG_LEVEL=0
CPPFLAGS += -D__WXGTK__
CPPFLAGS += -I$(WXROOT)/lib/wx/include/gtk3-unicode-static-3.1 -I$(WXROOT)/include
CPPFLAGS +=  -fPIC -I$(INC_DIR) -include $(ROOT_DIR)/src/precompile.h

CXXFLAGS += -std=c++11 
LDFLAGS += -L$(WXROOT)/lib/linux/  -no-pie
LDFLAGS += -L$(db_lib_idr) $(db_lib_flags)
LDFLAGS += -L$(WXROOT)/lib
LDFLAGS += -pthread -lexpat   -lwx_gtk3u-3.1 -lwxscintilla-3.1 -lwxtiff-3.1 -lwxjpeg-3.1     -lwxregexu-3.1  -pthread    -lz -ldl -lm  -lexpat -lgtk-3 -lgdk-3 -lpangocairo-1.0 -lpango-1.0 -latk-1.0 -lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lgthread-2.0 -pthread -lglib-2.0 -lX11 -lXxf86vm -lSM -lgtk-3 -lgdk-3 -lpangocairo-1.0 -lpango-1.0 -latk-1.0 -lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0 -lXtst -lpangoft2-1.0 -lpango-1.0 -lgobject-2.0 -lglib-2.0 -lfontconfig -lfreetype -lpng -lz -llzma -lz -ldl -lm

#note:fanhongxuan@gmail.com
#include this at the beginning.
include $(ROOT_DIR)/scripts/config.mk

#if what the makefile show the build progress, include this
include $(EANBLE_COMPILE_PROGRESS)

include $(CLEAR_VARS)
LOCAL_MODULE := lat
LOCAL_PRECOMPILE_HEADER := $(ROOT_DIR)/src/precompile.h
LOCAL_SRC_DIR := src
LOCAL_LD_FLAGS :=
include $(DEFINE_EXECUTE)

#note fanhongxuan@gmail.com
#must include this file at the last.
include $(BUILD_ALL_MODULE)
