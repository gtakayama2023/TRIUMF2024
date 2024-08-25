#!/bin/bash

#TODAY=`date +%m%d`
TODAY="0808"
RAW_DIR="../RAW"
ROOT_DIR="../ROOT"

echo "Please type number which you want to do"
echo "0: Test rawdata2root"
echo "1: ThDACScan"
echo "2: PlotAllHist"
echo "3: Check Channel Setting"
echo "4: Check Real Channel Assign"
echo "5: rawdata2root"
echo "6: Test NIMTDC2root"

read -p "type No. : " choice

case "$choice" in
    0|1|2)
	read -p "Read NIM-TDC? [0:No, 1:Yes] : " fNIM
	case "$fNIM" in
	    0|1) ;; *) echo "Invalid input. Type 0 or 1"; exit 1;;
	esac
	read -p "Fill in tree? [0:No, 1:Yes] : " ftree
	case "$ftree" in
	    0|1) ;; *) echo "Invalid input. Type 0 or 1"; exit 1;;
	esac
	#read -p "run No. : " runN
	runN=94
	path="test_$TODAY/run_$runN"
	raw_path="$RAW_DIR/$path"
	root_path="$ROOT_DIR/$path"
	if [ ! -d "$raw_path" ]; then
            echo "Rawdata Directry does not exist : $raw_path"
	    TODAY_DIR="$RAW_DIR/test_$TODAY"
	    echo "$TODAY_DIR includes these files"
	    ls $TODAY_DIR
            echo "exit"
            exit 1
	else
	    echo "Directory already exists: $raw_path"
	fi
	
	#if [ -e "$RAW_DIR/test_$TODAY/run_$runN/MSE000094_192_168_10_16.rawdata" ]; then
	#    fNIM=1
	#else
	#    fNIM=0
	#fi
	if [ ! -d "$root_path" ]; then
            echo "Creating directory: $root_path"
            mkdir -p "$root_path"
	else
            echo "Directory already exists: $root_path"
	fi

	case "$choice" in
	    0)
		g++ ./rawdata2root.cpp -o rawdata2root `root-config --cflags --libs` -lSpectrum
		IP_max=3
		path="test_0808/run_$runN"
		root -l -b<<EOF
.L ./rawdata2root.cpp++g	
Rawdata2root($runN,$IP_max,$fNIM,$ftree,"$path")
EOF
		cat ./txt/Nevent_run$runN.txt
		echo ""
		if sed '1d' ./txt/EvtMatch_run$runN.txt | grep -q "Mismatch"; then
		  grep "Mismatch" ./txt/EvtMatch_run$runN.txt
		  echo ""
		fi
		;;
	    1)
		;;
	esac
	;;
#	1);;
#	2);;
#	;;
#	root -l ./macros/script_PlotAllHist.C;;
    3);;
#	root -l ./macros/script_Check_CH_Setting.C;;
    4)
	runN=0
	path="test_noise";;
#	root -l ./macros/script_Check_Real_CH.C;;
    5);;
    6);;
    *) echo "invalid input"; exit 1 ;;
esac
