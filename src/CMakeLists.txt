aux_source_directory(. SRC)

add_library(src ${SRC}
        decoder.cpp
        decoder.h)
