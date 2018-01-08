echo "build 3rd library ..."

export ROOT_DIR=`pwd`
INSTALL_DIR="$ROOT_DIR/thirdparty"
PKG_DIR="thirdparty/packages"

if [ ! -d $PKG_DIR ]; then
  mkdir -p $PKG_DIR
fi

if [ $# -ne 1 ]; then
    echo "build_deps.sh <jansson|libevent|zlog>"
    exit -1
fi

if [ $1 == 'libevent' ]; then

	# build libevent
	#
	echo ">>>>>--------> build libevent ..."

	if [ ! -f $PKG_DIR/libevent-2.0.22-stable.tar.gz ]; then 
		wget https://github.com/libevent/libevent/releases/download/release-2.0.22-stable/libevent-2.0.22-stable.tar.gz --output-document=$PKG_DIR/libevent-2.0.22-stable.tar.gz
	fi
	
	cd $PKG_DIR/
	tar -zxvf libevent-2.0.22-stable.tar.gz

	cd libevent-2.0.22-stable/

	echo "make..."
	./configure --prefix=$INSTALL_DIR --disable-openssl
	make

	echo "install..."
	make install

	echo "clean..."
	cd ../
	rm -rf libevent-2.0.22-stable/

elif [ $1 == 'msgpack' ]; then

	# build libevent
	#
	echo ">>>>>--------> build msgpack ..."

	if [ ! -f $PKG_DIR/msgpack-2.1.1.tar.gz ]; then 
		wget https://github.com/msgpack/msgpack-c/releases/download/cpp-2.1.1/msgpack-2.1.1.tar.gz --output-document=$PKG_DIR/msgpack-2.1.1.tar.gz
	fi
	
	cd $PKG_DIR/
	tar -zxvf msgpack-2.1.1.tar.gz

	cd msgpack-2.1.1

	echo "make..."
	cmake ./ -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DMSGPACK_BUILD_EXAMPLES=OFF -DMSGPACK_ENABLE_CXX=OFF
	make

	echo "install..."
	make install

	echo "clean..."
	cd ../
	rm -rf msgpack-2.1.1/

elif [ $1 == 'jansson' ]; then

	# build jansson
	#
	echo ">>>>>--------> build jansson ..."

	if [ ! -f $PKG_DIR/jansson_v2.9.zip ]; then 
		wget https://github.com/akheron/jansson/archive/v2.9.zip --output-document=$PKG_DIR/jansson_v2.9.zip
	fi
	
	cd $PKG_DIR/
	unzip jansson_v2.9.zip
	cd jansson-2.9/

	echo "make..."
	cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DJANSSON_INSTALL_INCLUDE_DIR=$INSTALL_DIR/include/jansson -DJANSSON_BUILD_DOCS=OFF ./
	make

	echo "install..."
	make install

	echo "clean..."
	cd ../
	rm -rf jansson-2.9/

elif [ $1 == 'zlog' ]; then

	# build zlog
	#
	echo ">>>>>--------> build zlog ..."

	if [ ! -f $PKG_DIR/zlog_1.2.12.tar.gz ]; then 
		wget https://github.com/HardySimpson/zlog/archive/1.2.12.tar.gz --output-document=$PKG_DIR/zlog_1.2.12.tar.gz
	fi

	cd $PKG_DIR/
	tar -zxvf zlog_1.2.12.tar.gz

	cd zlog-1.2.12/

	echo "make..."
	make

	echo "install..."
	make PREFIX=$INSTALL_DIR/ install

	echo "clean..."
	cd ..
	rm -rf zlog-1.2.12/

fi