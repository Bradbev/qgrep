#!/bin/sh
cd dist/linux
strip qgrep
tar zcf qgrep-linux.tgz --exclude=.gitkeep *
mv qgrep-linux.tgz ../../installer/linux

