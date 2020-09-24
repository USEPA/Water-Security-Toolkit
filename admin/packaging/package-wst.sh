#!/usr/bin/env bash -e

if test -z "$WORKSPACE"; then
    echo "ERROR: \$WORKSPACE is not set."
    exit 1
fi

if test -n "$1"; then
    ZIPDIR="$1"
else
    ZIPDIR=wst
fi
if test -n "$2"; then
    ZIPNAME="$2"
else
    ZIPNAME=wst
fi


CLIEXE="$WORKSPACE"/wst/admin/packaging/cli.executable
TARGET="$WORKSPACE/$ZIPDIR"

# Clean things out
rm -rf "$TARGET"
mkdir -p "$TARGET"/dist/python

# Unzip the python.zip cache file (wst/admin/vpy/python.zip)
pushd "$TARGET"/dist/python
unzip "$WORKSPACE"/wst/admin/vpy/python.zip
# Extract any packages that were downloaded as TGZ/ZIP archives
pushd dist
CLEARGLOB=`shopt nullglob | grep off | wc -l`
if test $CLEARGLOB -eq 1; then
    shopt -s nullglob
fi
for x in *.tar.gz *.tgz; do
    tar -xzf $x
    rm $x
done
for x in *.tar; do
    tar -xf $x
    rm $x
done
for x in *.zip; do
    unzip $x
    rm $x
done
if test $CLEARGLOB -eq 1; then
    shopt -u nullglob
fi
popd
popd

# Add the WST packages / tpls
for package in pywst pyepanet; do
    rm -rf "$TARGET"/dist/python/src/$package
    cp -r "$WORKSPACE"/wst/[pt][ap][cl]*/$package "$TARGET"/dist/python/src/
done

# Extract the compiled binaries
echo "(INFO) Copying the compiled binaries"
mkdir "$TARGET"/bin
mkdir tmp-bin
pushd tmp-bin
for zipfile in "$WORKSPACE"/executables/*.zip; do
    unzip $zipfile
done
find . -name \*.exe -exec mv -v {} "$TARGET"/bin/. \;
popd 
rm -rf tmp-bin
find "$WORKSPACE"/executables -name \*.exe -exec cp -vp {} "$TARGET"/bin/. \;

# Build the "gateway" scripts
AUTODIST=wst/admin/packaging/autodist/pyutilib/autodist
cp "$WORKSPACE"/$AUTODIST/autodist.py "$TARGET"/bin
cp "$WORKSPACE"/$AUTODIST/autodist_core.py "$TARGET"/bin
for script in wst sp booster_response flushing_response pyomo runef; do
    echo "(INFO) Creating gateway autodist script for $script"
    cp "$CLIEXE" "$TARGET"/bin/${script}.exe
    cat > "$TARGET"/bin/${script}-script.py <<EOF
#!python
import autodist
autodist.run_script('${script}')
EOF
done

# Copy over selected top-level directories and files
for dir in examples \*.txt; do
    echo "(INFO) Copying $dir from the WST source"
    cp -rp "$WORKSPACE/wst"/$dir "$TARGET/."
done

# Clean out selected examples sub-directories 
for dir in simple tutorial; do
    echo "(INFO) Removing examples/$dir from target directories"
    rm -rf "$TARGET/examples/$dir"
done

# Promote specific license text out of the admin directory
#for file in admin/LICENSE\*; do
#    echo "(INFO) Copying $file from the WST source"
#    cp -rvp "$WORKSPACE/wst/"$file "$TARGET/."
#done

# Copy over selected pywst documentation
echo "(INFO) Copying wst documentation from the wst source"
mkdir -p "$TARGET"/doc/
cp -rp "$WORKSPACE"/wst/doc/wst/*.pdf "$TARGET"/doc/.

# Clean out any ".svn" directories
echo "(INFO) Removing .svn from target directories"
find "$TARGET" -name .svn | xargs -i rm -rf "{}"

# Build the final zipfile
#   (current dir *should* be $WORKSPACE, but just in case...)
cd "$WORKSPACE"
rm -f "$WORKSPACE"/*.zip
zip -r "$WORKSPACE"/$ZIPNAME.zip $ZIPDIR

# Clean up the working directory for the next build
rm -rf "$TARGET"
