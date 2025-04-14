prefix=/usr/local
exec_prefix=$(prefix)
bindir=$(exec_prefix)/bin

.PHONY: install
install: install-bin

install-bin:
	cp jpw $(bindir)