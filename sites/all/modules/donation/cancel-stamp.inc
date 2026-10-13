<?php
/**
 * Cancelled receipts.
 *
 * Cancelling a donation asks for a reason, which is prepended to d_comment as
 * "Cancelled reason: <reason> | <old comment>". The receipt PDF in S3 is never
 * changed: when a cancelled receipt is opened, a copy is stamped on the fly with
 * a CANCELLED watermark centred on each receipt copy on the page, plus the
 * cancelled reason in the top margin (only when a reason was recorded).
 */

define('DONATION_CANCEL_PREFIX', 'Cancelled reason: ');
define('DONATION_CANCEL_REASON_MAX', 150);

/**
 * Normalise a typed cancelled reason: one line, no "|" (it separates the reason
 * from the old comment), no tags, and no 4-byte characters (d_comment is utf8mb3).
 */
function donation_cancel_clean_reason($reason)
{
	$reason = preg_replace('/[\x{10000}-\x{10FFFF}]/u', '', (string)$reason);
	$reason = str_replace(array('|', '<', '>'), array('/', '', ''), $reason);
	return trim(preg_replace('/\s+/u', ' ', $reason));
}

/**
 * The comment stored on cancel: "Cancelled reason: <reason> | <old comment>",
 * cut to the 255-character column. The full old comment stays in dh_log (the
 * dh_don_au trigger logs every comment change).
 */
function donation_cancel_comment($reason, $old_comment)
{
	$comment = DONATION_CANCEL_PREFIX.$reason;
	if (trim($old_comment) != '')
		$comment .= ' | '.$old_comment;
	if (drupal_strlen($comment) > 255)
		$comment = drupal_substr($comment, 0, 252).'...';
	return $comment;
}

/**
 * The cancelled reason recorded in a comment, or '' for donations cancelled
 * before reasons were recorded.
 */
function donation_cancel_reason($comment)
{
	if (strpos($comment, DONATION_CANCEL_PREFIX) !== 0)
		return '';
	$reason = substr($comment, strlen(DONATION_CANCEL_PREFIX));
	$pos = strpos($reason, ' | ');
	if ($pos !== FALSE)
		$reason = substr($reason, 0, $pos);
	return trim($reason);
}

/**
 * Words with their positions (pt) for every page of a PDF, via pdftotext -bbox.
 */
function donation_cancel_pdf_words($file)
{
	$xml = shell_exec('/usr/bin/pdftotext -bbox '.escapeshellarg($file).' - 2>/dev/null');
	$pages = array();
	if (!$xml || !preg_match_all('/<page width="([\d.]+)" height="([\d.]+)">(.*?)<\/page>/s', $xml, $pm, PREG_SET_ORDER))
		return $pages;
	foreach ($pm as $p)
	{
		$words = array();
		preg_match_all('/<word xMin="([\d.]+)" yMin="([\d.]+)" xMax="([\d.]+)" yMax="([\d.]+)">(.*?)<\/word>/', $p[3], $wm, PREG_SET_ORDER);
		foreach ($wm as $w)
			$words[] = array('x1' => (float)$w[1], 'y1' => (float)$w[2], 'x2' => (float)$w[3], 'y2' => (float)$w[4], 't' => html_entity_decode($w[5]));
		$pages[] = array('w' => (float)$p[1], 'h' => (float)$p[2], 'words' => $words);
	}
	return $pages;
}

/**
 * One box per receipt copy on a page. Templates print the receipt number once
 * per copy (donor copy, office copy), so the page is split between consecutive
 * receipt numbers at the widest blank band. Without a match the whole text area
 * is one copy; a blank page has none.
 */
function donation_cancel_copy_boxes($page, $needle)
{
	$words = $page['words'];
	if (!$words)
		return array();
	usort($words, function ($a, $b) { return $a['y1'] < $b['y1'] ? -1 : ($a['y1'] > $b['y1'] ? 1 : 0); });
	$occ = array();
	if ($needle != '')
		foreach ($words as $w)
			if (strpos($w['t'], $needle) !== FALSE)
				$occ[] = $w;
	$cuts = array();
	for ($i = 0; $i + 1 < count($occ); $i++)
	{
		$best = -1;
		$cut = NULL;
		$maxy = $occ[$i]['y2'];
		foreach ($words as $w)
		{
			if ($w['y1'] < $occ[$i]['y1'] || $w['y1'] > $occ[$i + 1]['y1'])
				continue;
			$gap = $w['y1'] - $maxy;
			if ($gap > $best)
			{
				$best = $gap;
				$cut = $maxy + $gap / 2;
			}
			$maxy = max($maxy, $w['y2']);
		}
		if ($cut !== NULL)
			$cuts[] = $cut;
	}
	$boxes = array();
	foreach ($words as $w)
	{
		$k = 0;
		while ($k < count($cuts) && $w['y1'] > $cuts[$k])
			$k++;
		if (!isset($boxes[$k]))
			$boxes[$k] = array('x1' => $w['x1'], 'y1' => $w['y1'], 'x2' => $w['x2'], 'y2' => $w['y2']);
		$boxes[$k]['x1'] = min($boxes[$k]['x1'], $w['x1']);
		$boxes[$k]['y1'] = min($boxes[$k]['y1'], $w['y1']);
		$boxes[$k]['x2'] = max($boxes[$k]['x2'], $w['x2']);
		$boxes[$k]['y2'] = max($boxes[$k]['y2'], $w['y2']);
	}
	return array_values($boxes);
}

/**
 * Stamp a receipt PDF as cancelled. Returns the stamped PDF bytes, or FALSE if
 * anything fails (callers must then not serve the unstamped receipt).
 */
function donation_cancel_stamp_pdf($pdf_data, $receipt_no, $reason)
{
	$tmp_dir = is_writable('/var/dana') ? '/var/dana' : file_directory_temp();
	$base = tempnam($tmp_dir, 'CNCL');
	if (!$base)
		return FALSE;
	$in = $base.'-in.pdf';
	$stamp_html = $base.'-stamp.html';
	$stamp_pdf = $base.'-stamp.pdf';
	$out = $base.'-out.pdf';
	$ret = FALSE;
	if (file_put_contents($in, $pdf_data))
	{
		$pages = donation_cancel_pdf_words($in);
		if ($pages)
		{
			// Match the last part of the receipt number: prefixes may contain spaces ("DK ER0000002").
			$parts = preg_split('/\s+/', trim($receipt_no));
			$needle = end($parts);
			file_put_contents($stamp_html, donation_cancel_stamp_html($pages, $needle, $reason));
			$pt = 25.4 / 72;
			$cmd = sprintf('/usr/local/bin/wkhtmltopdf -q --disable-smart-shrinking --page-width %.2fmm --page-height %.2fmm -T 0 -B 0 -L 0 -R 0 --no-background %s %s 2>&1',
				$pages[0]['w'] * $pt, $pages[0]['h'] * $pt, escapeshellarg($stamp_html), escapeshellarg($stamp_pdf));
			exec($cmd, $output, $rc);
			if (is_file($stamp_pdf) && filesize($stamp_pdf) > 0)
			{
				exec('/usr/bin/pdftk '.escapeshellarg($in).' multistamp '.escapeshellarg($stamp_pdf).' output '.escapeshellarg($out).' 2>&1', $output, $rc);
				if ($rc == 0 && is_file($out) && filesize($out) > 0)
					$ret = file_get_contents($out);
			}
			if ($ret === FALSE)
				watchdog('CancelStamp', 'Stamping failed for receipt @r: @o', array('@r' => $receipt_no, '@o' => implode(' ', $output)), WATCHDOG_ERROR);
		}
	}
	foreach (array($base, $in, $stamp_html, $stamp_pdf, $out) as $f)
		if (is_file($f))
			unlink($f);
	return $ret;
}

/**
 * The overlay page(s): a CANCELLED watermark centred on each receipt copy, and
 * the cancelled reason across the top margin of the first page.
 */
function donation_cancel_stamp_html($pages, $needle, $reason)
{
	$pt = 25.4 / 72; // pt -> mm
	$html = '<!doctype html><html><head><meta charset="utf-8"><style>
		html,body{margin:0;padding:0;background:transparent;}
		.pg{position:relative;overflow:hidden;page-break-after:always;}
		.wm{position:absolute;text-align:center;white-space:nowrap;font-family:Arial,Helvetica,sans-serif;font-weight:bold;
			color:rgba(200,0,0,0.28);letter-spacing:0.08em;-webkit-transform:rotate(-25deg);}
		.bn{position:absolute;top:1.2mm;left:8mm;right:8mm;border:0.4mm solid #c00;background:#fff;color:#c00;
			font:bold 11.5pt Arial,"Noto Sans","Noto Sans Devanagari",sans-serif;padding:0.8mm 2.5mm;line-height:1.3;
			max-height:19mm;overflow:hidden;}
		.bn span{font-weight:normal;color:#900;}
		</style></head><body>';
	$any_text = FALSE;
	foreach ($pages as $page)
		$any_text = $any_text || count($page['words']) > 0;
	foreach ($pages as $n => $page)
	{
		// A PDF with no text at all still gets one watermark in the middle of its first page.
		if (!$any_text && $n == 0)
			$page['words'] = array(array('x1' => 0, 'y1' => 0, 'x2' => $page['w'], 'y2' => $page['h'], 't' => ''));
		$html .= sprintf('<div class="pg" style="width:%.2fmm;height:%.2fmm">', $page['w'] * $pt, $page['h'] * $pt - 0.5);
		if ($n == 0 && $reason != '')
			$html .= '<div class="bn">Cancelled reason: <span>'.htmlspecialchars($reason, ENT_QUOTES, 'UTF-8').'</span></div>';
		foreach (donation_cancel_copy_boxes($page, $needle) as $b)
		{
			$bw = ($b['x2'] - $b['x1']) * $pt;
			$bh = ($b['y2'] - $b['y1']) * $pt;
			$cx = ($b['x1'] + $b['x2']) / 2 * $pt;
			$cy = ($b['y1'] + $b['y2']) / 2 * $pt;
			// "CANCELLED" in bold Arial is about 6.9em wide (with spacing) and 0.75em tall; fit it, rotated, inside the copy.
			$c = cos(deg2rad(25));
			$s = sin(deg2rad(25));
			$f = min(($bw * 0.95) / (6.9 * $c + 0.75 * $s), ($bh * 1.35) / (6.9 * $s + 0.75 * $c), 45);
			$w = 7.2 * $f;
			$h = 1.2 * $f;
			$html .= sprintf('<div class="wm" style="left:%.2fmm;top:%.2fmm;width:%.2fmm;height:%.2fmm;line-height:%.2fmm;font-size:%.2fmm">CANCELLED</div>',
				$cx - $w / 2, $cy - $h / 2, $w, $h, $h, $f);
		}
		$html .= '</div>';
	}
	return $html.'</body></html>';
}
