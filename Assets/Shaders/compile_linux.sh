#!/bin/sh

BASEDIR=$(dirname $0)
for file in $BASEDIR/*.glsl; do
  echo "Compiling $file to ${file%.glsl}.spv"
  $BASEDIR/glslc "$file" -o "${file%.glsl}.spv"

echo "The shade has been compilade"
done