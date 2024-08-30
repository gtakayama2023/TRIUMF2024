#!/bin/bash

# Define constants
TODAY="0808"  # For testing, set today manually
RAW_DIR="../RAW"
ROOT_DIR="../ROOT"

# Check if command-line arguments are provided
if [ "$#" -gt 0 ]; then
    choice=$1
    fNIM=${2:-0}
    ftree=${3:-0}
else
    # Prompt user for action if no arguments provided
    echo "Please type the number corresponding to what you want to do:"
    echo "0: Test rawdata2root"
    echo "1: ThDACScan"
    echo "2: PlotAllHist"
    echo "3: Check Channel Setting"
    echo "4: Check Real Channel Assign"
    echo "5: rawdata2root"
    echo "6: Test NIMTDC2root"

    read -p "Type No.: " choice

    # Additional prompts for choices 0, 1, or 2
    if [[ "$choice" == "0" || "$choice" == "1" || "$choice" == "2" ]]; then
        read -p "Read NIM-TDC? [0: No, 1: Yes] : " fNIM
        case "$fNIM" in
            0|1) ;; 
            *) echo "Invalid input. Type 0 or 1"; exit 1;;
        esac

        read -p "Fill in tree? [0: No, 1: Yes] : " ftree
        case "$ftree" in
            0|1) ;; 
            *) echo "Invalid input. Type 0 or 1"; exit 1;;
        esac
    fi
fi

# Main action based on user choice
case "$choice" in
    0|1|2)
        # Define paths
        runN=7  # Example run number
        path="test_$TODAY/run_$runN"
        raw_path="$RAW_DIR/$path"
        root_path="$ROOT_DIR/$path"

        # Check if the raw data directory exists
        if [ ! -d "$raw_path" ]; then
            echo "Rawdata directory does not exist: $raw_path"
            TODAY_DIR="$RAW_DIR/test_$TODAY"
            echo "$TODAY_DIR contains the following files:"
            ls "$TODAY_DIR"
            echo "Exiting..."
            exit 1
        else
            echo "Directory already exists: $raw_path"
        fi

        # Check if the root data directory exists, create if not
        if [ ! -d "$root_path" ]; then
            echo "Creating directory: $root_path"
            mkdir -p "$root_path"
        else
            echo "Directory already exists: $root_path"
        fi

        # Actions for specific choices
        case "$choice" in
            0)  # Test rawdata2root
                IP_max=2
                root -l -b -q "./rawdata2root.cpp($runN, $IP_max, $fNIM, $ftree, \"$path\")"
                cat ./txt/Nevent_run$runN.txt
                echo ""
                if sed '1d' ./txt/EvtMatch_run$runN.txt | grep -q "Mismatch"; then
                    grep "Mismatch" ./txt/EvtMatch_run$runN.txt
                    echo ""
                fi
                ;;
            1)
                # Placeholder for ThDACScan functionality
                ;;
        esac
        ;;
    3)
        # Placeholder for Check Channel Setting
        ;;
    4)
        # Check Real Channel Assign
        runN=0
        path="test_noise"
        # Placeholder for script execution
        ;;
    5)
        # Placeholder for rawdata2root functionality
        ;;
    6)
        # Placeholder for Test NIMTDC2root functionality
        ;;
    *)
        echo "Invalid input"; exit 1 ;;
esac

