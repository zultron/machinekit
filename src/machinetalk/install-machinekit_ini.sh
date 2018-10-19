#!/bin/bash
#
# Install machinekit.ini without clobbering any existing MKUUID

SRC="$1"
DST="$2"

# Read any current values
read_val() {
    awk -F = "/^${1}/ {print \$2}"  $DST
}
replace_vars="MKUUID REMOTE"
if test -f $2; then
    add_vars="GROUP_STATUS_PORT COMMAND_PORT LOG_PORT"
    add_vars+=" BIND_IPV4 BIND_IPV6 ANNOUNCE_IPV4 ANNOUNCE_IPV6"
    for v in $add_vars $replace_vars; do
        eval "${v}=\$(read_val ${v})"
    done
fi

# Copy new file
cp "$SRC" "$DST"

# Fill empty MKUUID and REMOTE
if test -z "$MKUUID"; then
    MKUUID=$(uuidgen)
    echo "    Generated new MKUUID=$MKUUID"
fi
test -n "$REMOTE" || REMOTE=1

# Update file
replace_val() {
    test -n "$2" || return
    sed -i "$DST" -e "s/^#\?${1}=[^ ]*$/${1}=${2}/"
}
for v in $add_vars $replace_vars; do
    eval val=\"\$${v}\"
    replace_val $v "$val"
done
