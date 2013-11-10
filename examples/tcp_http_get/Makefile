include ../Makefile.inc

all: webbrowser

webbrowser: webbrowser.c
	cc -g $< -o $@ ${INCS} ${LIBDIR} -Wl,--no-as-needed ${LIBS}

clean:
	rm -f *core webbrowser
