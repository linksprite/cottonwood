set -e
rm -f build.log
#svn revert as399x_config.h
hg revert as399x_config.h
VERSION='v'`grep -e 'Reader Firmware .\..\..' as399x_config.h | grep -oe '.\..\..' | sed -e 's/\./-/g'`
echo "Creating Delivery $VERSION"
cp as399x_config.h as399x_config.orig

echo "--- LEO AS3992 ---"
rm as399x_config.h
sed -e 's/RUN_ON_AS3992..*/RUN_ON_AS3992 1/' as399x_config.orig > as399x_config.h
grep COMMUNICATION as399x_config.h
grep RUN_ON as399x_config.h
make clean ; make > build.log
cp objects/AS399XDEMO.bin LEO_AS3992_$VERSION.bin

echo "--- LEO AS3991 ---"
rm as399x_config.h
sed -e 's/#define *LEO.*/#define LEO 1/;s/#define ROGER.*/#define ROGER 0/;s/RUN_ON_AS3992..*/RUN_ON_AS3992 0/' as399x_config.orig > as399x_config.h
grep COMMUNICATION as399x_config.h
grep RUN_ON as399x_config.h
make clean ; make > build.log
cp objects/AS399XDEMO.bin LEO_AS3991_$VERSION.bin

echo "--- ROGER AS3992 ---"
rm as399x_config.h
sed -e 's/#define *LEO.*/#define LEO 0/;s/#define ROGER.*/#define ROGER 1/;s/RUN_ON_AS3992..*/RUN_ON_AS3992 1/' as399x_config.orig > as399x_config.h
grep COMMUNICATION as399x_config.h
grep RUN_ON as399x_config.h
make clean ; make > build.log
cp objects/AS399XDEMO.bin ROGER_AS3992_$VERSION.bin

echo "--- ROGER AS3991 ---"
rm as399x_config.h
sed -e 's/#define *LEO.*/#define LEO 0/;s/#define ROGER.*/#define ROGER 1/;s/RUN_ON_AS3992..*/RUN_ON_AS3992 0/' as399x_config.orig > as399x_config.h
grep COMMUNICATION as399x_config.h
grep RUN_ON as399x_config.h
make clean ; make > build.log
cp objects/AS399XDEMO.bin ROGER_AS3991_$VERSION.bin

echo "--- LEO AS3992 UART ---"
rm as399x_config.h
sed -e 's/UARTSUPPORT.*/UARTSUPPORT 1/;s/RUN_ON_AS3992..*/RUN_ON_AS3992 1/' as399x_config.orig > as399x_config.h
grep COMMUNICATION as399x_config.h
grep RUN_ON as399x_config.h
make clean ; make > build.log
cp objects/AS399XDEMO.bin LEO_AS3992_uart_$VERSION.bin

echo "--- LEO AS3991 UART ---"
rm as399x_config.h
sed -e 's/UARTSUPPORT.*/UARTSUPPORT 1/;s/#define *LEO.*/#define LEO 1/;s/#define ROGER.*/#define ROGER 0/;s/RUN_ON_AS3992..*/RUN_ON_AS3992 0/' as399x_config.orig > as399x_config.h
grep COMMUNICATION as399x_config.h
grep RUN_ON as399x_config.h
make clean ; make > build.log
cp objects/AS399XDEMO.bin LEO_AS3991_uart_$VERSION.bin

echo "--- ROGER AS3992 UART ---"
rm as399x_config.h
sed -e 's/UARTSUPPORT.*/UARTSUPPORT 1/;s/#define *LEO.*/#define LEO 0/;s/#define ROGER.*/#define ROGER 1/;s/RUN_ON_AS3992..*/RUN_ON_AS3992 1/' as399x_config.orig > as399x_config.h
grep COMMUNICATION as399x_config.h
grep RUN_ON as399x_config.h
make clean ; make > build.log
cp objects/AS399XDEMO.bin ROGER_AS3992_uart_$VERSION.bin

echo "--- ROGER AS3991 UART ---"
rm as399x_config.h
sed -e 's/UARTSUPPORT.*/UARTSUPPORT 1/;s/#define *LEO.*/#define LEO 0/;s/#define ROGER.*/#define ROGER 1/;s/RUN_ON_AS3992..*/RUN_ON_AS3992 0/' as399x_config.orig > as399x_config.h
grep COMMUNICATION as399x_config.h
grep RUN_ON as399x_config.h
make clean ; make > build.log
cp objects/AS399XDEMO.bin ROGER_AS3991_uart_$VERSION.bin

echo "--- LEO AS3991 SPI ---"
rm as399x_config.h
sed -e 's/COMMUNICATION_SERIAL.*/COMMUNICATION_SERIAL 1/;s/RUN_ON_AS3992..*/RUN_ON_AS3992 0/' as399x_config.orig > as399x_config.h
grep COMMUNICATION as399x_config.h
grep RUN_ON as399x_config.h
make clean ; make > build.log
cp objects/AS399XDEMO.bin LEO_AS3991_spi_$VERSION.bin

echo "--- LEO AS3992 SPI ---"
rm as399x_config.h
sed -e 's/COMMUNICATION_SERIAL.*/COMMUNICATION_SERIAL 1/;s/RUN_ON_AS3992..*/RUN_ON_AS3992 1/' as399x_config.orig > as399x_config.h
grep COMMUNICATION as399x_config.h
grep RUN_ON as399x_config.h
make clean ; make > build.log
cp objects/AS399XDEMO.bin LEO_AS3992_spi_$VERSION.bin

echo "--- PICO AS3991 SPI ---"
rm as399x_config.h
sed -e 's/#define *LEO.*/#define LEO 0/;s/#define PICO.*/#define PICO 1/;s/COMMUNICATION_SERIAL.*/COMMUNICATION_SERIAL 1/;s/RUN_ON_AS3992..*/RUN_ON_AS3992 0/' as399x_config.orig > as399x_config.h
grep COMMUNICATION as399x_config.h
grep RUN_ON as399x_config.h
make clean ; make > build.log
cp objects/AS399XDEMO.bin PICO_AS3991_spi_$VERSION.bin

echo "--- PICO AS3992 SPI ---"
rm as399x_config.h
sed -e 's/#define *LEO.*/#define LEO 0/;s/#define PICO.*/#define PICO 1/;s/COMMUNICATION_SERIAL.*/COMMUNICATION_SERIAL 1/;s/RUN_ON_AS3992..*/RUN_ON_AS3992 1/' as399x_config.orig > as399x_config.h
grep COMMUNICATION as399x_config.h
grep RUN_ON as399x_config.h
make clean ; make > build.log
cp objects/AS399XDEMO.bin PICO_AS3992_spi_$VERSION.bin

echo "--- MICROREADER AS3991 ---"
rm as399x_config.h
sed -e 's/#define *LEO.*/#define LEO 0/;s/#define MICROREADER.*/#define MICROREADER 1/;s/RUN_ON_AS3992..*/RUN_ON_AS3992 0/' as399x_config.orig > as399x_config.h
grep COMMUNICATION as399x_config.h
grep RUN_ON as399x_config.h
make clean ; make > build.log
cp objects/AS399XDEMO.bin MICROREADER_AS3991_$VERSION.bin

echo "--- MICROREADER AS3991 SPI ---"
rm as399x_config.h
sed -e 's/#define *LEO.*/#define LEO 0/;s/#define MICROREADER.*/#define MICROREADER 1/;s/RUN_ON_AS3992..*/RUN_ON_AS3992 0/;s/COMMUNICATION_SERIAL.*/COMMUNICATION_SERIAL 1/' as399x_config.orig > as399x_config.h
grep COMMUNICATION as399x_config.h
grep RUN_ON as399x_config.h
make clean ; make > build.log
cp objects/AS399XDEMO.bin MICROREADER_AS3991_spi_$VERSION.bin

echo "--- MICROREADER AS3992 ---"
rm as399x_config.h
sed -e 's/#define *LEO.*/#define LEO 0/;s/#define MICROREADER.*/#define MICROREADER 1/;s/RUN_ON_AS3992..*/RUN_ON_AS3992 1/' as399x_config.orig > as399x_config.h
grep COMMUNICATION as399x_config.h
grep RUN_ON as399x_config.h
make clean ; make > build.log
cp objects/AS399XDEMO.bin MICROREADER_AS3992_$VERSION.bin

echo "--- MICROREADER AS3992 SPI ---"
rm as399x_config.h
sed -e 's/#define *LEO.*/#define LEO 0/;s/#define MICROREADER.*/#define MICROREADER 1/;s/RUN_ON_AS3992..*/RUN_ON_AS3992 1/;s/COMMUNICATION_SERIAL.*/COMMUNICATION_SERIAL 1/' as399x_config.orig > as399x_config.h
grep COMMUNICATION as399x_config.h
grep RUN_ON as399x_config.h
make clean ; make > build.log
cp objects/AS399XDEMO.bin MICROREADER_AS3992_spi_$VERSION.bin

# uart versions of micro reader
echo "--- MICROREADER AS3991 UART ---"
rm as399x_config.h
sed -e 's/UARTSUPPORT.*/UARTSUPPORT 1/;s/#define *LEO.*/#define LEO 0/;s/#define MICROREADER.*/#define MICROREADER 1/;s/RUN_ON_AS3992..*/RUN_ON_AS3992 0/' as399x_config.orig > as399x_config.h
grep COMMUNICATION as399x_config.h
grep RUN_ON as399x_config.h
make clean ; make > build.log
cp objects/AS399XDEMO.bin MICROREADER_AS3991_uart_$VERSION.bin

echo "--- MICROREADER AS3991 SPI UART ---"
rm as399x_config.h
sed -e 's/UARTSUPPORT.*/UARTSUPPORT 1/;s/#define *LEO.*/#define LEO 0/;s/#define MICROREADER.*/#define MICROREADER 1/;s/RUN_ON_AS3992..*/RUN_ON_AS3992 0/;s/COMMUNICATION_SERIAL.*/COMMUNICATION_SERIAL 1/' as399x_config.orig > as399x_config.h
grep COMMUNICATION as399x_config.h
grep RUN_ON as399x_config.h
make clean ; make > build.log
cp objects/AS399XDEMO.bin MICROREADER_AS3991_spi_uart_$VERSION.bin

echo "--- MICROREADER AS3992 UART ---"
rm as399x_config.h
sed -e 's/UARTSUPPORT.*/UARTSUPPORT 1/;s/#define *LEO.*/#define LEO 0/;s/#define MICROREADER.*/#define MICROREADER 1/;s/RUN_ON_AS3992..*/RUN_ON_AS3992 1/' as399x_config.orig > as399x_config.h
grep COMMUNICATION as399x_config.h
grep RUN_ON as399x_config.h
make clean ; make > build.log
cp objects/AS399XDEMO.bin MICROREADER_AS3992_uart_$VERSION.bin

echo "--- MICROREADER AS3992 SPI UART ---"
rm as399x_config.h
sed -e 's/UARTSUPPORT.*/UARTSUPPORT 1/;s/#define *LEO.*/#define LEO 0/;s/#define MICROREADER.*/#define MICROREADER 1/;s/RUN_ON_AS3992..*/RUN_ON_AS3992 1/;s/COMMUNICATION_SERIAL.*/COMMUNICATION_SERIAL 1/' as399x_config.orig > as399x_config.h
grep COMMUNICATION as399x_config.h
grep RUN_ON as399x_config.h
make clean ; make > build.log
cp objects/AS399XDEMO.bin MICROREADER_AS3992_spi_uart_$VERSION.bin

echo "--- ROLAND AS3992 ---"
rm as399x_config.h
sed -e 's/#define *LEO.*/#define LEO 0/;s/#define ROLAND.*/#define ROLAND 1/;s/RUN_ON_AS3992..*/RUN_ON_AS3992 1/;s/COMMUNICATION_SERIAL.*/COMMUNICATION_SERIAL 1/;s/CRC16_ENABLE.*/CRC16_ENABLE 0/;s/ISO6B.*/ISO6B 0/' as399x_config.orig > as399x_config.h
grep COMMUNICATION as399x_config.h
grep RUN_ON as399x_config.h
make clean ; make > build.log
cp objects/AS399XDEMO.bin ROLAND_AS3992_$VERSION.bin

echo "--- ARNIE AS3992 ---"
rm as399x_config.h
sed -e 's/#define *LEO.*/#define LEO 0/;s/#define ARNIE.*/#define ARNIE 1/;s/RUN_ON_AS3992..*/RUN_ON_AS3992 1/;s/COMMUNICATION_SERIAL.*/COMMUNICATION_SERIAL 1/' as399x_config.orig > as399x_config.h
grep COMMUNICATION as399x_config.h
grep RUN_ON as399x_config.h
make clean ; make > build.log
cp objects/AS399XDEMO.bin ARNIE_AS3992_$VERSION.bin

echo "--- ARNIE AS3992 ---"
rm as399x_config.h
sed -e 's/UARTSUPPORT.*/UARTSUPPORT 1/;s/#define *LEO.*/#define LEO 0/;s/#define ARNIE.*/#define ARNIE 1/;s/RUN_ON_AS3992..*/RUN_ON_AS3992 1/;s/COMMUNICATION_SERIAL.*/COMMUNICATION_SERIAL 1/' as399x_config.orig > as399x_config.h
grep COMMUNICATION as399x_config.h
grep RUN_ON as399x_config.h
make clean ; make > build.log
cp objects/AS399XDEMO.bin ARNIE_AS3992_uart_$VERSION.bin

#end
cp as399x_config.orig as399x_config.h
