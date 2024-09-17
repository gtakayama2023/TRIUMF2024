#!/bin/bash

# Define constants
TODAY="track"
RAW_DIR="../RAW"
ROOT_DIR="../ROOT"
HISTORY_FILE="./KAL/load.sh"

# Default values for variables
choice=-1
fNIM=0
ftree=0
path_choice="prompt"
runN=7
selected_dir=""
#selected_subdir=""
runN_choice=""
ONLINE_FLAG=0  # New ONLINE_FLAG default
DC_mode=-1
th_xtCurve_min=800;
th_xtCurve_max=1500;
th_tracking_min=700;
th_tracking_max=1600;
eventNum=10000;
target_dir=""

# Function to save current settings to load file
save_to_load() {
    echo "#!/bin/bash" > $HISTORY_FILE
    echo "choice=$choice" >> $HISTORY_FILE
    echo "fNIM=$fNIM" >> $HISTORY_FILE
    echo "ftree=$ftree" >> $HISTORY_FILE
    echo "path_choice=\"$path_choice\"" >> $HISTORY_FILE
    echo "runN=$runN" >> $HISTORY_FILE
    echo "selected_dir=\"$selected_dir\"" >> $HISTORY_FILE
    #echo "selected_subdir=\"$selected_subdir\"" >> $HISTORY_FILE
    echo "runN_choice=\"$runN_choice\"" >> $HISTORY_FILE
    echo "ONLINE_FLAG=$ONLINE_FLAG" >> $HISTORY_FILE  # Save ONLINE_FLAG
    echo "DC_mode=$DC_mode" >> $HISTORY_FILE
    echo "th_xtCurve_min=$th_xtCurve_min" >> $HISTORY_FILE
    echo "th_xtCurve_max=$th_xtCurve_max" >> $HISTORY_FILE
    echo "th_tracking_min=$th_tracking_min" >> $HISTORY_FILE
    echo "th_tracking_max=$th_tracking_max" >> $HISTORY_FILE
    echo "eventNum=$eventNum" >> $HISTORY_FILE
}

# Check if command-line arguments are provided
if [ "$#" -gt 0 ]; then
    # Handle special mode for loading previous settings
    if [ "$1" == "load" ]; then
        if [ -f "$HISTORY_FILE" ]; then
            source "$HISTORY_FILE"
            echo "Loaded previous settings from $HISTORY_FILE"
            target_dir=${selected_dir#../RAW/}
            target_dir=${target_dir%/}
            echo "Selected directory: $selected_dir"
            echo "Target directory: $target_dir"
        else
            echo "No load file found."
            exit 1
        fi
    else
        choice=$1
        fNIM=${2:-$fNIM}
        ftree=${3:-$ftree}
        path_choice=${4:-$path_choice}
        runN=${5:-$runN}
        ONLINE_FLAG=${6:-$ONLINE_FLAG}  # Set ONLINE_FLAG from command-line or use default
    fi
else
    echo "Please type the number corresponding to what you want to do:"
    echo "0: Run rawdata2root"               
    echo "1: ThDACScan"               
    echo "2: PlotAllHist"               
    echo "3: Check Channel Setting"               
    echo "4: Check Real Channel Assign"               
    echo "5: rawdata2root"               
    echo "6: Test NIMTDC2root" 
    echo "7: DC Test"              
    read -p "Type No.: " choice

    echo "Selected choice: $choice"

    if [[ "$choice" == "7" ]]; then
        echo "DC Test"
        target_dir="DC_TEST"

        echo "Please select run number:"
        read -p "Type No.: " runN

        echo "Please select run mode:"
        echo "0: raw mode"
        echo "1: sup mode"
        read -p "Type No.: " DC_mode
        if [ "$DC_mode" == "1" ]; then
            echo "Sup mode"

            echo "Please select parameters:"
            read -p "Threshold for x-t curve (min): " th_xtCurve_min
            read -p "Threshold for x-t curve (max): " th_xtCurve_max
            read -p "Threshold for tracking (min) : " th_tracking_min
            read -p "Threshold for tracking (max) : " th_tracking_max
            read -p "Number of events             : " eventNum
        fi
    fi

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

        read -p "Set ONLINE_FLAG? [0: No, 1: Yes] : " ONLINE_FLAG
        case "$ONLINE_FLAG" in
            0|1) ;;
            *) echo "Invalid input. Type 0 or 1"; exit 1;;
        esac

        path_choice="prompt"
    fi
fi

echo "target_dir: $target_dir"
echo "DC_mode: $DC_mode"

if [[ "$target_dir" == "DC_TEST" || "$target_dir" == "DC_RUN" ]]; then
    if [ "$DC_mode" == "0" ]; then
	echo "Raw mode"
	
	analysis_dir="/home/kal-dc-ana/RP1212/ana/RP1212/vecvec"
	echo "Selected directory: $analysis_dir"
	cd $analysis_dir
	
	root -l -b -q 'decodeRun.C('$runN')'
	./rawmode.sh $runN
	source_dir="/var/www/html/JSROOT_ORG/"
	base_file_name=$(printf "run_%04d_raw" "$runN")
	target_dir="/home/kal-dc-ana/EXP/TRIUMF/2024/ROOT/DC_TEST/"
	cp -r "${source_dir}${base_file_name}.root" $target_dir
	echo "File copied to $target_dir"
	base_file_name_MSE=$(printf "MSE%06d" "$runN")
	mv "${target_dir}${base_file_name}.root" "${target_dir}${base_file_name_MSE}.root"
	echo "File renamed to $base_file_name_MSE"
	cd $target_dir
	touch "${base_file_name_MSE}.html"
	cd /home/kal-dc-ana/EXP/TRIUMF/2024/ANA
	echo "begin send.sh"
	./KAL/send.sh "DC_TEST"
	echo "end send.sh"
	exit 0
    fi
fi

if [[ "$target_dir" == "DC_TEST" || "$target_dir" == "DC_RUN" ]]; then
    if [ "$DC_mode" == "1" ]; then
	echo "Sup mode"

	analysis_dir="/home/kal-dc-ana/RP1212/ana/RP1212/vecvec"
	echo "Selected directory: $analysis_dir"
	cd $analysis_dir
	
	root -l -b -q 'decodeRun.C('$runN')'
	./supmode.sh $runN $th_xtCurve_min $th_xtCurve_max $th_tracking_min $th_tracking_max $eventNum
	source_dir="/var/www/html/JSROOT_ORG/"
	base_file_name=$(printf "run_%04d_sup" "$runN")
	target_dir="/home/kal-dc-ana/EXP/TRIUMF/2024/ROOT/DC_TEST/"
	cp -r ${source_dir}${base_file_name}.root $target_dir
	echo "File copied to $target_dir"
	base_file_name_MSE=$(printf "MSE%06d" "$runN")
	mv ${target_dir}${base_file_name}.root ${target_dir}${base_file_name_MSE}.root
	echo "File renamed to $file_name_MSE"
	cd $target_dir
	touch "${file_name_MSE}.html"
	cd /home/kal-dc-ana/EXP/TRIUMF/2024/ANA
	echo "begin send.sh"
	./KAL/send.sh "DC_TEST"
	echo "end send.sh"
	exit 0
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
#if [ -z "$selected_subdir" ]; then
#    subdirs=($(ls -d ${selected_dir}*/))
#    echo "Available subdirectories:"
#    for i in "${!subdirs[@]}"; do
#        echo "$i: ${subdirs[$i]}"
#    done
#    read -p "Select subdirectory: " subdir_choice
#    selected_subdir=${subdirs[$subdir_choice]}
#fi
selected_subdir=$selected_dir

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
# Test rawdata2root
IP_max=2
#root -l -b -q "./rawdata2root.cpp($runN, $IP_max, $fNIM, $ftree, \"$path\", $ONLINE_FLAG)" 
root -l -b -q "./KAL/conv.C($choice, $runN, $IP_max, $fNIM, $ftree, \"$path\", $ONLINE_FLAG)"  

cat ./txt/Nevent_run$runN.txt
echo ""
if sed '1d' ./txt/EvtMatch_run$runN.txt | grep -q "Mismatch"; then
    grep "Mismatch" ./txt/EvtMatch_run$runN.txt
    echo ""
fi

# Save current settings to load file
save_to_load

./KAL/send.sh $path
