<?php
/**
 * Donor ID rules shared by the uploads (Upload CSV, Upload Donor Master), the same as the DIPI sync
 * (domain-dana students-dipi.inc: dipi_id_clean / dipi_is_pan / dipi_is_aadhaar / dipi_id_from_fields, 2026-10-29)
 * and the ID cleanup of 2026-10-29. Keep the two copies the same (test: upload-work t_id_rules.php).
 * ID types: 1 PAN, 2 Aadhaar, 4 Passport, 5 Voter ID, 6 Driving License, 7 Ration Card, 99 No ID.
 */

/** Spaces (also invisible ones), dots and dashes removed; capital letters. */
function dana_id_clean($no)
{
	return strtoupper(preg_replace('/[\s\x{00A0}\x{200B}-\x{200D}\x{2060}\x{FEFF}.\-]+/u', '', (string) $no));
}

/** A PAN: 5 letters (the 4th one a holder type), 4 digits, 1 letter. */
function dana_is_pan($v)
{
	return (bool) preg_match('/^[A-Z]{3}[PCHFATBLJG][A-Z][0-9]{4}[A-Z]$/', (string) $v);
}

/** An Aadhaar number: 12 digits, the first 2-9, a valid Verhoeff check digit. */
function dana_is_aadhaar($v)
{
	static $d = array(array(0,1,2,3,4,5,6,7,8,9), array(1,2,3,4,0,6,7,8,9,5), array(2,3,4,0,1,7,8,9,5,6), array(3,4,0,1,2,8,9,5,6,7),
		array(4,0,1,2,3,9,5,6,7,8), array(5,9,8,7,6,0,4,3,2,1), array(6,5,9,8,7,1,0,4,3,2), array(7,6,5,9,8,2,1,0,4,3),
		array(8,7,6,5,9,3,2,1,0,4), array(9,8,7,6,5,4,3,2,1,0));
	static $p = array(array(0,1,2,3,4,5,6,7,8,9), array(1,5,7,6,2,8,3,0,9,4), array(5,8,0,3,7,9,6,1,4,2), array(8,9,1,6,0,4,3,5,2,7),
		array(9,4,5,3,1,2,6,8,7,0), array(4,2,8,6,5,7,3,9,0,1), array(2,7,9,3,8,0,6,4,1,5), array(7,0,4,6,9,1,3,2,5,8));
	$v = (string) $v;
	if (!preg_match('/^[2-9][0-9]{11}$/', $v))
		return FALSE;
	$c = 0;
	$r = strrev($v);
	for ($i = 0; $i < 12; $i++)
		$c = $d[$c][$p[$i % 8][(int) $r[$i]]];
	return $c === 0;
}

/**
 * The ID type and the cleaned number to store. For an Indian donor a clear PAN becomes type PAN and a valid Aadhaar
 * becomes type Aadhaar, whatever type was chosen (a PAN wins); otherwise the chosen type stays.
 */
function dana_id_retype($type, $no, $india)
{
	$n = dana_id_clean($no);
	$t = (int) $type;
	if ($n !== '' && $india)
	{
		if (dana_is_pan($n))
			$t = 1;
		elseif (dana_is_aadhaar($n))
			$t = 2;
	}
	return array($t, $n);
}

/** ID type names (capital letters) => code. */
function dana_id_type_names()
{
	return array('PAN' => 1, 'AADHAAR' => 2, 'PASSPORT' => 4, 'VOTER ID' => 5, 'DRIVING LICENSE' => 6, 'RATION CARD' => 7, 'NO ID' => 99);
}

/** What is wrong with this cleaned number for this type, in plain English; '' when it is fine. */
function dana_id_problem($type, $no)
{
	$t = (int) $type;
	$no = (string) $no;
	if ($t === 1)
	{
		if (!preg_match('/^[A-Z]{5}[0-9]{4}[A-Z]$/', $no))
			return 'A PAN has 10 characters: 5 letters, 4 digits, then 1 letter (for example ABCPE1234F).';
		if (!dana_is_pan($no))
			return 'This is not a valid PAN: its 4th letter must be one of P, C, H, F, A, T, B, L, J or G.';
		return '';
	}
	if ($t === 2)
	{
		if (preg_match('/[^0-9]/', $no))
			return 'An Aadhaar number has only digits.';
		if (strlen($no) !== 12)
			return 'An Aadhaar number has 12 digits; this one has ' . strlen($no) . '.';
		if (preg_match('/^[01]/', $no))
			return 'An Aadhaar number never starts with 0 or 1.';
		if (!dana_is_aadhaar($no))
			return 'This is not a valid Aadhaar number; one of the digits may be wrong.';
		return '';
	}
	if (in_array($t, array(4, 5, 6, 7), TRUE))
	{
		if ($no === '')
			return 'The ID number is empty.';
		if (!preg_match('/[0-9]/', $no))
			return 'This ID number has no digits.';
		if (strlen($no) > 20)
			return 'This ID number is too long (more than 20 characters).';
		return '';
	}
	if ($t === 99)
		return $no === '' ? '' : 'The ID Type is No ID, but an ID number is given.';
	return 'Unknown ID Type.';
}
