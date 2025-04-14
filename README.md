# TP1_SO

#### descargar la imagen de docker con 

docker pull agodio/itba-so-multi-platform:3.0

#### correr en la terminal primero

docker run -v "${PWD}:/root" --privileged -ti agodio/itba-so-multi-platform:3.0

#### MAKEFILE CMDS

make **__// compiles everithing__**

make run **__// runs the "run" configured__**

make analysis **__// runs PVS-Studio on all compiled files__**

make clean **__// removes /bin ; /analysis ; and the created compile_commands.json__**

#### Valgrind
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --log-file=valgrind-out.txt \
         bin/master [options]
