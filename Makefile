CC = gcc
LDFLAGS =  -libverbs -lpthread -lrdmacm
CFLAGS += -Wall -I./include 

PREFIX = .
BINDIR = $(PREFIX)/bin
BENCHDIR = $(BINDIR)/bench

LIBPATH = $(PREFIX)/lib
SRCPATH = $(PREFIX)/src
TESTPATH = $(PREFIX)/tests

HEADERS = $(shell echo $(PREFIX)/include/conn/*.h)

SRC_BENCH = $(BENCHDIR)/sendrcv_client
SRC_BENCH_SRCS=  $(TESTPATH)/benchmark/sendrcv_client.c $(SRCPATH)/conn/sendrcv.c
SRC_BENCH_OBJS= $(SRC_BENCH_SRCS:.c=.o)

SRS_BENCH = $(BENCHDIR)/sendrcv_server
SRS_BENCH_SRCS=  $(TESTPATH)/benchmark/sendrcv_server.c $(SRCPATH)/conn/sendrcv.c
SRS_BENCH_OBJS= $(SRS_BENCH_SRCS:.c=.o)

SSRC_BENCH = $(BENCHDIR)/sendsrcv_client
SSRC_BENCH_SRCS=  $(TESTPATH)/benchmark/sendsrcv_client.c $(SRCPATH)/conn/sendsrcv.c
SSRC_BENCH_OBJS= $(SSRC_BENCH_SRCS:.c=.o)


SSRS_BENCH = $(BENCHDIR)/sendsrcv_server
SSRS_BENCH_SRCS=  $(TESTPATH)/benchmark/sendsrcv_server.c $(SRCPATH)/conn/sendsrcv.c
SSRS_BENCH_OBJS= $(SSRS_BENCH_SRCS:.c=.o)

bench: sr_bench ssr_bench
sr_bench: $(SRS_BENCH_OBJS) $(SRC_BENCH_OBJS) $(HEADERS)
	mkdir -pm 755 $(BENCHDIR)
	$(CC) $(CFLAGS) -o $(SRC_BENCH) $(SRC_BENCH_OBJS) $(LDFLAGS)
	$(CC) $(CFLAGS) -o $(SRS_BENCH) $(SRS_BENCH_OBJS) $(LDFLAGS)
	@echo "##############################"
	@echo
ssr_bench: $(SSRS_BENCH_OBJS) $(SSRC_BENCH_OBJS) $(HEADERS)
	mkdir -pm 755 $(BENCHDIR)
	$(CC) $(CFLAGS) -o $(SSRC_BENCH) $(SSRC_BENCH_OBJS) $(LDFLAGS)
	$(CC) $(CFLAGS) -o $(SSRS_BENCH) $(SSRS_BENCH_OBJS) $(LDFLAGS)
	@echo "##############################"
	@echo

debug: CFLAGS += -DDEBUG -g -O0
debug: ${APPS}
clean:
	@echo "##### CLEAN-UP #####"
	-rm -f $(SRC_BENCH_OBJS)
	-rm -f $(SRC_BENCH)
	-rm -f $(SRS_BENCH_OBJS)
	-rm -f $(SRS_BENCH)
	@echo "####################"
	@echo

%.o: %.c $(HEADERS)
	$(CC) $(FLAGS) $(CFLAGS) -c -o $@ $<

.DELETE_ON_ERROR:
.PHONY: all clean


