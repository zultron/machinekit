#!/bin/bash -e

usage() {
    test -z "$*" || info "$*"
    {
	echo "Usage:"
	echo "  $0 [ options ] CHROOT COMMAND [ ARG ... ]"
	echo "    options:"
	echo "      -v                      extra verbose"
	echo "      -d                      debug"
	echo "      -l                      list containers"
	echo "      -b                      build container"
	echo "      -s                      run shell in container"
	echo "      -L                      run last intermediate image"
	echo "      -m                      mount /usr/src/machinekit"
	echo "      -u                      update tarball in chroot"
	echo "  containers:"
	list-containers 2>&1 | sed 's/^/      /'
    } >&2
    exit 1
}

info() {
    echo "$*" >&2
}

debug-info() {
    ! $DDEBUG || echo "$*" >&2
}

list-containers() {
    for i in $TOP_DIR/docker/Dockerfile.*; do
	echo ${i#$TOP_DIR/docker/Dockerfile.} >&2
    done
    exit 0
}


TOP_DIR=$(readlink -f $(dirname $0)/..)

DEBUG=false
DDEBUG=false  # Double-verbose
RUN_SHELL=false
RUN_BUILD=false
RUN_LAST=false
UPDATE_TARBALL=false
while getopts dvlmbsLu ARG; do
    case $ARG in
        d) DEBUG=true ;;
        v) DDEBUG=true ;;
	l) list-containers ;;
	m) MOUNT_VOL=--volume=$TOP_DIR:/usr/src/machinekit ;;
	b) RUN_BUILD=true ;;
	s) RUN_SHELL=true ;;
	L) RUN_LAST=true ;;
	u) UPDATE_TARBALL=true ;;
        *) usage "Unrecognized option '$ARG'" ;;
    esac
done
shift $((OPTIND-1))

debug-info "DEBUG=$DEBUG"
debug-info "DDEBUG=$DDEBUG"
debug-info "MOUNT_VOL=$MOUNT_VOL"
debug-info "RUN_BUILD=$RUN_BUILD"
debug-info "RUN_SHELL=$RUN_SHELL"
debug-info "RUN_LAST=$RUN_LAST"


# Build archive for import into chroot
if $UPDATE_TARBALL; then
    TARBALL=docker/machinekit.tar.gz
    info "Updating tarball in $TARBALL"
    cd $(dirname $0)/..
    git archive --prefix=machinekit/ --format=tar HEAD | \
	gzip > $TARBALL
fi

# Run latest container
if $RUN_LAST; then
    IMAGE=$(docker images -q | head -1)
    info "Running image $IMAGE in container"
    CMD="docker run -i -t $MOUNT_VOL $IMAGE"
    debug-info "CMD=$CMD"
    $CMD
    exit $?
fi

CONTAINER=$1; shift || true
test -n "$CONTAINER" || usage
DOCKERFILE=$TOP_DIR/docker/Dockerfile.$CONTAINER
test -f $DOCKERFILE || \
    usage "Unknown container '$CONTAINER'" ;
debug-info "CONTAINER=$CONTAINER"
debug-info "DOCKERFILE=$DOCKERFILE"

# Set Dockerfile
cp $DOCKERFILE $TOP_DIR/docker/Dockerfile

if $RUN_BUILD; then
    docker build -t mk-$CONTAINER docker
fi

if $RUN_SHELL; then
    docker run -i -t $MOUNT_VOL mk-$CONTAINER
fi
