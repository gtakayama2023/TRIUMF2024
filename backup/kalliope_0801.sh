#!/bin/bash

echo "Please type number which you want to do"
echo "0: rawdata2root"
echo "1: ThDACScan"
echo "2: PlotAllHist"
echo "3: Check Channel Setting"
echo "4: Check Real Channel Assign"
echo "5: Test rawdata2root"
echo "6: Test NIMTDC2root"

read -p "type No. : " choice

case "$choice" in
    0|1|2|3|4|5|6) ;;
#if [ "choice" != "0" ] && [ "$choice" != "1" ] && [ "$choice" != "2" ] && [ "$choice" != "3" ] && [ "$choice" != "4" ] && [ "$choice" != "5" ] && [ "$choice" != "6" ]; then
    *) echo "invalid input"; exit 1 ;;
esac
#fi

if [ "$choice" = "1" ]; then
    read -p "Type IP_max of Kalliope. Type 0 for experiment : " IP_max
fi

read -p "Fill data in tree? [0:No, 1:Yes] : " ftree

if [ "$choice" == "5" ]; then
    read -p "run No. : " runN
    g++ ./rawdata2root.cpp -o rawdata2root `root-config --cflags --libs` -lSpectrum
    #runN=94
    IP_max=3
    path="test_0808/run_$runN"
    #path="noise_HV55"
    root -l -b<<EOF
.L ./rawdata2root.cpp++g
Rawdata2root($runN,$IP_max,$ftree,"$path")
EOF
fi



if [ "$choice" == "0" ]; then
    read -p "type runNo : " runN
    #path="run_$runN"
    path="noise_HV55"
    raw_path="../RAW/$path"
    root_path="../ROOT/$path"
    if [ ! -d "$raw_path" ]; then
	echo "Rawdata Directry does not exist : $raw_path"
	echo "exit"
	exit 1
    else
	echo "Directory already exists: $raw_path"
    fi
    if [ ! -d "$root_path" ]; then
	echo "Creating directory: $root_path"
	mkdir -p "$root_path"
    else
	echo "Directory already exists: $root_path"
    fi
    echo "execute rawdata2root"
    g++ ./backup/test_79.cpp -o rawdata2root `root-config --cflags --libs` -lSpectrum
    root -l -b <<EOF
.L ./backup/test_79.cpp++g
Rawdata2root($runN,$IP_max,$ftree,"$path")
EOF
elif [ "$choice" == "1" ]; then
    read -p "For which? [0:noise, 1:signal] : " NS
    if [ "$NS" == "0" ]; then
	name="noise"
    elif [ "$NS" == "1" ]; then
	name="signal"
    fi
    read -p "Type in HV value : " HV
    #path="${name}_HV${HV}"
    path="${name}_20240808"
    raw_path="../RAW/$path"
    root_path="../ROOT/$path"
    if [ ! -d "$raw_path" ]; then
	echo "Rawdata Directry does not exist : $raw_path"
	echo "exit"
	exit 1
    else
	echo "Directory already exists: $raw_path"
    fi
    if [ ! -d "$root_path" ]; then
	echo "Creating directory: $root_path"
	mkdir -p "$root_path"
    else
	echo "Directory already exists: $root_path"
    fi
    echo "execute ThDACScan"
    g++ ./backup/test_79.cpp -o rawdata2root `root-config --cflags --libs` -lSpectrum
    root -l <<EOF
.L ./backup/test_79.cpp++g
ThDACScan($IP_max,$ftree,"$path")
.L ./macros/PlotAllNhit.cpp++g
Plot_All_Histograms("$path")
EOF
    #open ../pdf/$path.pdf
fi

if [ "$choice" == "2" ]; then
    root -l ./macros/script_PlotAllHist.C
fi

if [ "$choice" == "3" ]; then
    root -l ./macros/script_Check_CH_Setting.C
fi

if [ "$choice" == "4" ]; then
    runN=0
    path="test_noise"
    root -l ./macros/script_Check_Real_CH.C
fi



if [ "$choice" == "6" ]; then
    g++ ./NIMTDC2root.cpp -o NIMTDC2root `root-config --cflags --libs` -lSpectrum
    runN=10
    path="signal_HV55/run_$runN"
    root -l -b<<EOF
.L ./NIMTDC2root.cpp++g
NIMTDC2root($runN,$ftree,"$path")
EOF
fi
