#!/bin/bash

tar -cf .languages.tar .languages
zstd .languages.tar
ld -r -b binary .languages.tar.zst -o src/lang_archive.o 
rm -f .langauges.tar
