#!/bin/bash

# Define default variables
default_source_dir="../ROOT/"
main_destination_dir="gibf:/rarf/u/takayama/public_html/JSROOT/EXP/TRIUMF/2024"
base_url="https://ribf.riken.jp/~takayama/JSROOT/EXP/TRIUMF/2024/"

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
    select dir in $(find "$default_source_dir" -mindepth 1 -maxdepth 1 -type d -exec basename {} \;); do
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
files=$(find "$source_dir" -maxdepth 1 \( -name "*.root" -o -name "*.html" \) | sort)

# Check if any files were found
if [ -z "$files" ]; then
    echo "No .root or .html files found."
    exit 1
fi

# Create the new index HTML file to list all the URLs
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
        td {
            background-color: #fff;
        }
    </style>
</head>
<body>
    <header>
        <h1>$title</h1>
    </header>
    <main>
        <h2>Statistics List</h2>
        <ul>
EOF

# Create a temporary file to hold HTML entries
html_entries="$source_dir/html_entries.txt"

# Loop through each sorted file
for file in $files; do
    # Extract the base name without extension
    base_name=$(basename "$file")

    # Check if the file is a .root file
    if [[ "$base_name" == *.root ]]; then
        # Create a CGI script for .root files
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
print '      <script type="text/javascript" src="../../../../scripts/JSRootCore.js?gui"></script> ';
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

    elif [[ "$base_name" == *.html ]]; then
        # Add the HTML file URL to the temporary entries file
        echo "<tr><td><a href=\"$base_url$dir/$base_name\">$base_name</a></td><td><a href=\"$base_url$dir/${base_name%.html}.cgi\">${base_name%.html}.cgi</a></td></tr>" >> "$html_entries"
    fi

    # Transfer the file to the destination directory
    #rsync -av "$file" "$destination_dir"

    #echo "File $base_name transferred successfully."
done

# Close the JSROOT list section in the new index file
cat <<EOF >> "$index_html"
        </ul>

        <h2>Data</h2>
        <table>
            <thead>
                <tr>
                    <th>Statistics</th>
                    <th>JSROOT</th>
                </tr>
            </thead>
            <tbody>
EOF

# Append the HTML entries to the new index file
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

# Transfer the index_html file to the destination directory
#rsync -av "$index_html" "$destination_dir"

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
        <h1>TRIUMF2024 experiment: Data Platform</h1>
    </header>
    <main>
        <h2>Directory Index</h2>
        <ul>
EOF

# List directories under main destination directory and add links to each index.html
for dir in $(find "$default_source_dir" -mindepth 1 -maxdepth 1 -type d -exec basename {} \;); do
    echo "<li><a href=\"$base_url$dir/index.html\">$dir</a></li>" >> "$main_index_html"
done

# Close the HTML tags
cat <<EOF >> "$main_index_html"
        </ul>
    </main>
</body>
</html>
EOF

# Transfer the main index.html file to the main destination directory
rsync -arve "ssh" --progress -h $main_index_html $main_destination_dir
rsync -arve "ssh" --progress -h $source_dir $destination_dir

echo "Main index.html created/updated successfully."

