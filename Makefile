#!/usr/bin/make -f
# Top-level wrapper Makefile: always build into ./build

.PHONY: all clean rebuild run

all: build-dir
	@# Ensure PARAM.SFO exists before PSPSDK pack step (some build.mak versions don't auto-generate it)
	@if [ ! -f build/PARAM.SFO ]; then \
	  if command -v mksfoex >/dev/null 2>&1; then \
	    echo "Generating build/PARAM.SFO via mksfoex"; \
	    mksfoex "Hello World" build/PARAM.SFO; \
	  elif command -v mksfo >/dev/null 2>&1; then \
	    echo "Generating build/PARAM.SFO via mksfo"; \
	    mksfo "Hello World" build/PARAM.SFO; \
	  else \
	    echo "Warning: mksfoex/mksfo not found; pack-pbp may fail"; \
	  fi; \
	fi
	$(MAKE) -C build -f ../Makefile.base
	@# If the PSPSDK build system emitted files in the project root, move them into build/
	@set -e; \
	for f in EBOOT.PBP PARAM.SFO *.elf *.o *.prx *.PBP; do \
	  if [ -e "$$f" ]; then \
	    echo "Moving $$f -> build/"; \
	    mv -f "$$f" build/; \
	  fi; \
	done || true

build-dir:
	mkdir -p build

clean:
	@if [ -d build ]; then \
		$(MAKE) -C build -f ../Makefile.base clean || true; \
	fi
	rm -rf build

rebuild: clean all

# Convenience target to list outputs (if built)
ls:
	ls -lah build || true
