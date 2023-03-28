#!/bin/bash
#

LASISU="git@git.las.iastate.edu:asminer/Smart.git"
GITHUB="git@github.com:asminer/smart.git"

if [ $# -ne 0 ]; then
    echo
    echo "Usage: $0"
    echo
    echo "Creates a release based on the version number in configure.ac."
    echo "Automatically updates the revision date in the source,"
    echo "and creates a tag for the release, named v[version]."
    echo
    echo "Before running, be sure that all modified files have been"
    echo "committed  to the repository."
    echo
    exit 0
fi

if [ ! -f INSTALL ]; then
    echo
    echo "Run this script in the root directory."
    echo
    exit 1
fi


echo "Checking remotes..."

OREM=`git remote -v | awk '/origin.*(push)/ {print $2}'`
GREM=`git remote -v | awk '/public-github.*(push)/ {print $2}'`

if [ "$OREM" == "$LASISU" ]; then
    echo "    Found origin from ISU"
else
    echo "    Missing: origin should be ISU repository"
    exit 1
fi

if [ "$GREM" == "$GITHUB" ]; then
    echo "    Found public-github"
else
    echo "    Missing: public-github"
    echo
    echo "        To add this remote, run"
    echo "            git remote add public-github $GITHUB"
    echo "        and then re-run the script."
    echo
    exit 1
fi


version=`awk '/AC_INIT/{print $2}' configure.ac | tr -d '[],'`


echo "Creating release for version $version"

rdate=`date +"%Y %B %d"`
echo "const char* SMART_DATE = \"$rdate\";" > src/revision.h

nextver=`awk -F. '{for (i=1; i<NF; i++) printf($i"."); print $NF+1}' <<< $version`

#
# Save changes
#

git add src/revision.h
git commit -m "Updated release date for $version"
git push origin
git push public-github

#
# Create and publish a tag for $version
#

git tag -a "v$version" -m "Tag for release $version"
git push origin v$version
git push public-github v$version


#
# Automatically bump the version number
#

echo "Updating version to $nextver, edit configure.ac to change"
sed "/AC_INIT/s/$version/$nextver/" configure.ac > configure.new
mv -f configure.ac configure.ac.old
mv configure.new configure.ac

