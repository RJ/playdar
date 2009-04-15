audioscrobbled: EXAMPLE.c scrobsub.c portable/http-curl.c portable/auth-libxml2.c portable/md5.c portable/persistence-simple.c _configure.h
	gcc -w -std=c99 -g3 -ggdb -o $@ -I. EXAMPLE.c -lcrypto -lxml2 -lcurl

_configure.h: VERSION
	@echo '#define SCROBSUB_CLIENT_VERSION "'`cat VERSION`'"' > _configure.h
	@echo '#define SCROBSUB_API_KEY "c8c7b163b11f92ef2d33ba6cd3c2c3c3"' >> _configure.h
	@echo '#define SCROBSUB_SHARED_SECRET "73582dfc9e556d307aead069af110ab8"' >> _configure.h
