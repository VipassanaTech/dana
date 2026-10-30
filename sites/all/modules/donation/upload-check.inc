<?php
/**
 * The two uploads (user decision 2026-10-30): Upload CSV (course and student list) and Upload Donor Master (bulk ID
 * correction). A file is first CHECKED as a whole and nothing is written; the person sees every problem. Only a file
 * without problems can be used ("Use this file"), and it is checked again just before it is used.
 * ID rules: id-rules.inc (the same as the DIPI sync).
 * Messages in 'errors' and 'notes' are safe HTML: built with t() and @placeholders (file and database values escaped),
 * so they are shown as they are and never escaped a second time.
 */

/** All rows of a CSV file (a UTF-8 byte order mark on line 1 removed), or NULL when it cannot be opened. */
function dana_upload_read_csv($uri)
{
	if (($h = @fopen($uri, 'r')) === FALSE)
		return NULL;
	$rows = array();
	while (($r = fgetcsv($h, 0, ',')) !== FALSE)
		$rows[] = $r;
	fclose($h);
	if (isset($rows[0][0]))
		$rows[0][0] = preg_replace('/^\xEF\xBB\xBF/', '', $rows[0][0]);
	return $rows;
}

function dana_upload_row_is_blank(array $r)
{
	return trim(implode('', $r)) === '';
}

/** "Line 5 (Asha Rao)" */
function dana_upload_where($line, $name)
{
	$name = trim((string) $name);
	return $name === '' ? t('Line @l', array('@l' => $line)) : t('Line @l (@n)', array('@l' => $line, '@n' => $name));
}

/** The ID type name for a code, e.g. 2 -> Aadhaar. */
function dana_id_type_label($code)
{
	$labels = array(1 => 'PAN', 2 => 'Aadhaar', 4 => 'Passport', 5 => 'Voter ID', 6 => 'Driving License', 7 => 'Ration Card', 99 => 'No ID');
	return isset($labels[(int) $code]) ? $labels[(int) $code] : 'ID type ' . (int) $code;
}

/* ====================================================================== Upload CSV (courses and students) */

function donation_csv_columns()
{
	return array('Course Type', 'Start', 'End', 'Name', 'Gender', 'Address', 'Pincode', 'Contact', 'Email', 'Country', 'State', 'City', 'ID No', 'ID Type');
}

/** YYYY-MM-DD (also YYYY/MM/DD) as YYYY-MM-DD, or '' when it is not a real date. */
function donation_csv_date($v)
{
	if (!preg_match('/^(\d{4})[-\/](\d{1,2})[-\/](\d{1,2})$/', trim((string) $v), $m) || !checkdate((int) $m[2], (int) $m[3], (int) $m[1]))
		return '';
	return sprintf('%04d-%02d-%02d', $m[1], $m[2], $m[3]);
}

/** Course names compared without case, spaces and dashes: "3-Day Course" = "3 day course". */
function donation_csv_course_key($type)
{
	return preg_replace('/[^a-z0-9]+/', '', drupal_strtolower((string) $type));
}

/**
 * Checks a course/student CSV file without writing anything.
 * Returns array('errors' => list of messages, 'notes' => list, 'courses' => key => course, 'students' => list, 'new', 'update').
 */
function donation_csv_check($uri)
{
	$res = array('errors' => array(), 'notes' => array(), 'courses' => array(), 'students' => array(), 'new' => 0, 'update' => 0);
	if (!donation_versioning_ready())
	{
		$res['errors'][] = t('Donor versioning is not completely installed on this site.');
		return $res;
	}
	$rows = dana_upload_read_csv($uri);
	if ($rows === NULL || !$rows)
	{
		$res['errors'][] = $rows === NULL ? t('The file could not be opened.') : t('The file is empty.');
		return $res;
	}
	$head = array_shift($rows);
	if (count($head) != 14)
		$res['errors'][] = t('Line 1 must have the 14 column names (@c). It has @n columns.',
			array('@c' => implode(', ', donation_csv_columns()), '@n' => count($head)));
	$types = dana_id_type_names();
	$seen = array();
	$course_end = array();
	$course_line = array();
	foreach ($rows as $i => $r)
	{
		$line = $i + 2;
		if (dana_upload_row_is_blank($r))
			continue;
		$at = dana_upload_where($line, isset($r[3]) ? $r[3] : '');
		if (count($r) != 14)
		{
			$res['errors'][] = $at . ': ' . t('has @n columns; it needs 14.', array('@n' => count($r)));
			continue;
		}
		list($ctype, $cstart, $cend, $name, $gender, $addr, $pin, $contact, $email, $country, $state, $city, $idno, $idtype) = array_map('trim', $r);
		$err = array();
		if ($ctype === '')
			$err[] = t('Course Type is empty.');
		$s = donation_csv_date($cstart);
		$e = donation_csv_date($cend);
		if ($s === '')
			$err[] = t('Start "@v" is not a date like 2026-10-17.', array('@v' => $cstart));
		if ($e === '')
			$err[] = t('End "@v" is not a date like 2026-10-28.', array('@v' => $cend));
		if ($s !== '' && $e !== '' && $e < $s)
			$err[] = t('End is before Start.');
		if ($name === '')
			$err[] = t('Name is empty.');
		$g = drupal_strtolower($gender);
		if (!in_array($g, array('male', 'female'), TRUE))
			$err[] = t('Gender "@v" must be male or female.', array('@v' => $gender));
		if ($email !== '' && !valid_email_address($email))
			$err[] = t('Email "@v" is not a valid email address.', array('@v' => $email));
		$co = $country === '' ? '' : get_country($country);
		$st = '';
		if ($country === '')
			$err[] = t('Country is empty.');
		elseif ($co === '')
			$err[] = t('Country "@v" is not in Dana\'s list of countries. Write the full name, for example India.', array('@v' => $country));
		elseif (db_query_range('SELECT 1 FROM dh_state WHERE s_country = :c', 0, 1, array(':c' => $co))->fetchField())
		{
			$st = $state === '' ? '' : get_state($state, $co);
			if ($state === '')
				$err[] = t('State is empty.');
			elseif ($st === '')
				$err[] = t('State "@v" is not in Dana\'s list for @c. Write the full name, for example Maharashtra.', array('@v' => $state, '@c' => $country));
		}
		$tkey = strtoupper(preg_replace('/\s+/', ' ', $idtype));
		$t2 = 0;
		$n2 = '';
		if (!isset($types[$tkey]))
			$err[] = t('ID Type "@v" is not known. Use PAN, Aadhaar, Passport, Voter ID, Driving License, Ration Card or No ID.', array('@v' => $idtype));
		else
		{
			list($t2, $n2) = dana_id_retype($types[$tkey], $idno, $co === 'IN');
			$p = dana_id_problem($t2, $n2);
			if ($p !== '')
				$err[] = t('ID No "@v": @p', array('@v' => $idno, '@p' => $p));
			elseif ($t2 !== $types[$tkey])
				$res['notes'][] = $at . ': ' . t('the ID number is a @new number, so it will be saved as @new (the file says @old).',
					array('@new' => dana_id_type_label($t2), '@old' => $idtype));
		}
		$k = drupal_strtolower($name) . '|' . drupal_strtolower($email) . '|' . $contact;
		if (isset($seen[$k]))
			$err[] = t('The same student (name, contact and email) is also on line @l.', array('@l' => $seen[$k]));
		else
			$seen[$k] = $line;
		$ck = '';
		if ($ctype !== '' && $s !== '' && $e !== '')
		{
			$ck = $ctype . '||' . $s;
			if (isset($course_end[$ck]) && $course_end[$ck] !== $e)
				$err[] = t('Course @t starting @s has End @e on line @l, but @x here.',
					array('@t' => $ctype, '@s' => $s, '@e' => $course_end[$ck], '@l' => $course_line[$ck], '@x' => $e));
			elseif (!isset($course_end[$ck]))
			{
				$course_end[$ck] = $e;
				$course_line[$ck] = $line;
			}
		}
		if ($err)
		{
			foreach ($err as $m)
				$res['errors'][] = $at . ': ' . $m;
			continue;
		}
		$res['students'][] = array('line' => $line, 'course' => $ck, 'n' => $name, 'g' => $g, 'a' => $addr, 'p' => $pin, 'c' => $contact,
			'e' => $email, 'co' => $co, 'st' => $st, 'ci' => $city, 'it' => $t2, 'i' => $n2);
	}
	// Courses: existing, or new; a new name that differs from an existing course of the same day only in spaces/dashes/case
	// is almost always the same course written differently (both spellings exist at centres today).
	foreach ($course_end as $ck => $end)
	{
		list($ct, $cs) = explode('||', $ck, 2);
		$cid = (int) db_query('SELECT c_id FROM dh_course WHERE c_start = :s AND c_type = :t', array(':s' => $cs, ':t' => $ct))->fetchField();
		if (!$cid)
			foreach (db_query('SELECT c_type FROM dh_course WHERE c_start = :s', array(':s' => $cs))->fetchCol() as $other)
				if (donation_csv_course_key($other) === donation_csv_course_key($ct))
					$res['errors'][] = t('Line @l: course "@t" starting @s is not in Dana, but "@o" starting the same day is. Write the course name exactly as "@o".',
						array('@l' => $course_line[$ck], '@t' => $ct, '@s' => $cs, '@o' => $other));
		$res['courses'][$ck] = array('type' => $ct, 'start' => $cs, 'end' => $end, 'id' => $cid);
	}
	foreach ($res['students'] as $j => $st)
	{
		$id = (int) db_query('SELECT d_id FROM dh_donor WHERE d_name = :n AND d_email = :e AND d_contact = :c AND d_deleted = 0',
			array(':n' => $st['n'], ':e' => $st['e'], ':c' => $st['c']))->fetchField();
		$res['students'][$j]['id'] = $id;
		$id ? $res['update']++ : $res['new']++;
	}
	if (!$res['errors'] && !$res['students'])
		$res['errors'][] = t('The file has no students. Under the line with the column names, add one line for each student.');
	return $res;
}

/** Writes a checked course/student file (donation_csv_check() found no errors). Returns the message for the person. */
function donation_csv_apply(array $res)
{
	global $user;
	$new_courses = 0;
	foreach ($res['courses'] as $ck => $c)
	{
		$cid = (int) db_query('SELECT c_id FROM dh_course WHERE c_start = :s AND c_type = :t', array(':s' => $c['start'], ':t' => $c['type']))->fetchField();
		if (!$cid)
		{
			$cid = (int) db_insert('dh_course')->fields(array('c_start' => $c['start'], 'c_end' => $c['end'], 'c_type' => $c['type']))->execute();
			$new_courses++;
		}
		$res['courses'][$ck]['id'] = $cid;
	}
	$added = 0;
	$updated = 0;
	$failed = array();
	foreach ($res['students'] as $s)
	{
		$cid = isset($res['courses'][$s['course']]) ? (int) $res['courses'][$s['course']]['id'] : 0;
		$txn = db_transaction();
		try
		{
			$id = (int) db_query('SELECT d_id FROM dh_donor WHERE d_name = :n AND d_email = :e AND d_contact = :c AND d_deleted = 0',
				array(':n' => $s['n'], ':e' => $s['e'], ':c' => $s['c']))->fetchField();
			if ($id)
			{
				$old = db_query('SELECT * FROM dh_donor WHERE d_id = :d FOR UPDATE', array(':d' => $id))->fetchAssoc();
				$d = array('d_name' => mb_convert_encoding($s['n'], 'UTF-8'), 'd_gender' => $s['g'], 'd_address' => mb_convert_encoding($s['a'], 'UTF-8'),
					'd_pincode' => $s['p'], 'd_contact' => $s['c'], 'd_email' => $s['e'], 'd_course' => $cid);
				if ($s['co'] !== '' && $s['st'] !== '')
				{
					$old_city = (string) db_query('SELECT c_city FROM dh_city WHERE c_id = :i', array(':i' => (int) $old['d_city']))->fetchField();
					$same_place = ($s['co'] === (string) $old['d_country'] && $s['st'] === (string) $old['d_state']
						&& drupal_strtolower($s['ci']) === drupal_strtolower(trim($old_city)));
					$d['d_country'] = $s['co'];
					$d['d_state'] = $s['st'];
					$d['d_city'] = $same_place ? (int) $old['d_city'] : get_city($s['ci'], $s['st'], $s['co']);
				}
				// a new ID only when one is given and it differs; an existing ID is never removed
				if ($s['i'] !== '' && donor_id_key($s['it'], $s['i'], TRUE) !== donor_id_key($old['d_id_type'], $old['d_id_no'], TRUE))
				{
					$d['d_id_type'] = (int) $s['it'];
					$d['d_id_no'] = $s['i'];
				}
				$f = array();
				foreach ($d as $k => $val)
					if ((string) $val !== (string) $old[$k])
						$f[$k] = $val;
				if ($f)
				{
					$f['d_updatedby'] = (int) $user->uid;
					db_update('dh_donor')->fields($f)->expression('d_updated', 'NOW()')->condition('d_id', $id)->execute();
				}
				$updated++;
			}
			else
			{
				db_insert('dh_donor')->fields(array('d_name' => mb_convert_encoding($s['n'], 'UTF-8'), 'd_gender' => $s['g'],
					'd_address' => mb_convert_encoding($s['a'], 'UTF-8'), 'd_pincode' => $s['p'], 'd_contact' => $s['c'], 'd_email' => $s['e'],
					'd_country' => $s['co'], 'd_state' => $s['st'], 'd_city' => get_city($s['ci'], $s['st'], $s['co']), 'd_course' => $cid,
					'd_id_type' => (int) $s['it'], 'd_id_no' => $s['i'], 'd_occupation' => '', 'd_created' => date('Y-m-d H:i:s'),
					'd_createdby' => $user->uid))->execute();
				$added++;
			}
		}
		catch (Throwable $e)
		{
			$txn->rollback();
			watchdog('donation', 'Save rolled back: @m (@f:@l)', array('@m' => $e->getMessage(), '@f' => $e->getFile(), '@l' => $e->getLine()), WATCHDOG_ERROR);
			$failed[] = dana_upload_where($s['line'], $s['n']);
		}
		unset($txn);
	}
	$msg = t('File used: @a new students added, @u students updated, @c new courses.', array('@a' => $added, '@u' => $updated, '@c' => $new_courses));
	if ($failed)
		$msg .= ' ' . t('Could not be saved (@n): !l', array('@n' => count($failed), '!l' => implode('; ', $failed)));
	return $msg;
}

/** The blank template for Upload CSV: the column names and two made-up example lines. */
function donation_csv_template()
{
	drupal_add_http_header('Content-Type', 'text/csv; charset=utf-8');
	drupal_add_http_header('Content-Disposition', 'attachment; filename="Dana-student-list-template.csv"');
	$out = fopen('php://output', 'w');
	foreach (donation_csv_template_rows() as $row)
		fputcsv($out, $row);
	fclose($out);
	drupal_exit();
}

/** The lines of the template: the column names and two made-up students. */
function donation_csv_template_rows()
{
	return array(donation_csv_columns(),
		array('10-Day Course', '2026-11-05', '2026-11-16', 'Sample Student One', 'male', '12 Sample Road', '400001', '9000000001',
			'sample.one@example.org', 'India', 'Maharashtra', 'Mumbai', 'ABCPE1234F', 'PAN'),
		array('10-Day Course', '2026-11-05', '2026-11-16', 'Sample Student Two', 'female', '5 Sample Lane', '400002', '9000000002',
			'', 'India', 'Maharashtra', 'Mumbai', '', 'No ID'));
}

/* ====================================================================== Upload Donor Master (bulk ID correction) */

/** TRUE when a donor's country counts as India for the ID rules (as the 2026-10-29 cleanup: IN, or blank / India / 0). */
function dana_donor_is_india($country)
{
	return in_array((string) $country, array('IN', '', 'India', '0'), TRUE);
}

/**
 * Checks a Donor Master file without writing anything.
 * Returns array('errors', 'notes', 'plan' => rows to change, 'unedited', 'unchanged', 'signed').
 */
function donor_master_check($uri)
{
	$res = array('errors' => array(), 'notes' => array(), 'plan' => array(), 'unedited' => 0, 'unchanged' => 0, 'signed' => FALSE);
	if (!donation_versioning_ready())
	{
		$res['errors'][] = t('Donor versioning is not completely installed on this site.');
		return $res;
	}
	$lines = dana_upload_read_csv($uri);
	if ($lines === NULL || !$lines)
	{
		$res['errors'][] = $lines === NULL ? t('The file could not be opened.') : t('The file is empty.');
		return $res;
	}
	$first = 2;
	if (isset($lines[0][0]) && trim($lines[0][0]) === '#Dana Donor Master')
	{
		$meta = array();
		foreach (array_slice($lines[0], 1) as $cell)
		{
			$kv = explode('=', (string) $cell, 2);
			if (count($kv) == 2)
				$meta[trim($kv[0])] = trim($kv[1]);
		}
		$site = donation_site_key();
		if (!isset($meta['site']) || $meta['site'] !== $site)
		{
			$res['errors'][] = t('This file was downloaded from @o, not from this centre (@s). Download the Donor Master from this centre.',
				array('@o' => isset($meta['site']) ? $meta['site'] : '?', '@s' => $site));
			return $res;
		}
		if (!isset($meta['exported'], $meta['sig']) || !hash_equals(donor_master_sig($site, $meta['exported']), $meta['sig']))
		{
			$res['errors'][] = t('The first line of the file was changed. Download the Donor Master again and do not change its first line.');
			return $res;
		}
		$res['signed'] = TRUE;
		array_shift($lines);
		$first = 3;
	}
	array_shift($lines);                                           // column names
	$types = dana_id_type_names();
	foreach ($lines as $n => $d)
	{
		if (dana_upload_row_is_blank($d))
			continue;
		$line = $n + $first;
		$name = trim(isset($d[0]) ? $d[0] : '');
		$at = dana_upload_where($line, $name);
		$need = $res['signed'] ? 7 : 5;
		if (count($d) < $need)
		{
			$res['errors'][] = $at . ': ' . t('has @n columns; it needs @m.', array('@n' => count($d), '@m' => $need));
			continue;
		}
		// which donor
		if ($res['signed'] && ctype_digit(trim($d[5])))
		{
			$id = (int) $d[5];
			$live = db_query('SELECT d_id, d_name FROM dh_donor WHERE d_id = :id AND d_deleted = 0', array(':id' => $id))->fetchAssoc();
			if (!$live)
			{
				$res['errors'][] = $at . ': ' . t('there is no donor with Donor ID @i.', array('@i' => $id));
				continue;
			}
			if (!donor_master_identity_ok($id, $name, $live['d_name']))
			{
				$res['errors'][] = $at . ': ' . t('Donor ID @i belongs to "@o", not to this name. Do not change the Full Name or Donor ID columns.',
					array('@i' => $id, '@o' => $live['d_name']));
				continue;
			}
		}
		else
		{
			$ids = db_query('SELECT d_id FROM dh_donor WHERE d_name = :n AND d_contact = :c AND d_email = :e AND d_deleted = 0',
				array(':n' => $d[0], ':c' => $d[1], ':e' => $d[2]))->fetchCol();
			if (count($ids) != 1)
			{
				$res['errors'][] = $at . ': ' . (count($ids) ? t('more than one donor has this name, contact and email.')
					: t('no donor has this name, contact and email.'));
				continue;
			}
			$id = (int) $ids[0];
		}
		// which ID type
		$label = strtoupper(preg_replace('/\s+/', ' ', trim($d[3])));
		$unknown = FALSE;
		if (preg_match('/^UNKNOWN\s*\((\d+)\)$/', $label, $m))
		{
			$type = (int) $m[1];
			$unknown = TRUE;
		}
		elseif (isset($types[$label]))
			$type = $types[$label];
		else
		{
			$res['errors'][] = $at . ': ' . t('ID Type "@v" is not known. Use PAN, Aadhaar, Passport, Voter ID, Driving License, Ration Card or No ID.', array('@v' => $d[3]));
			continue;
		}
		$no = dana_id_clean($d[4]);
		$check = ($res['signed'] && isset($d[6])) ? trim($d[6]) : '';
		if ($check !== '' && hash_equals(donor_master_row_check($id, $type, $no), $check))
		{
			$res['unedited']++;                                    // not changed in the file
			continue;
		}
		if ($unknown)
		{
			$res['errors'][] = $at . ': ' . t('ID Type "@v" is not known. Use PAN, Aadhaar, Passport, Voter ID, Driving License, Ration Card or No ID.', array('@v' => $d[3]));
			continue;
		}
		$now = db_query('SELECT d_id_type, d_id_no, d_country FROM dh_donor WHERE d_id = :id', array(':id' => $id))->fetchAssoc();
		list($t2, $n2) = dana_id_retype($type, $no, dana_donor_is_india($now['d_country']));
		$p = dana_id_problem($t2, $n2);
		if ($p !== '')
		{
			$res['errors'][] = $at . ': ' . t('ID No "@v": @p', array('@v' => $d[4], '@p' => $p));
			continue;
		}
		if (donor_id_key($now['d_id_type'], $now['d_id_no'], TRUE) === donor_id_key($t2, $n2, TRUE))
		{
			$res['unchanged']++;                                   // already like this in Dana
			continue;
		}
		if ($check !== '' && !hash_equals(donor_master_row_check($id, $now['d_id_type'], $now['d_id_no']), $check))
		{
			$res['errors'][] = $at . ': ' . t('someone changed this donor\'s ID in Dana after the file was downloaded (it is now @t @n). Download the Donor Master again.',
				array('@t' => dana_id_type_label($now['d_id_type']), '@n' => $now['d_id_no']));
			continue;
		}
		if (donor_id_is_real($now['d_id_type'], $now['d_id_no']) && !donor_id_is_real($t2, $n2))
		{
			$res['errors'][] = $at . ': ' . t('an upload cannot remove an ID (Dana has @t @n). To remove it, use Edit on the Donors page.',
				array('@t' => dana_id_type_label($now['d_id_type']), '@n' => $now['d_id_no']));
			continue;
		}
		if ($t2 !== $type)
			$res['notes'][] = $at . ': ' . t('the ID number is a @new number, so it will be saved as @new (the file says @old).',
				array('@new' => dana_id_type_label($t2), '@old' => $d[3]));
		$res['plan'][] = array('line' => $line, 'at' => $at, 'id' => $id, 'type' => $t2, 'no' => $n2, 'check' => $check,
			'old' => dana_id_type_label($now['d_id_type']) . ' ' . $now['d_id_no']);
	}
	return $res;
}

/** Writes a checked Donor Master file (donor_master_check() found no errors). Returns the message for the person. */
function donor_master_apply(array $res)
{
	global $user;
	$updated = array();
	$skipped = array();
	foreach ($res['plan'] as $p)
	{
		$txn = db_transaction();
		try
		{
			$now = db_query('SELECT d_id_type, d_id_no FROM dh_donor WHERE d_id = :id FOR UPDATE', array(':id' => $p['id']))->fetchAssoc();
			if (donor_id_key($now['d_id_type'], $now['d_id_no'], TRUE) === donor_id_key($p['type'], $p['no'], TRUE))
				continue;
			if ($p['check'] !== '' && !hash_equals(donor_master_row_check($p['id'], $now['d_id_type'], $now['d_id_no']), $p['check']))
			{
				$skipped[] = $p['at'];                             // changed by someone in the last moments
				continue;
			}
			db_update('dh_donor')->fields(array('d_id_type' => $p['type'], 'd_id_no' => $p['no'], 'd_updatedby' => (int) $user->uid))
				->expression('d_updated', 'NOW()')->condition('d_id', $p['id'])->execute();
			$updated[] = $p['id'];
		}
		catch (Throwable $e)
		{
			$txn->rollback();
			watchdog('donation', 'Save rolled back: @m (@f:@l)', array('@m' => $e->getMessage(), '@f' => $e->getFile(), '@l' => $e->getLine()), WATCHDOG_ERROR);
			$skipped[] = $p['at'];
		}
		unset($txn);
	}
	$out = array(format_plural(count($updated), 'File used: the ID of 1 donor was changed.', 'File used: the IDs of @count donors were changed.'));
	if ($skipped)
		$out[] = t('Not changed, because someone changed them at the same time (@n): !l. Download the Donor Master again for these.',
			array('@n' => count($skipped), '!l' => implode('; ', $skipped)));
	if ($updated && donation_in_apr_may())
	{
		$prev = donation_cur_fy() - 1;
		$n = (int) db_query('SELECT COUNT(DISTINCT d_donor) FROM dh_donation WHERE d_fy = :f AND d_donor IN (:ids)',
			array(':f' => $prev, ':ids' => $updated))->fetchField();
		if ($n)
			$out[] = t('@n of these donors have FY @f donations. FY @f keeps the IDs it had when it closed on 31 March, so its Form 10BD does not show these changes yet. Review them on the Income Tax 10BD page (FY @f review list) and apply where needed.',
				array('@n' => $n, '@f' => donation_fy_label($prev)));
	}
	return implode(' ', $out);
}

/* ====================================================================== the two-step pages */

/** Step 2 of both upload pages: what the check found, and the buttons. $kind: 'csv' or 'master'. */
function dana_upload_review(&$form, array $res, $kind)
{
	$items = function (array $list) {
		return theme('item_list', array('items' => $list));            // safe HTML already (see the file comment)
	};
	if ($res['errors'])
	{
		$form['result'] = array('#markup' => '<div class="messages error"><p><strong>'
			. format_plural(count($res['errors']), 'The file was checked and has 1 problem. Nothing was changed.', 'The file was checked and has @count problems. Nothing was changed.')
			. '</strong> ' . t('Correct the file and upload it again.') . '</p>' . $items($res['errors']) . '</div>');
		$form['again'] = array('#type' => 'submit', '#value' => t('Upload a corrected file'), '#submit' => array('dana_upload_restart'),
			'#limit_validation_errors' => array());
		return;
	}
	if ($kind === 'csv')
	{
		$courses = array();
		foreach ($res['courses'] as $c)
			$courses[] = t('@t, @s to @e', array('@t' => $c['type'], '@s' => $c['start'], '@e' => $c['end']))
				. ($c['id'] ? '' : ' (' . t('new course') . ')');
		$summary = '<p>' . t('The file was checked: no problems. Nothing has been changed yet.') . '</p><ul><li>'
			. t('@n new students will be added, @u students already in Dana will be updated.', array('@n' => $res['new'], '@u' => $res['update']))
			. '</li><li>' . t('Courses:') . ' ' . implode('; ', $courses) . '</li></ul>';
		$todo = count($res['students']);
	}
	else
	{
		$changes = array();
		foreach ($res['plan'] as $p)
			$changes[] = $p['at'] . ': ' . check_plain($p['old']) . ' &rarr; ' . check_plain(dana_id_type_label($p['type']) . ' ' . $p['no']);
		$summary = '<p>' . t('The file was checked: no problems. Nothing has been changed yet.') . '</p><p>'
			. t('@c donors will get a new ID. @e lines were not edited and @s already match Dana; they stay as they are.',
				array('@c' => count($res['plan']), '@e' => $res['unedited'], '@s' => $res['unchanged'])) . '</p>'
			. ($changes ? $items($changes) : '');
		$todo = count($res['plan']);
	}
	if ($res['notes'])
		$summary .= '<p>' . t('Dana will also correct these ID types:') . '</p>' . $items($res['notes']);
	$form['result'] = array('#markup' => '<div class="messages status">' . $summary . '</div>');
	if ($todo)
		$form['use'] = array('#type' => 'submit', '#value' => t('Use this file'), '#submit' => array($kind === 'csv' ? 'donation_csv_use' : 'donor_master_use'));
	else
		$form['nothing'] = array('#markup' => '<p>' . t('There is nothing to change.') . '</p>');
	$form['cancel'] = array('#type' => 'submit', '#value' => t('Cancel'), '#submit' => array('dana_upload_restart'), '#limit_validation_errors' => array());
}

/** Step 1 validation of both pages: the uploaded CSV file, kept in the site's private folder. */
function dana_upload_file_validate($form, &$form_state)
{
	$file = file_save_upload('upload', array('file_validate_extensions' => array('csv')));
	if (!$file)
	{
		form_set_error('upload', t('Choose a CSV file first.'));
		return;
	}
	if (!($file = file_move($file, 'private://')))
	{
		form_set_error('upload', t('The file could not be saved. Please try again.'));
		return;
	}
	$file->status = FILE_STATUS_PERMANENT;
	file_save($file);
	$form_state['values']['upload'] = $file;
}

function dana_upload_restart($form, &$form_state)
{
	$form_state['storage'] = array();
	$form_state['rebuild'] = TRUE;
}

/* ====================================================================== one call: check, then use only if clean */

/** Course/student CSV: used only when the whole file has no problems. Returns the message for the person. */
function process_csv($uri)
{
	$res = donation_csv_check($uri);
	if ($res['errors'])
		return format_plural(count($res['errors']), 'The file has 1 problem; nothing was changed: !l', 'The file has @count problems; nothing was changed: !l', array('!l' => implode('; ', $res['errors'])));
	return donation_csv_apply($res);
}

/** Donor Master CSV: used only when the whole file has no problems. Returns the message for the person. */
function process_donor_csv($uri)
{
	$res = donor_master_check($uri);
	if ($res['errors'])
		return format_plural(count($res['errors']), 'The file has 1 problem; nothing was changed: !l', 'The file has @count problems; nothing was changed: !l', array('!l' => implode('; ', $res['errors'])));
	if (!$res['plan'])
		return t('There is nothing to change: @e lines were not edited and @s already match Dana.', array('@e' => $res['unedited'], '@s' => $res['unchanged']));
	return donor_master_apply($res);
}
