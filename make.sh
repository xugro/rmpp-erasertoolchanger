source .env
source $SDK_DIR/environment-setup-cortexa53-crypto-remarkable-linux

python3 $XOVI_HOME/util/xovigen.py --cpp -o build/tmp pen_eraser.xovi
mv build/tmp/xovi.c build/tmp/xovi.cpp


cd build/tmp
qmake6 .
make
