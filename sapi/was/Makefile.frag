was: $(SAPI_WAS_PATH)

$(SAPI_WAS_PATH): $(PHP_GLOBAL_OBJS) $(PHP_BINARY_OBJS) $(PHP_WAS_OBJS)
	$(BUILD_WAS)

install-was: $(SAPI_WAS_PATH)
	@echo "Installing PHP WAS binary:  $(INSTALL_ROOT)$(bindir)/"
	@$(mkinstalldirs) $(INSTALL_ROOT)$(bindir)
	@$(INSTALL) -m 0755 $(SAPI_WAS_PATH) $(INSTALL_ROOT)$(bindir)/$(program_prefix)php-was$(program_suffix)
