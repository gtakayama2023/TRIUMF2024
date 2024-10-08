<?php
// Initialize form values
$data_dir = isset($_POST['data_dir']) ? $_POST['data_dir'] : '';
$runN = isset($_POST['runN']) ? $_POST['runN'] : '';
$ftree = isset($_POST['ftree']) ? $_POST['ftree'] : '';
$choice = isset($_POST['choice']) ? $_POST['choice'] : '';
$fNIM = isset($_POST['fNIM']) ? $_POST['fNIM'] : '';
$ONLINE_FLAG = isset($_POST['ONLINE_FLAG']) ? $_POST['ONLINE_FLAG'] : '';
// for DC mode
$DC_mode = isset($_POST['DC_mode']) ? $_POST['DC_mode'] : '-1';
$th_txCurve_min = isset($_POST['th_txCurve_min']) ? $_POST['th_txCurve_min'] : '800';
$th_txCurve_max = isset($_POST['th_txCurve_max']) ? $_POST['th_txCurve_max'] : '1500';
$th_tracking_min = isset($_POST['th_tracking_min']) ? $_POST['th_tracking_min'] : '700';
$th_tracking_max = isset($_POST['th_tracking_max']) ? $_POST['th_tracking_max'] : '1600';
$eventNum = isset($_POST['eventNum']) ? $_POST['eventNum'] : '10000';


// Process form submission
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['action']) && $_POST['action'] === 'Write to load.sh') {
    // Prepare the content for the shell script
    $script_content = "#!/bin/bash\n";
    $script_content .= "choice=$choice\n";
    $script_content .= "fNIM=$fNIM\n";
    $script_content .= "ftree=$ftree\n";
    $script_content .= "path_choice=\"".str_replace('../ROOT/', '', $data_dir)."/\"\n";
    $script_content .= "runN=$runN\n";
    $script_content .= "selected_dir=\"$data_dir/\"\n";
    $script_content .= "runN_choice=$runN\n";
    $script_content .= "ONLINE_FLAG=$ONLINE_FLAG\n";
    $script_content .= "DC_mode=$DC_mode\n";
    $script_content .= "th_txCurve_min=$th_txCurve_min\n";
    $script_content .= "th_txCurve_max=$th_txCurve_max\n";
    $script_content .= "th_tracking_min=$th_tracking_min\n";
    $script_content .= "th_tracking_max=$th_tracking_max\n";
    $script_content .= "eventNum=$eventNum\n";

    // Path to the shell script
    $script_path = './KAL/load.sh';

    // Write content to the shell script file
    file_put_contents($script_path, $script_content);

    // Make the script executable
    chmod($script_path, 0755);

    echo "<div class='container mt-4'>";
    echo "<div class='alert alert-success' role='alert'>";
    echo "<h1 class='text-success'>Parameters Loaded Successfully</h1>";
    echo "<p>Parameters have been written to <code>$script_path</code>.</p>";
    echo "</div>";
    
    // Display the parameters
    echo "<div class='card mt-3'>";
    echo "<div class='card-header bg-primary text-white'>";
    echo "<h2 class='mb-0'>Parameters Set: </h2>";
    echo "</div>";
    echo "<div class='card-body'>";
    echo "<pre class='bg-primary text-white p-3 rounded'>";
    echo "<strong>selected_dir:</strong> $data_dir/\n";
    echo "<strong>ftree:</strong> $ftree\n";
    echo "<strong>runN:</strong> $runN\n";
    echo "<strong>choice:</strong> $choice\n";
    echo "<strong>fNIM:</strong> $fNIM\n";
    echo "<strong>path_choice:</strong> ".str_replace('../RAW/', '', $data_dir)."/\n";
    echo "<strong>ONLINE_FLAG:</strong> $ONLINE_FLAG\n";
    echo "<strong>DC_mode:</strong> $DC_mode\n";
    echo "<strong>th_txCurve_min:</strong> $th_txCurve_min\n";
    echo "<strong>th_txCurve_max:</strong> $th_txCurve_max\n";
    echo "<strong>th_tracking_min:</strong> $th_tracking_min\n";
    echo "<strong>th_tracking_max:</strong> $th_tracking_max\n";
    echo "<strong>eventNum:</strong> $eventNum\n";
    echo "</pre>";
    echo "</div>";
    echo "</div>";
    
    // Add button to run the script
    echo "<div class='card mt-3'>";
    echo "<h2 class='mb-0'>Next thing you should do: </h2>";
    echo "<pre>> ssh kal-dc-ana@142.90.154.231\nPW: m20c\n> KAL_ANA_START\n</pre>";
    #echo "<a href='http://142.90.154.232/JSROOT/EXP/TRIUMF/2024/ANA/conv.cgi' class='btn btn-danger btn-lg' role='button' target='_blank'>";
    #echo "Are you sure to run?";
    #echo "</a>";
    echo "</div>";
    
    // Add button to run the script
    echo "<div class='card mt-3'>";
    echo "<h2 class='mb-0'>Links: </h2>";
    echo "<a href='http://142.90.154.231/JSROOT/EXP/TRIUMF/2024/ROOT/index.html' class='btn btn-danger btn-lg' role='button' target='_blank'>";
    echo "Home";
    echo "</a>";
    #echo "<a href='http://142.90.154.232/JSROOT/EXP/TRIUMF/2024/ANA/conv.cgi' class='btn btn-danger btn-lg' role='button' target='_blank'>";
    #echo "Are you sure to run?";
    #echo "</a>";
    echo "</div>";

} else {
    // Display the HTML form for selecting directory and other parameters
    $raw_dir = "../ROOT"; // Set this path correctly
    $directories = array_diff(scandir($raw_dir), array('..', '.'));

    // Process the selected directory
    $selected_dir = $data_dir;


    // raw_dirの"../ROOT"を"../RAW"に変更する
    $raw_dir = str_replace('ROOT', 'RAW', $raw_dir);

    $runN_suggestion = '';
    if ($selected_dir && $selected_dir !== 'Other') {
        $files = glob("$selected_dir/*.rawdata");
        $runNs = [];

        foreach ($files as $file) {
            if (preg_match('/MSE(\d{6})/', basename($file), $matches)) {
                $runN = (int)$matches[1];
                $runNs[] = $runN;
            }
        }

        if (!empty($runNs)) {
            $runNs = array_unique($runNs);
            sort($runNs);
            $suggested_runN = end($runNs) + 0;
        } else {
            $suggested_runN = 1;
        }
        $runN_suggestion = $suggested_runN;
    }
    $th_txCurve_min_suggestion = 800;
    $th_txCurve_max_suggestion = 1500;
    $th_tracking_min_suggestion = 700;
    $th_tracking_max_suggestion = 1600;
    $eventNum_suggestion = 10000;

    // Extract the directory name from the selected path
    $directory_name = basename($selected_dir);

    if ($_SERVER['REQUEST_METHOD'] === 'POST') {
        $data_dir = $_POST['data_dir'] ?? '';
        $runN = $_POST['runN'] ?? '';
        $ftree = $_POST['ftree'] ?? '';
        $choice = $_POST['choice'] ?? '';
        $fNIM = $_POST['fNIM'] ?? '';
        $ONLINE_FLAG = $_POST['ONLINE_FLAG'] ?? '';
        $DC_mode = $_POST['DC_mode'] ?? '0';
        $th_txCurve_min = $_POST['th_txCurve_min'] ?? '800';
        $th_txCurve_max = $_POST['th_txCurve_max'] ?? '1500';
        $th_tracking_min = $_POST['th_tracking_min'] ?? '700';
        $th_tracking_max = $_POST['th_tracking_max'] ?? '1600';
        $eventNum = $_POST['eventNum'] ?? '10000';
    } else {
        $data_dir = '../ROOT';
        $runN = '1';
        $ftree = '0';
        $choice = '0';
        $fNIM = '0';
        $ONLINE_FLAG = '0';
        $DC_mode = '0';
        $th_txCurve_min = '800';
        $th_txCurve_max = '1500';
        $th_tracking_min = '700';
        $th_tracking_max = '1600';
        $eventNum = '10000';
    }
    

    // Construct a select form for directory choices
    echo '<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>TRIUMF 2024 Experiment: ANA for Kalliope</title>
    <link href="https://stackpath.bootstrapcdn.com/bootstrap/4.5.2/css/bootstrap.min.css" rel="stylesheet">
    <style>
        /* ボタンの背景を青にして、文字色はそのまま */
        .btn-primary {
            background-color: #007bff; /* 青いボタン */
            border-color: #0056b3;     /* 濃い青の境界線 */
            color: white;              /* ボタンの文字を白に */
        }

        /* ボタンホバー時の色を調整 */
        .btn-primary:hover {
            background-color: #0056b3; /* 濃い青に変化 */
            border-color: #004080;     /* ホバー時の境界線もさらに濃い青 */
            color: white;              /* ホバー時も文字は白のまま */
        }
    </style>
</head>
<body>
    <div class="container">
        <a href="http://142.90.154.231/JSROOT/EXP/TRIUMF/2024/ROOT/index.html" class="btn btn-primary home-button">Home</a>
        <h1 class="text-center mb-4">TRIUMF 2024 Experiment: ANA for Kalliope</h1>
        <form action="" method="post">
            <div class="form-group">
                <label for="data_dir">DATA_DIR:</label>
                <select id="data_dir" name="data_dir" class="form-control" onchange="this.form.submit()" required>
                    <option value="">Select a directory...</option>';

    foreach ($directories as $dir) {
        $selected = ($selected_dir == "$raw_dir/$dir") ? 'selected' : '';
        echo "<option value=\"$raw_dir/$dir\" $selected>$dir</option>";
    }

    echo '       <option value="Other">Other</option>
                </select>
            </div>';

    // if selected directory name includes "DC", then show the DC parameters
    // DC_mode : 0 (raw mode) or 1 (suppress mode)
    if (strpos($directory_name, 'DC') !== false) {
        echo '<div class="form-group">
            <label for="DC_mode">DC_mode:</label>
            <select id="DC_mode" name="DC_mode" class="form-control" required onchange="this.form.submit()">
                <option value="0" ' . ($DC_mode === '0' ? 'selected' : '') . '>0: raw mode</option>
                <option value="1" ' . ($DC_mode === '1' ? 'selected' : '') . '>1: suppress mode</option>
            </select>
        </div>';
    }

    if ($selected_dir && $selected_dir !== 'Other') {
        echo '<div class="form-group">
            <label for="runN">runN:</label>
            <input type="number" id="runN" name="runN" class="form-control" value="' . htmlspecialchars($runN_suggestion) . '" required>
        </div>';
    } else {
        echo '<div class="form-group">
            <label for="runN">runN:</label>
            <input type="number" id="runN" name="runN" class="form-control" value="' . htmlspecialchars($runN) . '" required>
        </div>';
    }

    //if selected DC mode is 1, then show the threshold parameters
    // when DC_mode was selected, parameters are shown or hidden
    if ($DC_mode === '1' && strpos($directory_name, 'DC') !== false) {
        echo '<div class="form-group">
            <label for="th_txCurve_min">th_txCurve_min:</label>
            <input type="number" id="th_txCurve_min" name="th_txCurve_min" class="form-control" value="' . htmlspecialchars($th_txCurve_min_suggestion) . '" required>
        </div>

        <div class="form-group">
            <label for="th_txCurve_max">th_txCurve_max:</label>
            <input type="number" id="th_txCurve_max" name="th_txCurve_max" class="form-control" value="' . htmlspecialchars($th_txCurve_max_suggestion) . '" required>
        </div>

        <div class="form-group">
            <label for="th_tracking_min">th_tracking_min:</label>
            <input type="number" id="th_tracking_min" name="th_tracking_min" class="form-control" value="' . htmlspecialchars($th_tracking_min_suggestion) . '" required>
        </div>

        <div class="form-group">
            <label for="th_tracking_max">th_tracking_max:</label>
            <input type="number" id="th_tracking_max" name="th_tracking_max" class="form-control" value="' . htmlspecialchars($th_tracking_max_suggestion) . '" required>
        </div>

        <div class="form-group">
            <label for="eventNum">eventNum:</label>
            <input type="number" id="eventNum" name="eventNum" class="form-control" value="' . htmlspecialchars($eventNum_suggestion) . '" required>
        </div>';
    }
    
    
    echo '<div class="form-group">
            <label for="choice">choice:</label>
            <select id="choice" name="choice" class="form-control" required>
                <option value="0" ' . ($choice === '0' ? 'selected' : '') . '>0: Run rawdata2root</option>
                <option value="1" ' . ($choice === '1' ? 'selected' : '') . '>1: ThDACScan</option>
            </select>
        </div>

        <div class="form-group">
            <label for="fNIM">fNIM:</label>
            <select id="fNIM" name="fNIM" class="form-control" required>
                <option value="0" ' . ($fNIM === '1' ? 'selected' : '') . '>0</option>
                <option value="1" ' . ($fNIM === '0' ? 'selected' : '') . '>1</option>
            </select>
        </div>

        <div class="form-group">
            <label for="ONLINE_FLAG">ONLINE_FLAG:</label>
            <select id="ONLINE_FLAG" name="ONLINE_FLAG" class="form-control" required>
                <option value="0" ' . ($ONLINE_FLAG === '0' ? 'selected' : '') . '>0</option>
                <option value="1" ' . ($ONLINE_FLAG === '1' ? 'selected' : '') . '>1</option>
            </select>
        </div>

        <div class="form-group">
            <label for="ftree">ftree:</label>
            <select id="ftree" name="ftree" class="form-control" required>
                <option value="0" ' . ($ftree === '0' ? 'selected' : '') . '>0</option>
                <option value="1" ' . ($ftree === '1' ? 'selected' : '') . '>1</option>
            </select>
        </div>

        <button type="submit" name="action" value="Write to load.sh" class="btn btn-primary">Write to load.sh</button>
    </form>
    </div>

    <script src="https://code.jquery.com/jquery-3.5.1.slim.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/@popperjs/core@2.5.4/dist/umd/popper.min.js"></script>
    <script src="https://stackpath.bootstrapcdn.com/bootstrap/4.5.2/js/bootstrap.min.js"></script>
</body>
</html>';
}
?>
