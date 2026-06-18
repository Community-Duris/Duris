#!/bin/bash
cd "$(dirname "$0")" || exit 1
nohup ./cycle_mud.sh &
echo "Mud started!"
