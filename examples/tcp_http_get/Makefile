include ../Makefile.inc

all: webbrowser

webbrowser: webbrowser.c
	cc -g $< -o $@ ${INCS} ${LIBDIR} -Wl,--no-as-needed -Wl,--whole-archive ${NETLIBS} -Wl,--no-whole-archive ${LIBS}

clean:
	rm -f *core webbrowser
