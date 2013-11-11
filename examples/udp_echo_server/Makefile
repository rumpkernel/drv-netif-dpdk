include ../Makefile.inc

all: udp_echo_server

udp_echo_server: udp_echo_server.c
	cc -g $< -o $@ ${INCS} ${LIBDIR} -Wl,--no-as-needed -Wl,--whole-archive ${NETLIBS} -Wl,--no-whole-archive ${LIBS}

clean:
	rm -f udp_echo_server *core
