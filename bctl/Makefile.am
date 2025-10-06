# PgBalancer Control Tool (bctl) Makefile
# REST API client for pgbalancer control

AM_CPPFLAGS = -D_GNU_SOURCE -D_FORTIFY_SOURCE=2 $(shell pkg-config --cflags json-c libcurl openssl)
AM_CFLAGS = -Wall -Wextra -Werror -std=c99 -O2

bin_PROGRAMS = bctl

bctl_SOURCES = bctl.c

bctl_LDFLAGS = $(shell pkg-config --libs json-c libcurl openssl)

bctl_CPPFLAGS = $(AM_CPPFLAGS)

# Check for required libraries
check-deps:
	@which curl-config > /dev/null || (echo "Error: libcurl development files not found" && exit 1)
	@pkg-config --exists json-c || (echo "Error: json-c development files not found" && exit 1)

all: check-deps
