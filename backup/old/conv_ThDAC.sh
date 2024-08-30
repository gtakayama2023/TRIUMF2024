#!/bin/bash

# Function to convert decimal to hexadecimal
decimal_to_hex() {
    printf "%X\n" $1
}

# Main script
for (( runN=1; runN<=10; runN++ )); do
    hex_runN=$(decimal_to_hex $runN)
    ftree=1

    g++ ./test_717.cpp -o rawdata2root `root-config --cflags --libs` -lSpectrum

    root -l -b <<EOF
.L ./test_717.cpp++g
Rawdata2root($hex_runN,"$ftree")
EOF

done
