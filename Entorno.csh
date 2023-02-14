#!/bin/bash
echo "  > Clonando archivos fuente del sismografo"
git clone https://github.com/jhon-stack/lab-4-microcontroladores-.git
echo "  > Dirigiendose al directorio clonado"
cd lab-4-microcontroladores-

echo "Configurando uso de biblioteca:"
echo "  > Clonando la biblioteca"
git clone https://github.com/libopencm3/libopencm3.git
echo "  > Clonando la biblioteca de los ejemplos"
git clone https://github.com/libopencm3/libopencm3-examples.git
echo "  > copiando archivos a la biblioteca de los ejemplos"
cp -rf libopencm3 libopencm3-examples/
echo "  > Borrando biblioteca"
rm -rf libopencm3

echo "  > Reemplazando Makefiles por propios"
cp -f src/Makes/Makefile libopencm3-examples/
cp -f src/Makes/rules.mk libopencm3-examples/examples
rm -rf src/Makes

echo "  > Instalar compilador"
sudo apt install gcc-arm-embedded
echo "  > ver la version compilador"
arm-none-eabi-gcc --version

echo "Configurando para el laboratorio 4:"
echo "  > creando directorio de trabajo"
cp -rf src libopencm3-examples/examples/stm32/f4/stm32f429i-discovery/
rm -rf src

echo "  > Creando enlace Simbolico"
ln -sf /home/alexvarela/Videos/Lab_4/lab-4-microcontroladores-/libopencm3-examples/examples/stm32/f4/stm32f429i-discovery/src src

echo "  > Compilando un ejemplo para configurar biblioteca"
ln -sf /home/alexvarela/Videos/Lab_4/lab-4-microcontroladores-/libopencm3-examples/examples/stm32/f4/stm32f429i-discovery/random random
cd random
make
rm -rf random
cd ..
echo "  > Compilando el  proyecto del sismografo"
echo "  > Dirrigiendo a carpeta src"
cd src/src
echo "  > Limpiando entorno"
make clean
echo "  > Haciendo make"
make
echo "  > Generando spi-mems.bin"
st-flash write lcd-serial.bin  0x8000000
cd ..



