#!/usr/bin/env bash
echo This batch is an admin tool to return QB64-PE to its pre-setup state

echo Purging temp folders
rm -rf temp temp2 temp3 temp4 temp5 temp6 temp7 temp8 temp9
echo Replacing main temp folder
mkdir temp

echo Replacing dummy file in temp folder to maintain directory structure
cp source/temp.bin temp/temp.bin


echo Pruning source folder
rm -rf source/undo2.bin
rm -rf source/recompile.bat
rm -rf source/debug.bat
rm -rf source/files.txt
rm -rf source/paths.txt
rm -rf source/root.txt
rm -rf source/bookmarks.bin
rm -rf source/recent.bin

echo Culling precompiled libraries
rm -rf /s c/libqb/*.o
rm -rf /s c/libqb/*.a
rm -rf /s c/parts/*.o
rm -rf /s c/parts/*.a

echo Culling temporary copies of qbx.cpp such as qbx2.cpp
rm -rf c/qbx2.cpp c/qbx3.cpp c/qbx4.cpp c/qbx5.cpp c/qbx6.cpp c/qbx7.cpp c/qbx8.cpp c/qbx9.cpp
