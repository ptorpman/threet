#!/usr/bin/bash

if [ $# -ne 1 ]; then
   echo "Usage: $0 <file>"
   exit 1
fi

echo "* Indenting $1 ..."
res=`emacs -batch $1 -l ../emacs-format-file -f emacs-format-function 2>&1 1>/dev/null`
echo "* Done."
