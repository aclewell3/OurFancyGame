if [ ! -d "build" ]; then
 mkdir build
fi

if [ "$1" = "-c" ]; then
  cd build
  rm -rf *
  cmake ../src
  make
elif [ "$1" = "-v" ]; then
  cd build
  cmake ../src -DVERBOSE=ON
  make
elif [ "$1" = "-g" ]; then
  cd build
  cmake ../src -DCMAKE_BUILD_TYPE=Debug
  make
else
  cd build
  cmake ../src -DVERBOSE=OFF
  make
fi
