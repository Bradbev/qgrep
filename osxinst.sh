#!/bin/sh
cd dist/osx
strip qgrep
tar zcf qgrep-osx.tgz *
mv qgrep-osx.tgz ../../installer/osx
