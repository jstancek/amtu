
CONFIG_CLEAN_FILES = *.rej *.orig
EXTRA_DIST = amtu.init amtu.sysconfig
initdir=$(sysconfdir)/rc.d/init.d
sysconfigdir=$(sysconfdir)/sysconfig

install-data-hook:
	$(INSTALL_DATA) -D -m 640 ${srcdir}/amtu.sysconfig ${DESTDIR}${sysconfigdir}/amtu

install-exec-hook:
	$(INSTALL_SCRIPT) -D -m 755 ${srcdir}/amtu.init ${DESTDIR}${initdir}/amtu

uninstall-hook:
	rm ${DESTDIR}${sysconfigdir}/amtu
	rm ${DESTDIR}${initdir}/amtu

