NAME      := lbc
VERSION   := 2.0d
LIBS      := -ljpeg -lm ${EXTRA_LIBS} 
KLIB      := klib
KLIB_INC  := $(KLIB)/include
KLIB_LIB  := $(KLIB)
TARGET	  := $(NAME)
SOURCES   := $(shell find src/ -type f -name *.c)
OBJECTS   := $(patsubst src/%,build/%,$(SOURCES:.c=.o))
DEPS	  := $(OBJECTS:.o=.deps)
DESTDIR   := /
PREFIX    := /usr
BINDIR    := $(DESTDIR)/$(PREFIX)/bin
MANDIR    := $(DESTDIR)/$(PREFIX)/share/man/man1/
EXTRA_CFLAGS ?=
EXTRA_LDFLAGS ?=
CFLAGS    := -g -O0 -Wall -Wno-unused-value -DNAME=\"$(NAME)\" -DVERSION=\"$(VERSION)\" -DPREFIX=\"$(PREFIX)\" -I $(KLIB_INC) ${EXTRA_CFLAGS} -ffunction-sections -fdata-sections

LDFLAGS := -s -Wl,--gc-sections ${EXTRA_LDFLAGS}

$(TARGET): $(OBJECTS) 
	echo $(SOURCES)
	make -C klib
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJECTS) $(KLIB)/klib.a $(LIBS) 

build/%.o: src/%.c
	@mkdir -p build/
	$(CC) $(CFLAGS) -MD -MF $(@:.o=.deps) -c -o $@ $<

clean:
	$(RM) -r build/ $(TARGET) 
	gmake -C klib clean

install: $(TARGET)
	mkdir -p $(BINDIR) 
	strip $(TARGET)
	install -m 755 $(TARGET) ${BINDIR}
	install -m 644 man1/* $(MANDIR)

-include $(DEPS)

.PHONY: clean

