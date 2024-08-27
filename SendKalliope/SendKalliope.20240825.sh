#!/bin/bash

# Define variables
source_dir="../ROOT/test_0808/"
destination_dir="gibf:/rarf/u/takayama/public_html/JSROOT/EXP/TRIUMF/2024/Kalliope0808/"
base_url="https://ribf.riken.jp/~takayama/JSROOT/EXP/TRIUMF/2024/Kalliope0808/"

# Find all .root and .html files in the directory and sort them by name
files=$(find "$source_dir" -maxdepth 1 \( -name "*.root" -o -name "*.html" \) | sort)

# Check if any files were found
if [ -z "$files" ]; then
    echo "No .root or .html files found."
    exit 1
fi

# Create an HTML file to list all the URLs
index_html="$source_dir/index.html"
cat <<EOF > "$index_html"
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Kalliope test (08/08/2024) for TRIUMF2024</title>
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
        <h1>Kalliope test (08/08/2024) for TRIUMF2024</h1>
    </header>
    <main>
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
print '     <div id="simpleGUI" path="./" files="$base_name"> ';
print '         loading scripts ...';
print '      </div>';
print '   </body>';
print '</html>';
EOF

        # Make the CGI script executable
        chmod +x "$cgi_script"

        # Add the CGI script URL to the index.html file
        #echo "        <li><a href=\"$base_url${base_name%.root}.cgi\">${base_name%.root}.cgi</a></li>" >> "$index_html"

    elif [[ "$base_name" == *.html ]]; then
        # Add the HTML file URL to the temporary entries file
        echo "<tr><td><a href=\"$base_url$base_name\">$base_name</a></td><td><a href=\"$base_url${base_name%.html}.cgi\">${base_name%.html}.cgi</a></td></tr>" >> "$html_entries"
    fi

    # Transfer the file to the destination directory
    rsync -av "$file" "$destination_dir"

    echo "File $base_name transferred successfully."
done

# Close the JSROOT list section in the index file
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

# Append the HTML entries to the index.html file
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

# Transfer the index.html file to the destination directory
rsync -av "$source_dir" "$destination_dir"

