#!/bin/sh

# If you change this, you may want to do it in a copy, and point to it in
# cc3dsfs_script.sh by changing ENV_SCRIPT, so it is not reverted when you update.

export TO_TOUCH_FILE=${HOME}/tmp_${PROGRAM_NAME}_file
export TOUCH_COMMAND="--touch_file ${TO_TOUCH_FILE}"

# Example of extra commands for cc3dsfs. By default, nothing is added.
# This example makes it so the software does not scan for Nisetro DS(i) capture cards.
#export EXTRA_COMMANDS="--no_nisetro"
# This example makes it so the software does not scan for Optimize Old 3DS capture cards.
#export EXTRA_COMMANDS="--no_opt_o3ds_cc"
# This example does not add anything.
export EXTRA_COMMANDS=""
