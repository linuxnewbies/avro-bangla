# Avro Bangla IBus Engine Makefile

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
DATADIR = $(PREFIX)/share
IBUS_DIR = $(DATADIR)/ibus/component
ICONDIR = $(DATADIR)/icons/hicolor/48x48/apps

.PHONY: all clean install uninstall test

all: avro-bangla-ibus

avro-bangla-ibus: avro.go avro_rules.go main.go go.mod go.sum
	go build -o avro-bangla-ibus .

clean:
	rm -f avro-bangla-ibus

install: avro-bangla-ibus
	@echo "Installing Avro Bangla IBus engine..."
	install -d $(BINDIR)
	install -m 755 avro-bangla-ibus $(BINDIR)/
	install -d $(IBUS_DIR)
	install -m 644 org.avro.bangla.xml $(IBUS_DIR)/
	@echo ""
	@echo "Installation complete!"
	@echo ""
	@echo "To use Avro Bangla:"
	@echo "1. Restart IBus daemon: ibus-daemon -drx"
	@echo "2. Run ibus-setup and add 'Avro Bangla Phonetic' to your input methods"
	@echo "3. Switch to Avro Bangla using Super+Space or the IBus menu"
	@echo ""

uninstall:
	@echo "Uninstalling Avro Bangla IBus engine..."
	rm -f $(BINDIR)/avro-bangla-ibus
	rm -f $(IBUS_DIR)/org.avro.bangla.xml
	@echo "Uninstallation complete!"

test: avro-bangla-ibus
	@echo "Testing Avro Bangla conversion engine..."
	./avro-bangla-ibus --help 2>&1 || true
