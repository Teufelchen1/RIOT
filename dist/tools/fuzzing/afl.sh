#!/bin/sh

if [ ! -d "${APPDIR}/input" ]; then
	echo "${APPDIR}: Doesn't provide a test corpus" 1>&2
	exit 1
fi

mkdir -p "${APPDIR}/findings"
exec afl-fuzz -i - -o "${APPDIR}/findings" "$@" -- \
	"${FLASHFILE}" "${PORT}" ${TERMFLAGS}
