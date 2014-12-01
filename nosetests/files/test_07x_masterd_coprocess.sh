#!/bin/bash
#
# This is a script that forks a few times and then runs 'sleep 10'.
#
# It produces plenty of output about each fork, exit codes, and
# signals caught.
#
# A plain 'kill -15' to the top-level process will not kill the lower
# layers, and in fact when $NONSTOP is true, will not even register
# with the signal handlers until the 'sleep' process exits.

# Parameter:  Number of shell levels to fork
MAX_DEPTH=3

# Parameter:  Whether to run e.g. one 10-second 'sleep' or ten 1-second
# 'sleep' subprocesses
NONSTOP=true

# Parameter:  How long to sleep
SECONDS=10


# Fork depth
export DEPTH=$((0$DEPTH + 1))

# Pretty identification output
ident() { echo "$(date +%s) (pid $$/depth $DEPTH)"; }

# Signal handler
croak_on_signal() {
    echo "$(ident):  Caught signal; exiting 42"
    exit 42
}
trap croak_on_signal SIGINT SIGTERM SIGHUP

# Sleep routine
sleeper() {
    echo "$(ident) sleeping..."
    if $NONSTOP; then
	# Sleep once for $SECONDS seconds
    	sleep $SECONDS
    else
	# Sleep many times, on second each, for $SECONDS seconds
    	for i in `seq $SECONDS`; do
    	    sleep 1
	    echo "$(ident)    ...slept $i seconds"
    	done
    fi
}

# Main loop
if test $DEPTH -lt $MAX_DEPTH; then
    # Above MAX_DEPTH, kick off a child process
    CHILD_DEPTH=$(($DEPTH + 1))

    echo "$(ident) forking child depth=$CHILD_DEPTH..."
    $0; RES=$?
    echo "$(ident) ...child depth=$CHILD_DEPTH exited with code $RES"
    echo "$(ident) exiting code 7"
    exit 7

elif test $DEPTH = $MAX_DEPTH; then
    # At MAX_DEPTH, run the sleeper
    sleeper
    exit 86

else
    # How'd we get here?
    echo "Uh oh, unknown DEPTH='$DEPTH'"
    exit 13
fi
