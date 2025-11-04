git clone https://github.com/brgl/libgpiod.git
mkdir -p tmp_install_folder
cd libgpiod
git checkout 50a9f04ee333900962f8632aeae6cdf19cc32454
./autogen.sh --host=$1 --prefix=$(pwd)/../tmp_install_folder/
make
make install
cd ../tmp_install_folder/
cp include/* /usr/include/
cp lib/* /usr/lib/$1/
cd ..
rm -rf libgpiod
rm -rf tmp_install_folder
