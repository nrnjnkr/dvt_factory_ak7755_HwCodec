# Include Ambarella's default top level makefile
include Makefile

# Prepend ccache to path so it is found first
export PATH:=/usr/lib/ccache:$(PATH)

# Add 'prebuild' target for everthing built before final image and amboot
prebuild: $(ALL_TARGETS)
	@echo "Prebuild Done"

