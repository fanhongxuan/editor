CC := g++
wx_inc_dir := import/wxWidgets/include/
wx_lib_dir := import/wxWidgets/lib/linux

db_inc_dir := import/libdb/include/
db_lib_idr := import/libdb/lib/linux/
db_lib_flags := -lgloutil -lglodb
outdir := build/linux/
srcdir := src/
target := $(outdir)ce

CFLAGS := \
	-I$(wx_inc_dir) \
	-I$(wx_lib_dir)/wx/include/gtk2-unicode-static-3.1/ \
    -I$(db_inc_dir) \
	-I. -D__WXGTK__ -DNDEBUG -D_FILE_OFFSET_BITS=64 -DwxDEBUG_LEVEL=0

LDFLAGS := \
	-L$(wx_lib_dir) \
    -L$(db_lib_idr) \
    $(db_lib_flags) \
	-pthread \
	-lexpat \
	-lwx_gtk2u-3.1 \
	-lwxscintilla-3.1 \
	-lwxtiff-3.1 \
	-lwxregexu-3.1 \
	-lz -ldl -lm \
	-lgtk-x11-2.0 \
	-lgdk-x11-2.0 \
	-latk-1.0 \
	-lgio-2.0 \
	-lpangoft2-1.0 \
	-lpangocairo-1.0 \
	-lgdk_pixbuf-2.0 \
	-lcairo \
	-lpango-1.0 \
	-lfreetype \
	-lfontconfig \
	-lgobject-2.0 \
	-lgthread-2.0 \
	-lrt \
	-lglib-2.0 \
	-lX11 \
	-lSM \
	-lgtk-x11-2.0 \
	-lgdk-x11-2.0 \
	-latk-1.0 \
	-lgio-2.0 \
	-lpangoft2-1.0 \
	-lpangocairo-1.0 \
	-lgdk_pixbuf-2.0 \
	-lpng

SRCS := $(wildcard $(srcdir)*.cpp)
DIRS := $(notdir $(SRCS))
DEPS := $(patsubst %.cpp, $(outdir)%.d, $(DIRS))
objects := $(patsubst %.cpp, $(outdir)%.o, $(DIRS))

all: $(outdir) $(target)

-include $(DEPS)

$(outdir):
	@mkdir -p $(outdir)

$(target): $(objects)
	$(CC) -o $(target) $(objects) $(LDFLAGS)

$(DEPS): $(outdir)%.d:$(srcdir)%.cpp
	$(CC) $(CFLAGS) -MF"$@" -MG -MM -MP -MT"$(@:.d=.o)" $<

$(objects): $(outdir)%.o:$(srcdir)%.cpp
	$(CC) -c $(CFLAGS) $< -o $@


.PHONY:clean
clean:
	-rm -rf $(objects) $(target) $(DEPS)
