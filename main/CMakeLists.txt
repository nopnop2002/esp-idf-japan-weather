set(srcs "main.c"
    "ili9340"
    "fontx.c"
    "m5stack.c"
    )

idf_component_register(SRCS ${srcs}
    INCLUDE_DIRS "."
    EMBED_TXTFILES weather_yahoo_cert.pem)

