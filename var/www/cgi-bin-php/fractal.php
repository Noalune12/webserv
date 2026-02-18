<?php declare(strict_types=1);

function iterToRed(int $iter, int $max_iter): int {
	$t = $iter / $max_iter;
	return (int)(9.0 * (1.0 - $t) * $t * $t * $t * 255);
}

function iterToGreen(int $iter, int $max_iter): int {
	$t = $iter / $max_iter;
	return (int)(15.0 * (1.0 - $t) * (1.0 - $t) * $t * $t * 255);
}

function iterToBlue(int $iter, int $max_iter): int {
	$t = $iter / $max_iter;
	return (int)(8.5 * (1.0 - $t) * (1.0 - $t) * (1.0 - $t) * $t * 255);
}

function getParam(string $key, float $default): float {
	if (!empty($_POST[$key])) return (float)$_POST[$key];
	if (!empty($_GET[$key]))  return (float)$_GET[$key];
	return $default;
}

function getStrParam(string $key, string $default): string {
	if (!empty($_POST[$key])) return (string)$_POST[$key];
	if (!empty($_GET[$key]))  return (string)$_GET[$key];
	return $default;
}

$width       = (int)getParam('width',     200.0);
$height      = (int)getParam('height',    100.0);
$max_iter    = (int)getParam('iteration',  50.0);
$camera_x    =      getParam('cam_x',      -0.5);
$camera_y    =      getParam('cam_y',       0.0);
$camera_zoom =      getParam('zoom',        0.75);

$fractal = getStrParam('fractal', 'mandelbrot');

$width    = max(1, min($width,  3840));
$height   = max(1, min($height, 2160));
$max_iter = max(1, min($max_iter, 500));


header('Content-Type: image/png');

$img = imagecreatetruecolor($width, $height);
if ($img === false) {
	http_response_code(500);
	exit;
}

$scale = 1.0 / max($width, $height) / $camera_zoom;

for ($y = 0; $y < $height; $y++) {
	$ci_base = ($y - $height / 2.0) * $scale + $camera_y;

	for ($x = 0; $x < $width; $x++) {
		$cr = ($x - $width / 2.0) * $scale + $camera_x;
		$ci = $ci_base;

		$iter = 0;

		if ($fractal === 'julia') {
			$zr = $cr;
			$zi = $ci;
			$cr = 0.285;
			$ci = 0.01;
			while ($iter < $max_iter && ($zr * $zr + $zi * $zi) <= 4.0) {
				$tmp = $zr * $zr - $zi * $zi + $cr;
				$zi  = 2.0 * $zr * $zi + $ci;
				$zr  = $tmp;
				$iter++;
			}
		} elseif ($fractal === 'burningship') {
			$zr = 0.0;
			$zi = 0.0;
			while ($iter < $max_iter && ($zr * $zr + $zi * $zi) <= 4.0) {
				$absr = $zr < 0.0 ? -$zr : $zr;
				$absi = $zi < 0.0 ? -$zi : $zi;
				$tmp  = $absr * $absr - $absi * $absi + $cr;
				$zi   = 2.0 * $absr * $absi + $ci;
				$zr   = $tmp;
				$iter++;
			}
		} else {
			$zr = 0.0;
			$zi = 0.0;
			while ($iter < $max_iter && ($zr * $zr + $zi * $zi) <= 4.0) {
				$tmp = $zr * $zr - $zi * $zi + $cr;
				$zi  = 2.0 * $zr * $zi + $ci;
				$zr  = $tmp;
				$iter++;
			}
		}

		$r = iterToRed($iter, $max_iter);
		$g = iterToGreen($iter, $max_iter);
		$b = iterToBlue($iter, $max_iter);

		imagesetpixel($img, $x, $y, ($r << 16) | ($g << 8) | $b);
	}
}

imagepng($img);
imagedestroy($img);
