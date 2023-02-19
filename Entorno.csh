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

echo "Configurando para el laboratorio 4:"
echo "  > creando directorio de trabajo"
cp -rf src libopencm3-examples/examples/stm32/f4/stm32f429i-discovery/
rm -rf src

echo "  > Creando enlace Simbolico"
ln -sf libopencm3-examples/examples/stm32/f4/stm32f429i-discovery/src src

echo "  > Compilando un ejemplo para configurar biblioteca"
ln -sf libopencm3-examples/examples/stm32/f4/stm32f429i-discovery/random random
cd random
make
st-flash write random.bin  0x8000000
cd ..
rm -rf random

echo "  > Compilando el  proyecto del sismografo"
echo "  > Dirrigiendo a carpeta src"
cd src
echo "  > Limpiando entorno"
make clean
echo "  > Haciendo make"
make
echo "  > Generando Sismografo.bin"
st-flash write Sismografo.bin  0x8000000
cd ..

echo "  > FALTA AGREGAR LO DE IOT"

Python3 ~/Documents/lab-microcontroladores/laboratorio_4/lab-4-microcontroladores-/src/mqtt_publish.py
