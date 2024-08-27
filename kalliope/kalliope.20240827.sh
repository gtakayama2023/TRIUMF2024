#!/bin/bash

# Define constants
TODAY="track"
RAW_DIR="../RAW"
ROOT_DIR="../ROOT"
HISTORY_FILE="./kalliope/history.sh"

# Default values for variables
choice=-1
fNIM=0
ftree=0
path_choice="prompt"
runN=7
selected_dir=""
selected_subdir=""
runN_choice=""

# Function to save current settings to history file
save_to_history() {
    echo "#!/bin/bash" > $HISTORY_FILE
    echo "choice=$choice" >> $HISTORY_FILE
    echo "fNIM=$fNIM" >> $HISTORY_FILE
    echo "ftree=$ftree" >> $HISTORY_FILE
    echo "path_choice=\"$path_choice\"" >> $HISTORY_FILE
    echo "runN=$runN" >> $HISTORY_FILE
    echo "selected_dir=\"$selected_dir\"" >> $HISTORY_FILE
    echo "selected_subdir=\"$selected_subdir\"" >> $HISTORY_FILE
    echo "runN_choice=\"$runN_choice\"" >> $HISTORY_FILE
}

# Check if command-line arguments are provided
if [ "$#" -gt 0 ]; then
    # Handle special mode for loading previous settings
    if [ "$1" == "load" ]; then
        if [ -f "$HISTORY_FILE" ]; then
            source "$HISTORY_FILE"
            echo "Loaded previous settings from $HISTORY_FILE"
        else
            echo "No history file found."
            exit 1
        fi
    else
        choice=$1
        fNIM=${2:-$fNIM}
        ftree=${3:-$ftree}
        path_choice=${4:-$path_choice}
        runN=${5:-$runN}
    fi
else
    echo "Please type the number corresponding to what you want to do:"
    echo "0: Test rawdata2root"
    echo "1: ThDACScan"
    echo "2: PlotAllHist"
    echo "3: Check Channel Setting"
    echo "4: Check Real Channel Assign"
    echo "5: rawdata2root"
    echo "6: Test NIMTDC2root"
    read -p "Type No.: " choice

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

        path_choice="prompt"
    fi
fi

# Select directory if not provided as a command-line argument
if [ -z "$selected_dir" ]; then
    dirs=($(ls -d $RAW_DIR/*/))
    echo "Available directories in RAW_DIR:"
    for i in "${!dirs[@]}"; do
        echo "$i: ${dirs[$i]}"
    done
    read -p "Select directory: " dir_choice
    selected_dir=${dirs[$dir_choice]}
fi

# Select subdirectory if not provided as a command-line argument
if [ -z "$selected_subdir" ]; then
    subdirs=($(ls -d ${selected_dir}*/))
    echo "Available subdirectories:"
    for i in "${!subdirs[@]}"; do
        echo "$i: ${subdirs[$i]}"
    done
    read -p "Select subdirectory: " subdir_choice
    selected_subdir=${subdirs[$subdir_choice]}
fi

# Extract and list runN from filenames
files=($(ls ${selected_subdir}*.rawdata))
runNs=($(for file in "${files[@]}"; do
    basename "$file" | sed -n 's/.*MSE0*\([0-9]*\).*/\1/p'
done))
unique_runNs=($(printf "%s\n" "${runNs[@]}" | sort -nu))

# Check if any runNs were found
if [ ${#unique_runNs[@]} -eq 0 ]; then
    echo "No runN values found in ${selected_subdir}"
    exit 1
fi

if [ -z "$runN_choice" ]; then
    echo "Available runN:"
    for i in "${!unique_runNs[@]}"; do
        echo "$i: ${unique_runNs[$i]}"
    done
    read -p "Select runN: " runN_choice
    runN=${unique_runNs[$runN_choice]}
fi

path="${selected_subdir%/}"
path="${path#../RAW/}"
echo "Selected path: $path"

# Continue with your existing logic
root_path="$ROOT_DIR/$path"

# Check if the raw data directory exists
if [ ! -d "$selected_subdir" ]; then
    echo "Rawdata directory does not exist: $selected_subdir"
    exit 1
else
    echo "Directory already exists: $selected_subdir"
fi

# Check if the root data directory exists, create if not
if [ ! -d "$root_path" ]; then
    echo "Creating directory: $root_path"
    mkdir -p "$root_path"
else
    echo "Directory already exists: $root_path"
fi

# Main action based on user choice
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
    2)
        # Placeholder for PlotAllHist functionality
        ;;
    3)
        # Placeholder for Check Channel Setting functionality
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

# Save current settings to history file
save_to_history
