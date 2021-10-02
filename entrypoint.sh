#!/bin/sh
set -e

if [[ "${USE_ENV}" == "true" ]]; then
	TMPDIR="/gserver/servers/${SERVER}"

	if [ `find ${TMPDIR} -prune -empty 2>/dev/null` ]; then
		echo "Directory empty, copying over default files"
		cp -fr /gserver/servers/default/* ${TMPDIR}/
	fi
fi

if [ -z "$@" ]; then
	exec /gserver/gs2emu
	echo "done"
else
	exec $@
fi
