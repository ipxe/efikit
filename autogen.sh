#!/bin/sh

mkdir -p build-aux
touch build-aux/config.rpath
autoreconf --install
