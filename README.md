# TP1_SO

#### descargar la imagen de docker con 

docker pull agodio/itba-so-multi-platform:3.0

#### correr en la terminal primero

docker run -v "${PWD}:/root" --privileged -ti agodio/itba-so-multi-platform:3.0

#### MAKEFILE CMDS

make **__compiles everithing__**

make run **__runs the "run" configured__**

make analysis **__runs PVS-Studio on all compiled files__**

make clean **__removes /bin ; /analysis ; and the created compile_commands.json__**
