SHELL := /bin/sh

.DEFAULT_GOAL := build

PYTHON ?= python3
TEST_JOBS ?= 0
TEST_MATCH ?=

AREA_GENERATORS := \
	areas/make_mob \
	areas/make_obj \
	areas/make_qst \
	areas/make_shp \
	areas/make_wld \
	areas/make_zon
AREA_WORLD_OUTPUTS := \
	areas/world.mob \
	areas/world.obj \
	areas/world.qst \
	areas/world.shp \
	areas/world.wld \
	areas/world.zon
AREA_WORLD_DIRECT_INPUTS := \
	areas/AREA \
	areas/RANDOM_AREA \
	areas/lookup.awk \
	areas/lookup.rawk \
	areas/lookup_with_limits.awk \
	areas/m_slow \
	areas/make_all \
	areas/make_lookup \
	areas/moveall \
	$(AREA_GENERATORS)

.PHONY: \
	help all build build-server build-editor build-area-tools world \
	test test-all test-python test-native test-list test-db clean

help:
	@printf '%s\n' \
		'Duris developer targets:' \
		'  make                 Build the server, area editor, and area tools' \
		'  make test            Run deterministic local regression tests' \
		'  make test-all        Build everything, generate world data, and test' \
		'  make test-list       List tests discovered by the regression runner' \
		'  make test-db         Run isolated Docker/MySQL integration tests' \
		'  make clean           Remove compiled build artifacts' \
		'' \
		'Test controls:' \
		'  TEST_JOBS=N          Worker count; 0 selects a bounded automatic value' \
		'  TEST_MATCH=TEXT      Run Python tests whose filename contains TEXT'

all: build

build: build-server build-editor build-area-tools

build-server:
	+$(MAKE) -C src

build-editor:
	+$(MAKE) -C areas/de/src

build-area-tools:
	+$(MAKE) -C areas/src

world: build-area-tools
	@set -eu; \
	stamp=areas/.world.stamp; \
	refresh=0; \
	for output in $(AREA_WORLD_OUTPUTS); do \
		if [ ! -s "$$output" ]; then refresh=1; break; fi; \
	done; \
	if [ "$$refresh" -eq 0 ] && [ ! -f "$$stamp" ]; then refresh=1; fi; \
	if [ "$$refresh" -eq 0 ]; then \
		for input in $(AREA_WORLD_DIRECT_INPUTS); do \
			if [ "$$input" -nt "$$stamp" ]; then refresh=1; break; fi; \
		done; \
	fi; \
	if [ "$$refresh" -eq 0 ]; then \
		newer=$$(find areas/mob areas/obj areas/qst areas/shp areas/wld areas/zon areas/src \
			-type f ! -name '*.o' -newer "$$stamp" -print -quit); \
		if [ -n "$$newer" ]; then refresh=1; fi; \
	fi; \
	if [ "$$refresh" -eq 1 ]; then \
		log=$$(mktemp -t duris-area-build.XXXXXX); \
		trap 'rm -f "$$log"' EXIT HUP INT TERM; \
		echo 'Generating combined world data...'; \
		if ! (cd areas && ./m_slow) >"$$log" 2>&1; then \
			cat "$$log"; \
			exit 1; \
		fi; \
		for output in $(AREA_WORLD_OUTPUTS); do \
			if [ ! -s "$$output" ]; then \
				echo "error: area generation did not produce $$output" >&2; \
				exit 1; \
			fi; \
		done; \
		touch "$$stamp"; \
		echo 'Combined world data is ready.'; \
	else \
		echo 'Combined world data is up to date.'; \
	fi

test-python: world
	$(PYTHON) tests/run_regression_tests.py --jobs "$(TEST_JOBS)" $(if $(strip $(TEST_MATCH)),--match "$(TEST_MATCH)",)

test-native:
	tests/async/run_signal_handlers.sh

test: test-python test-native

# Keep compilation ahead of the test phase even when the caller enables
# parallel make. The recursive make inherits the jobserver and test controls.
test-all: build
	+$(MAKE) test

test-list:
	$(PYTHON) tests/run_regression_tests.py --list $(if $(strip $(TEST_MATCH)),--match "$(TEST_MATCH)",)

# These suites create and destroy their own MySQL containers. They are kept out
# of test-all because Docker is intentionally not a core build dependency.
test-db:
	@command -v docker >/dev/null 2>&1 || { \
		echo 'error: make test-db requires Docker' >&2; \
		exit 127; \
	}
	@docker info >/dev/null 2>&1 || { \
		echo 'error: the Docker daemon is not available' >&2; \
		exit 1; \
	}
	tests/async/run_account_bound_reward_schema_mysql.sh
	tests/async/run_persistence_contract_mysql.sh

clean:
	+$(MAKE) -C src clean
	+$(MAKE) -C areas/de/src clean
	+$(MAKE) -C areas/src clean
