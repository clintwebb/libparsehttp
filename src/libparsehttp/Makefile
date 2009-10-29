## make file for libparsehttp

MAINFILE=libparsehttp.so
LIBVER=1.0.3
LIBFILE=$(MAINFILE).$(LIBVER)
SONAME=$(MAINFILE).3

MANPATH=/usr/local/man


all: $(LIBFILE)


LIBS=
ARGS=
REQH=



libparsehttp.o: libparsehttp.c parsehttp.h $(REQH)
	gcc -c -fPIC libparsehttp.c -o $@ $(ARGS)

libparsehttp.a: libparsehttp.o
	@>$@
	@rm $@
	ar rcs $@ $^

$(LIBFILE): libparsehttp.o
	gcc -shared -Wl,-soname,$(SONAME) -o $@ $^


lib-test: lib-test.c $(REQH) /usr/include/parsehttp.h
	gcc lib-test.c -o $@ -lparsehttp

install: $(LIBFILE) parsehttp.h 
	cp parsehttp.h /usr/include/
	cp $(LIBFILE) /usr/lib/
	@-test -e /usr/lib/$(MAINFILE) && rm /usr/lib/$(MAINFILE)
	ln -s /usr/lib/$(LIBFILE) /usr/lib/$(MAINFILE)
	nice ldconfig
	@echo "Install complete."

manpages: man/libparsehttp.3 man/ll_free.3 man/ll_get_head.3 man/ll_get_tail.3 man/ll_init.3 man/ll_pop_head.3 man/ll_pop_tail.3 man/ll_push_head.3 man/ll_push_tail.3 man/ll_count.3 man/ll_join_str.3 man/ll_move_head.3 man/ll_move_tail.3
	@mkdir tmp.install
	@cp man/* tmp.install/
	@gzip tmp.install/*.3
	cp tmp.install/*.3.gz $(MANPATH)/man3/
	@rm -r tmp.install


clean:
	@-rm libparsehttp.o
	@-rm libparsehttp.so*

