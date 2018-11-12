CC := g++
wx_inc_dir := import/wxWidgets/include/
wx_lib_dir := import/wxWidgets/lib/linux

CFLAGS := -I$(wx_inc_dir) -I$(wx_lib_dir)/include/ -I. -D__WXGTK__

LDFLAGS := -L$(wx_lib_dir) -pthread \
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


outdir := build/linux/
target := $(outdir)ce

objects = \
	$(outdir)ce.o \
	$(outdir)wxSearch.o \
	$(outdir)wxEdit.o \
	$(outdir)wxAutoComp.o \
	$(outdir)wxBufferSelect.o \
	$(outdir)wxDockArt.o \
	$(outdir)wxPrefs.o

all: $(outdir) $(target)

$(outdir):
	@mkdir -p $(outdir)

$(target): $(objects)
	$(CC) -o $(target) $(objects) $(LDFLAGS)

%.d: %.cpp
	@set -e; rm -f $@; \
	$(CC) -M $(CPPFLAGS) $< > $@.; \
	sed 's,\.o[ :]*,\1.o $@ : ,g' < $@. > $@; \
        rm -f $@.

$(objects): $(outdir)%.o:src/%.cpp
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY:clean
clean:
	-rm -rf $(objects) $(target)
