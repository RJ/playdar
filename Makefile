_configure.h: VERSION
	@echo '#define SCROBSUB_CLIENT_VERSION "'`cat VERSION`'"' > _configure.h
	@echo '#define SCROBSUB_API_KEY "c8c7b163b11f92ef2d33ba6cd3c2c3c3"' >> _configure.h
	@echo '#define SCROBSUB_SHARED_SECRET "73582dfc9e556d307aead069af110ab8"' >> _configure.h
