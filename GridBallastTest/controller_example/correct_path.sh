#!/bin/bash
oldpath='/tmp/temp'
newpath=`pwd`'/local_gd' 
gsed -i "s#$oldpath#$newpath#" local_gd/bin/gridlabd
gsed -i "s#$oldpath#$newpath#" local_gd/lib/gridlabd/glxengine.la
