include ../Makefile.inc

all: webbrowser

webbrowser: webbrowser.c
	cc -g $< -o $@ ${INCS} ${LIBDIR} -Wl,--no-as-needed -Wl,--whole-archive ${LIBS} -Wl,--no-whole-archive

clean:
	rm -f *core webbrowser
