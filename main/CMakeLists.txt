set(srcs "main.c"
	"ili9340_sjis.c"
	"fontx.c"
	"m5stack.c"
	)

idf_component_register(SRCS ${srcs} INCLUDE_DIRS "." EMBED_TXTFILES cert.pem)

