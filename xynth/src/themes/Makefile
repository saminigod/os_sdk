
include ../../Makefile.cfg
include ../../Makefile.parse

DIR = src/themes/

THEME_KERAMIK = Y
THEME_MICROGUI = Y
THEME_SILVERADO = Y

#subdirs-$(THEME_KERAMIK)   += keramik
#subdirs-$(THEME_MICROGUI)  += microgui
subdirs-$(THEME_SILVERADO) += silverado

all:
	@$(foreach F, $(subdirs-Y), $(MAKE) -C $F || exit 1;)

dist: all
	@$(foreach F, $(subdirs-Y), $(MAKE) dist -C $F || exit 1;)

clean:
	@$(foreach F, $(subdirs-Y), $(MAKE) clean -C $F || exit 1;)
	@$(foreach F, $(subdirs-N), $(MAKE) clean -C $F || exit 1;)
	@$(foreach F, $(subdirs-), $(MAKE) clean -C $F || exit 1;)
