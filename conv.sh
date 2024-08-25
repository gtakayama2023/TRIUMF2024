#!/bin/bash

# ファイル名を引数として受け取る
runN=$1
tree=$2

# rawdata2root.cppをコンパイル
g++ test.cpp -o rawdata2root `root-config --cflags --libs` -lSpectrum

# SIGINTシグナルを受け取ったときのハンドラを設定
#trap 'echo "Interrupt signal received. Do you want to exit? (0: Yes, 1: No)"; read answer; if [ "$answer" -eq 0 ]; then exit 1; fi' INT

# rootをバックグラウンドで開く
root -l -b <<EOF
.L test.cpp++g
Rawdata2root($runN,$tree)
.q
EOF
