#!/bin/bash

# Runs as many npc instances as the Mac OSX machine has cores.
# CTRL-C to kill them

function ctrlc {
  killall npc
  echo "Killed them... Bye."
  exit -1
}

trap ctrlc SIGINT

cores=$( sysctl hw.ncpu | awk '{print $2}' )

echo "Starting $cores processes... "
for ii in $(seq 1 $cores); do
  ./npc &
  sleep 2 # rand() is seeded from time, must wait to change starting point
done

echo "started."


for job in `jobs -p`; do
  echo $job
  wait $job
done

