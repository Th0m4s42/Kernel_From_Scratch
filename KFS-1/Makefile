# ============================================================
#  Configuration
# ============================================================
IMAGE    := myos-builder
UID      := $(shell id -u)
GID      := $(shell id -g)
DOCKER   := docker run --rm -v "$(CURDIR)":/os $(IMAGE)

# ============================================================
#  Cibles côté HÔTE (lancent Docker)
# ============================================================
.PHONY: all docker-image iso run clean shell

all: iso

# Construit l'image Docker (mis en cache après la 1re fois)
docker-image:
	docker build -t $(IMAGE) .

# Compile + génère l'ISO dans le conteneur
iso: docker-image
	$(DOCKER) make -f Makefile.inner iso

# Teste dans QEMU (sur l'hôte)
run:	iso
	qemu-system-i386 -cdrom myos.iso

# Nettoyage
clean:
		$(DOCKER) make -f Makefile.inner clean

# Shell interactif dans le conteneur (debug)
shell: docker-image
	docker run --rm -it -u $(UID):$(GID) -v "$(CURDIR)":/os $(IMAGE) bash