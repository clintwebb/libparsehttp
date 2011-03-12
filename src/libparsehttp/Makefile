## make file for libparsehttp

PROJECT=libparsehttp
OBJFILE=$(PROJECT).o
MAINFILE=$(PROJECT).so
LIBVER=1.0.1
LIBFILE=$(MAINFILE).$(LIBVER)
SONAME=$(MAINFILE).1

DESTDIR=
INCDIR=$(DESTDIR)/usr/include
LIBDIR=$(DESTDIR)/usr/lib
MANDIR=$(DESTDIR)/usr/share/man


all: $(LIBFILE)


LIBS=
ARGS=



$(OBJFILE): libparsehttp.c parsehttp.h
	gcc -c -fPIC libparsehttp.c -o $@ $(ARGS)

$(PROJECT).a: $(OBJFILE)
	@>$@
	@rm $@
	ar rcs $@ $^

$(LIBFILE): $(OBJFILE)
	gcc -shared -Wl,-soname,$(SONAME) -o $@ $^


install_dev: parsehttp.h 
	cp parsehttp.h $(INCDIR)/


install: $(LIBFILE)
	cp $(LIBFILE) $(LIBDIR)/
	@-test -e $(LIBDIR)/$(MAINFILE) && rm $(LIBDIR)/$(MAINFILE)
	ln -s $(LIBDIR)/$(LIBFILE) $(LIBDIR)/$(MAINFILE)


manpages: man/libparsehttp.3 
	@mkdir tmp.install
	@cp man/* tmp.install/
	@gzip tmp.install/*.3
	cp tmp.install/*.3.gz $(MANDIR)/man3/
	@rm -r tmp.install


clean:
	@-rm $(OBJFILE)
	@-rm libparsehttp.so*

