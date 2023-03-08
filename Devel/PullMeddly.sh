#!/bin/bash

if ! git subrepo --version 1> /dev/null 2> /dev/null; then
  echo "This script needs git subrepo."
  echo "To install, try the following."
  echo "    git clone https://github.com/ingydotnet/git-subrepo /path/to/subrepo"
  echo "    Run /path/to/subrepo/.rc in .bashrc or .zshrc"
  echo
  exit 1
fi

if [ ! -f src/_Meddly/.gitrepo ]; then
  cd ..
fi
if [ ! -f src/_Meddly/.gitrepo ]; then
  echo "Run this script either in the repository root directory or in Devel"
  exit 1
fi

git subrepo pull src/_Meddly
