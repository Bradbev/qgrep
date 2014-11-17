# Need -lpthread & -lz as part of the system
if [ $OSTYPE == "msys" ]; then
	mingw-get install libz libpthread
fi	

cd packages

hg clone https://re2.googlecode.com/hg re2
cd re2
if [ $OSTYPE == "msys" ]; then
patch -p 1 -i ../reqired_patches/re2.patch
fi	
make obj/libre2.a
cd ..

curl -R -O http://www.libarchive.org/downloads/libarchive-3.1.2.tar.gz
tar zxf libarchive-3.1.2.tar.gz
rm -f libarchive-3.1.2.tar.gz
cd libarchive-3.1.2
./configure --without-bz2lib --without-lzmadec --without-iconv --without-lzma --without-lzo2 --without-nettle --without-openssl --without-xml2 --without-expat
make 
cd ..

curl -R -O http://www.lua.org/ftp/lua-5.1.5.tar.gz
tar zxf lua-5.1.5.tar.gz
rm lua-5.1.5.tar.gz
cd lua-5.1.5
make generic test
cd ..

cd ..

