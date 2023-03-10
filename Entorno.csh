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

#Aqui el usuario debe especificar la ruta antes del directorio del repositorio.
echo " > Por favor agite el  STM32F29I Discovery kit y visite iot.eie.ucr.ac.cr para ver el estado del giroscopio y la bateria en la plataforma Thingsboard"
python3 ~/Documents/lab-microcontroladores/laboratorio_4/lab-4-microcontroladores-/src/mqtt_publish.py


echo "El script sigue corriendo"
while true; do
    echo "Por favor presione CTRL+Z para detener el proceso"
    sleep 1
    kill $!
done

# Stdio del teclado para detener el proceso, si se detiene, lo mata de una vez y asi no queda en el cache
if [ $? -eq 148 ]; then
    #Aqui se mata el script
    echo "Gracias por usar la plataforma de Thinsboard"
    kill $$
fi
echo "Eliminar proceso de Thingsboard"
jobs -l

# Matar todo
echo "Terminando de enviar datos."
for job in `jobs -l | awk '{print $2}'`; do
    kill $job
done

echo "Se ha detenido el programa, por favor desconecte la tarjeta"
kill -9 `jobs -ps`
