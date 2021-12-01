file(GLOB rijndael_src
    "source/rijndael-alg-fst.h"
    "source/rijndael-alg-fst.c"
)

#add_library(rijndael STATIC ${rijndael_src})
add_library(rijndael_obj OBJECT ${rijndael_src})
add_library(rijndael STATIC $<TARGET_OBJECTS:rijndael_obj>)
