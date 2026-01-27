NVMLLIB = /opt/cuda/lib64
NVMLLDFLAGS += -lnvidia-ml
CFLAGS += -L$(NVMLLIB) $(NVMLLDFLAGS)

CFLAGS += -O2 -march=native -fanalyzer -Wno-unknown-argument -Wpedantic -pedantic -Wall -Wextra -Wuninitialized -Wshadow -Warray-bounds -Wnull-dereference -Wformat -Wunused -Wwrite-strings
PROG = nvspeed
PREFIX = /usr/local

all: $(PROG)

nvspeed: config.h
	$(CC) -o $@ $(PROG).c $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)

config.h:
	cp config.def.h $@

config: config.h

clean:
	rm -f $(PROG)

install: $(PROG)
	strip $(PROG)
	chmod 755 $^
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	command -v rsync >/dev/null && rsync -a -r -c $^ $(DESTDIR)$(PREFIX)/bin || cp -f $^ $(DESTDIR)$(PREFIX)/bin

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(PROG)
