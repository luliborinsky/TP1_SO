# TP1_SO

#### descargar la imagen de docker con 

docker pull agodio/itba-so-multi-platform:3.0

#### correr en la terminal primero

docker run -v "${PWD}:/root" --privileged -ti agodio/itba-so-multi-platform:3.0

bpftrace -e/-lv ' profile:hz:99 / tracepoint:syscalss:sys_enter(exit)_openat {@[comm,kstack()]=count()} / {printf()/@b=hist(args->ret)} interval:s:2 {exit()}'