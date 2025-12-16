#!/bin/sh

make

rm main.o
rm starter-proxy.exe
mv starter-proxy.exe.so ../resources/