#!/bin/bash

ATTRS_OFF="` tput sgr0    2>/dev/null`"
BOLD="`      tput bold    2>/dev/null`"
BLACK="`     tput setaf 0 2>/dev/null`"
RED="`       tput setaf 1 2>/dev/null`"
GREEN="`     tput setaf 2 2>/dev/null`"
YELLOW="`    tput setaf 3 2>/dev/null`"
BLUE="`      tput setaf 4 2>/dev/null`"
MAGENTA="`   tput setaf 5 2>/dev/null`"
CYAN="`      tput setaf 6 2>/dev/null`"
LIGHT_GRAY="`tput setaf 7 2>/dev/null`"

for i in *.[ch]; do
  ll="$(
    awk '
      length > 79 {
        print "'"${BOLD}"'" FILENAME ":" FNR ":'"${ATTRS_OFF}"'\t" \
                                               "'"${RED}"'"$0"'"${ATTRS_OFF}"'"
      }
    ' "$i"
  )"

  [[ "$ll" != "" ]] && {
    echo "${BOLD}${MAGENTA}warning:${ATTRS_OFF} long lines found in '$i'"
    echo "$ll"
    echo
  }
done

exit 0;
