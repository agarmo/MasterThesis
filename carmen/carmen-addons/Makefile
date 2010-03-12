include Makefile.conf

PACKAGES = errorgraph gps xavier canon 

RECURSE = $(SILENT) \
	for i in $(PACKAGES) xxxx ; do \
		if [ -d $$i ] ; then \
			if ! $(MAKE) -C $$i $@ ; then \
                                exit -1; \
                        fi; \
		fi \
	done

all:
	$(RECURSE)

clean:
	$(ECHO)
	$(ECHO) "       *********"
	$(ECHO) "       C L E A N"
	$(ECHO) "       *********"
	$(ECHO)
	$(RECURSE)
	$(SILENT) $(RM) *~ .\#* \#* core

distclean:
	$(ECHO)
	$(ECHO) "       *****************"
	$(ECHO) "       D I S T C L E A N"
	$(ECHO) "       *****************"
	$(ECHO)
	$(RECURSE)
	$(SILENT) $(RM) *~ .\#* \#* core Makefile.vars