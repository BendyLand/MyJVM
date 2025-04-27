#!/bin/bash

tar -cf .languages.tar .languages
zstd .languages.tar
xxd -i .languages.tar.zst > lang_archive.h
rm -f .languages.tar*
mv lang_archive.h src

