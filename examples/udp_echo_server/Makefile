include ../Makefile.inc

all: udp_echo_server

udp_echo_server: udp_echo_server.c
	cc -g $< -o $@ ${INCS} ${LIBDIR} -Wl,--no-as-needed -Wl,--whole-archive ${LIBS} -Wl,--no-whole-archive

clean:
	rm -f udp_echo_server *core
