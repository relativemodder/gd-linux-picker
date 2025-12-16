#!/bin/sh

make

rm main.o
rm starterproxy.exe
mv starterproxy.exe.so ../resources/