## make file for librq.


PROJECT=librq
OBJFILE=$(PROJECT).o
MAINFILE=$(PROJECT).so
LIBVER=1.0.1
LIBFILE=$(MAINFILE).$(LIBVER)
SONAME=$(MAINFILE).1
DESTDIR=
LIBDIR=$(DESTDIR)/usr/lib
INCDIR=$(DESTDIR)/usr/include

ARGS=-g -Wall
OBJS=$(OBJFILE)


all: $(LIBFILE)

$(OBJFILE): librq.c rq.h 
	gcc -c -fPIC librq.c  -o $@ $(ARGS)


librq.a: $(OBJS)
	@>$@
	@rm $@
	ar -r $@
	ar -r $@ $^

$(LIBFILE): $(OBJS)
	gcc -shared -Wl,-soname,$(SONAME) -o $(LIBFILE) $(OBJS)
	

install: $(LIBFILE)
	cp $(LIBFILE) $(LIBDIR)/
	@-test -e $(LIBDIR)/$(MAINFILE) && rm $(LIBDIR)/$(MAINFILE)
	ln -s $(LIBDIR)/$(LIBFILE) $(LIBDIR)/$(MAINFILE)

install_dev: rq.h
	cp rq.h $(INCDIR)


uninstall: /usr/include/rq.h /usr/lib/librq.so.1.0.1
	rm /usr/include/rq.h
	rm /usr/lib/librq.so.1.0.1
	rm /usr/lib/librq.so.1
	rm /usr/lib/librq.so
	

man-pages: manpages/librq.3
	@mkdir tmp.install
	@cp manpages/* tmp.install/
	@gzip tmp.install/*.3
	cp tmp.install/*.3.gz $(MANPATH)/man3/
	@rm -r tmp.install	
	@echo "Man-pages Install complete."


clean:
	@-[ -e librq.o ] && rm librq.o
	@-[ -e librq.so* ] && rm librq.so*
