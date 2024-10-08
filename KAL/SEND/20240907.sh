#!/bin/bash

# Define default variables
default_source_dir="../ROOT/"
base_url232="http://142.90.154.232/JSROOT/EXP/TRIUMF/2024/ROOT/" # DAQ
base_url231="http://142.90.154.231/JSROOT/EXP/TRIUMF/2024/ROOT/" # ANA

# If a directory name is provided as a positional argument, use it as source_dir
if [ $# -gt 0 ]; then
  dir_name="$1"
  source_dir="$default_source_dir$dir_name/"
else
  # Parse command-line arguments
  while getopts ":s:" opt; do
    case ${opt} in
      s )
        source_dir="$OPTARG"
        ;;
      \? )
        echo "Usage: cmd [-s source_dir] or cmd <dir_name>"
        exit 1
        ;;
    esac
  done
  shift $((OPTIND -1))

  # If no source_dir is specified, list directories under default_source_dir
  if [ -z "$source_dir" ]; then
    echo "Available directories under $default_source_dir:"
    select dir in $(find "$default_source_dir" -mindepth 1 -maxdepth 1 -type d -exec basename {} \; | sort); do
      if [ -n "$dir" ]; then
        source_dir="$default_source_dir$dir/"
        break
      else
        echo "Invalid selection. Try again."
      fi
    done
  fi
fi

# Verify source_dir
if [ ! -d "$source_dir" ]; then
  echo "Source directory does not exist: $source_dir"
  exit 1
fi

# Extract the title from the README file
readme_file="$source_dir/README"
if [ -f "$readme_file" ]; then
  title=$(head -n 1 "$readme_file")
else
  title="Kalliope test (08/08/2024) for TRIUMF2024"
fi

# Create new index file name
index_html="$source_dir/index.html"

# Set the destination directory based on the selected directory
dir=$(basename "$source_dir")
destination_dir="gibf:/rarf/u/takayama/public_html/JSROOT/EXP/TRIUMF/2024/$dir/"

# Find all .root and .html files in the directory and sort them by name
files=$(find "$source_dir" -maxdepth 1 \( -name "*.root" -o -name "*.html" \) | sort -r)

# Check if any files were found
if [ -z "$files" ]; then
    echo "No .root or .html files found."
    exit 1
fi

# Define paths
index_html="$source_dir/index.html"

# Create the combined HTML file
cat <<EOF > "$index_html"
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>$title</title>
    <link rel="stylesheet" href="styles.css">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 0;
            background-color: #f4f4f4;
            color: #333;
        }
        header {
            background-color: #007bff;
            color: #fff;
            padding: 1rem;
            text-align: center;
        }
        main {
            padding: 2rem;
        }
        h1 {
            font-size: 2rem;
            margin-bottom: 1rem;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 1rem;
        }
        th, td {
            border: 1px solid #ddd;
            padding: 0.5rem;
            text-align: left;
        }
        th {
            background-color: #f4f4f4;
        }
        tr:nth-child(even) {
            background-color: #f9f9f9;
        }
        tr:hover {
            background-color: #f1f1f1;
        }
        a {
            color: #007bff;
            text-decoration: none;
        }
        a:hover {
            text-decoration: underline;
        }
    </style>
</head>
<body>
    <header>
        <h1>$title</h1>
    </header>
    <main>
	<li>
    	<a href="http://142.90.154.231/JSROOT/EXP/TRIUMF/2024/ROOT/index.html">Home</a>
	</li>
	<li>
	<a href="https://docs.google.com/spreadsheets/d/1LdJk-tyk8UCVEBBByiUcNRiroIleWNkwO0eEJwqG1mE/edit?usp=sharing">Runsummary (Google Sheets)</a>
	</li>
        <h2>Run Summary and Data</h2>
        <table>
            <thead>
                <tr>
                    <th>File</th>
                    <th>Statistics</th>
                    <th>JSROOT</th>
                    <th>Start Date</th>
                    <th>Run Info</th>
                    <th>Run Time</th>
                    <th>IP Addresses</th>
                </tr>
            </thead>
            <tbody>
EOF

# Create a temporary file to hold HTML entries
html_entries="$source_dir/html_entries.txt"
> "$html_entries"  # Clear the temporary file if it exists

# Loop through each file (only MSE files)
for file in $(ls -r "$source_dir"/*); do
    base_name=$(basename "$file")

    # Check if the file name starts with MSE
    if [[ "$base_name" == MSE* ]]; then
        # Check if the file is a .csv file
        if [[ "$file" == *.csv ]]; then
            # Extract the filename without extension
            csv_filename=$(basename "$file" .csv)

            # Read the parameters from the CSV
            run_info=$(csvtool col 1 "$file" | tail -n +2)
            start_date=$(csvtool col 2 "$file" | tail -n +2)
            run_time=$(csvtool col 3 "$file" | tail -n +2)
            IP_adress=$(csvtool col 4 "$file" | tail -n +2)

            # Add the CSV file information to the summary table
            cat <<EOF >> "$index_html"
                    <tr>
                        <td>$csv_filename</td>
                        <td><a href="$base_url231$dir/${csv_filename}.html">$csv_filename.html</a></td>
                        <td><a href="$base_url232$dir/${csv_filename}.cgi">${csv_filename}.cgi</a></td>
                        <td>$start_date</td>
                        <td>$run_info</td>
                        <td>$run_time</td>
                        <td>$IP_adress</td>
                    </tr>
EOF

        elif [[ "$file" == *.root ]]; then
            # Create a CGI script for the .root file
            cgi_script="$source_dir/${base_name%.root}.cgi"
            cat <<EOF > "$cgi_script"
#!/usr/bin/perl

print "Content-type: text/html\\n\\n";

print '<!DOCTYPE html> ';
print '<html lang="en"> ';
print '   <head> ';
print '      <meta charset="UTF-8"> ';
print '      <meta http-equiv="X-UA-Compatible" content="IE=edge"> ';
print '      <title>Read a ROOT file</title> ';
print '      <link rel="shortcut icon" href="img/RootIcon.ico"/> ';
print '      <script type="text/javascript" src="../../../../../scripts/JSRootCore.js?gui"></script> ';
print '      <script type="text/javascript"> ';
print '         JSROOT.gStyle.SetPalette(55); ';
print '      </script> ';
print '   </head> ';
print '   <body> ';
print '      <div id="simpleGUI" path="./" files="$base_name"> ';
print '         loading scripts ...';
print '      </div>';
print '   </body>';
print '</html>';
EOF

            # Make the CGI script executable
            chmod +x "$cgi_script"

            # Add to the HTML entries (Statistics and JSROOT columns)
            echo "<tr><td>$base_name</td><td><a href=\"$base_url231$dir/${base_name%.root}.html\">${base_name%.root}.html</a></td><td><a href=\"$base_url232$dir/${base_name%.root}.cgi\">${base_name%.root}.cgi</a></td></tr>" >> "$html_entries"

        elif [[ "$file" == *.html ]]; then
            # Add HTML files to the temporary HTML entries list
            echo "<tr><td>$base_name</td><td><a href=\"$base_url232$dir/$base_name\">$base_name</a></td><td><a href=\"$base_url232$dir/${base_name%.html}.cgi\">${base_name%.html}.cgi</a></td></tr>" >> "$html_entries"
        fi
    fi
done

# Append the HTML entries from the temporary file to the summary table
cat "$html_entries" >> "$index_html"

# Close the table and HTML tags
cat <<EOF >> "$index_html"
            </tbody>
        </table>
    </main>
</body>
</html>
EOF

# Clean up temporary file
rm "$html_entries"

# Transfer the index_html file and csv_summary_html file to the destination directory
#rsync -av "$index_html" "$csv_summary_html" "$destination_dir"

# Create or update the main index.html file with links to all index files
main_index_html="../ROOT/index.html"
#if [ ! -f "$main_index_html" ]; then
cat <<EOF > "$main_index_html"
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
		<title>TRIUMF2024 Experiment (02/09/2024 - 04/10/2024) </title>
    <link rel="stylesheet" href="styles.css">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 0;
            background-color: #f4f4f4;
            color: #333;
        }
        header {
            background-color: #007bff;
            color: #fff;
            padding: 1rem;
            text-align: center;
        }
        main {
            padding: 2rem;
        }
        h1 {
            font-size: 2rem;
            margin-bottom: 1rem;
        }
        ul {
            list-style-type: none;
            padding: 0;
        }
        li {
            margin: 0.5rem 0;
        }
        a {
            color: #007bff;
            text-decoration: none;
        }
        a:hover {
            text-decoration: underline;
        }
    </style>
</head>
<body>
    <header>
        <h1>TRIUMF2024 experiment: DAQ and ANA Platform</h1>
    </header>
    <main>
	<h2>Link</h2>
	<ul>
	    <li><a href="https://docs.google.com/document/d/13fUyKNljUS0DHTU0ejrSJgy6c_CmIjeES9y-XU0g_1A/edit#heading=h.x045u3w4ja0l"> Online Logbook</a></li>
	    <li><a href="https://github.com/kyasuda0820/TRIUMF2024_DC">DC Anlysis (GitHub Repository)</a></li>
	    <li><a href="https://github.com/gtakayama2023/TRIUMF2024">Kalliope Analysis (GitHub Repository)</a></li>
	    <li><a href="https://github.com/gtakayama2023/TRIUMF2024_KAL_DAQ">Kalliope DAQ (GitHub Repository)</a></li>
	</ul>
        <h2>DAQ</h2>
        <ul>
	    <li><a href="http://142.90.154.232/JSROOT/EXP/TRIUMF/2024/SlowControl/run.php">Kalliope DAQ</a> (ここから Kalliope DAQ の Run 設定を決めますのだ。)</li>
        </ul>
        <h2>ANA</h2>
        <ul>
	    <li><a href="http://142.90.154.231/JSROOT/EXP/TRIUMF/2024/ANA/conv.php">Kalliope ANA</a> (ここから Kalliope のデータ解析の設定を決めますのだ。)</li>
	<h2>Run information & JSROOT</h2>
        <li><a href="http://142.90.154.232/JSROOT/EXP/TRIUMF/2024/SlowControl/runsummary.cgi"> Runsummary</a></li>
EOF

# List directories under main destination directory and add links to each index.html
for dir in $(find "$default_source_dir" -mindepth 1 -maxdepth 1 -type d -exec basename {} \; | sort); do
    echo "<li><a href=\"$base_url231$dir/index.html\">$dir</a></li>" >> "$main_index_html"
done

# Close the HTML tags
cat <<EOF >> "$main_index_html"
        </ul>
        <h2>JupyROOT</h2>
	    <li><a href="http://142.90.154.231:8888/tree?">JupyROOT</a> (ここから好きな ROOT file を開いて自由に解析できますのだ。)</li>
	    <p>Jupyter Notebook を開けない場合は以下の手順で起動してくださいなのだ:</p>
	    <ul>
	        <li>SSHでサーバーに接続するのだ:</li>
	        <pre>> ssh kal-dc-ana@142.90.154.231</pre>
	        <li>パスワード: <strong>m20c</strong></li>
	        <li>接続後、Jupyter Notebookを起動するには以下を実行するのだ:</li>
	        <pre>> jn</pre>
	    </ul>
        </ul>
    </main>
</body>
</html>
EOF

# Transfer the main index.html file to the main destination directory
#rsync -arve "ssh" --progress -h $main_index_html $main_destination_dir
#rsync -arve "ssh" --progress -h $source_dir $destination_dir

echo "Main index.html and CSV summary created/updated successfully."
