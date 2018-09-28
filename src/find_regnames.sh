#!/bin/sh

grep is_matched * | grep if | cut -d: -f2 | sed 's/^ *//g' | sed 's/else //g' | \
 cut -d\" -f2 | sort | uniq | grep -v if
