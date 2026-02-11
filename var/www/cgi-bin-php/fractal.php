<?php declare(strict_types=1);

function squaredNorm($c)
{
	return ($c['real'] * $c['real'] + $c['imag'] * $c['imag']);
}

function absComplex($c)
{
	$result;

	$result['real'] = $c['real'];
	$result['imag'] = $c['imag'];
	if ($c['real'] < 0)
		$result['real'] = -$c['real'];
	if ($c['imag'] < 0)
		$result['imag'] = -$c['imag'];
	return ($result);
}

function addComplex($a, $b)
{
	$result;

	$result['real'] = $a['real'] + $b['real'];
	$result['imag'] = $a['imag'] + $b['imag'];
	return ($result);
}

function squareComplex($c)
{
	$result;

	$result['real'] = $c['real'] * $c['real'] - $c['imag'] * $c['imag'];
	$result['imag'] = 2.0 * $c['real'] * $c['imag'];
	return ($result);
}

function addComplexSquared($a, $b) {
	return (addComplex(squareComplex($a), $b));
}

function mandelbrotComputeIteration($z, $c, $max_iter) : int {
	$iter = 0;

	while ($iter < $max_iter && squaredNorm($z) <= 4) {
		$z = addComplexSquared($z, $c);
		$iter++;
	}
	return ($iter);
}

function juliaComputeIteration($z, $c, $max_iter) : int {
	$iter = 0;

	while ($iter < $max_iter && squaredNorm($c) <= 4) {
		$c = addComplexSquared($c, $z);
		$iter++;
	}
	return ($iter);
}

function burningShipComputeIteration($z, $c, $max_iter) : int {
	$iter = 0;

	while ($iter < $max_iter && squaredNorm($z) <= 4) {
		$z = addComplexSquared(absComplex($z), $c);
		$iter++;
	}
	return ($iter);
}

// $fractalSelector = [
// 	'mandelbrot' => 'mandelbrotComputeIteration',
// 	'julia' => 'juliaComputeIteration',
// 	'julia' => 'juliaComputeIteration',
// ];

function iterToRed($iter, $max_iter) {
	$t = $iter / $max_iter;
	return (9.0 * (1.0 - $t) * $t * $t * $t * 255);
}

function iterToGreen($iter, $max_iter) {
	$t = $iter / $max_iter;
	return (15.0 * (1.0 - $t) * (1.0 - $t) * $t * $t * 255);
}

function iterToBlue($iter, $max_iter) {
	$t = $iter / $max_iter;
	return (8.5 * (1.0 - $t) * (1.0 - $t) * (1.0 - $t) * $t * 255);
}

function displayCell($red, $green, $blue) {
	echo "<td style=\"background-color: rgb({$red}, {$green}, {$blue});\"></td>";
}

// echo "Hello World!";

if ($_POST['heigth']) {
	$heigth = $_POST['heigth'];
} else if ($_GET['heigth']) {
	$heigth = $_GET['heigth'];
} else {
	$heigth = 100.0;
}

if ($_POST['width']) {
	$width = $_POST['width'];
} else if ($_GET['width']) {
	$width = $_GET['width'];
} else {
	$width = 200.0;
}

if ($_POST['iteration']) {
	$max_iter = $_POST['iteration'];
} else if ($_GET['iteration']) {
	$max_iter = $_GET['iteration'];
} else {
	$max_iter = 50.0;
}


$fractalCompute = 'mandelbrotComputeIteration';

if ($_POST['fractal']) {
	$fractalCompute = $_POST['fractal'];
} else if ($_GET['fractal']) {
	$fractalCompute = $_GET['fractal'];
}

// $max_iter = 50;

$camera_pos = array(
	'x' => -0.5,
	'y' => 0.0,
);

if ($_POST['cam_x']) {
	$camera_pos['x'] = $_POST['cam_x'];
} else if ($_GET['cam_x']) {
	$camera_pos['x'] = $_GET['cam_x'];
}

if ($_POST['cam_y']) {
	$camera_pos['y'] = $_POST['cam_y'];
} else if ($_GET['cam_y']) {
	$camera_pos['y'] = $_GET['cam_y'];
}

if ($_POST['zoom']) {
	$camera_zoom = $_POST['zoom'];
} else if ($_GET['zoom']) {
	$camera_zoom = $_GET['zoom'];
} else {
	$camera_zoom = 0.75;
}

echo "<!DOCTYPE html>
<html>
<head>
        <title>Test PHP</title>
        <style>
                html, body {
                        height: 100%;
                        margin: 0;
                }

                body {
                        font-family: \"Noto Sans Mono\", sans-serif;
                        padding: 20px;
                        display: flex;
                        justify-content: center;
                        align-items: center;
                }

                table {
                        border-collapse: collapse;
                }

                td {
                        border: none;
                }
        </style>
</head>
<body>
        <table>\n";

for ($y=0; $y < $heigth; $y++) {
        echo "<tr>";
        $scale = 1 / max($width, $heigth) / $camera_zoom;
        for ($x=0; $x < $width; $x++) {
                $z = array(
                        'real' => 0.285,
                        'imag' => 0.01,
                );
                $c = array(
                        'real' => ($x - $width/2.0) * $scale + $camera_pos['x'],
                        'imag' => ($y - $heigth/2.0) * $scale + $camera_pos['y'],
                );
                // print_r ($z);
                // print_r ($c);
                $iter = $fractalCompute($z, $c, $max_iter);
                // echo $iter;
                $red = iterToRed($iter, $max_iter);
                $green = iterToGreen($iter, $max_iter);
                $blue = iterToBlue($iter, $max_iter);
                displayCell($red, $green, $blue);
        }
        echo "</tr>\n";
}

echo "</table>
</body>
</html>";

?>
