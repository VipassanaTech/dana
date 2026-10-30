<?php
/**
 * Donor versioning (revision 3): each past financial year keeps the donor details it had.
 *
 * dh_donor stays the donor's current details. When a donor changes, the BEFORE UPDATE trigger
 * dh_donor_bu_freeze saves the old details once (dh_donor_version, immutable) and links every past
 * financial year that has donations to it (dh_donor_fy: donor + FY -> version). Every donation-scoped
 * read uses COALESCE(year snapshot, current details). This file holds the PHP side: one clock, the
 * runtime guard, the comparators, the signed form baseline, year corrections / apply-to-FY, reprint
 * and receipt archive, Donor Master checks, and the read-path helpers (sends, report filter, 10BD
 * review list, 10BE, per-year summaries).
 * The version constants live in donation.module (the phase-C runner greps them there).
 * PHP 7.3-compatible (prod runs mod_php 7.3).
 */
/* ======================================================================
 * A. One clock (I20, L13). IST from UTC_TIMESTAMP()+330 min, never PHP date().
 * ====================================================================== */

function donation_db_today($reset = FALSE)
{
	$d = &drupal_static(__FUNCTION__);
	if ($reset || !isset($d))
		$d = (string) db_query("SELECT DATE(UTC_TIMESTAMP() + INTERVAL 330 MINUTE)")->fetchField();
	return $d;
}

function donation_db_now()
{
	return (string) db_query("SELECT DATE_FORMAT(UTC_TIMESTAMP() + INTERVAL 330 MINUTE, '%Y-%m-%d %H:%i:%s')")->fetchField();
}

function donation_cur_fy($reset = FALSE)
{
	return donation_fy_from_date(donation_db_today($reset));
}

/** FY start year (Apr-Mar) or 0 for an invalid/zero date; same rule as the SQL FYX expression. Test $fy > 0. */
function donation_fy_from_date($date)
{
	if (!preg_match('/^(\d{4})-(\d{1,2})-(\d{1,2})/', trim((string) $date), $m))
		return 0;
	$y = (int) $m[1];
	$mo = (int) $m[2];
	if ($y === 0 || $mo < 1 || $mo > 12)
		return 0;
	return ($mo >= 4) ? $y : $y - 1;
}

/** Strict date check for form input; normalises unpadded dates ('2026-3-5' -> '2026-03-05'). FALSE if invalid. */
function donation_normalise_ymd($date)
{
	if (!preg_match('/^\s*(\d{4})-(\d{1,2})-(\d{1,2})\s*$/', (string) $date, $m))
		return FALSE;
	if (!checkdate((int) $m[2], (int) $m[3], (int) $m[1]))
		return FALSE;
	return sprintf('%04d-%02d-%02d', $m[1], $m[2], $m[3]);
}

function donation_fy_label($fy)
{
	$fy = (int) $fy;
	return $fy . '-' . substr((string) ($fy + 1), -2);          // 2023 -> '2023-24'
}

function donation_10bd_due($fy)
{
	return ((int) $fy + 1) . '-05-31';
}

function donation_10bd_due_passed($fy)
{
	return donation_db_today() > donation_10bd_due($fy);
}

function donation_in_apr_may()
{
	$m = (int) substr(donation_db_today(), 5, 2);
	return ($m === 4 || $m === 5);
}

/* ======================================================================
 * B. Runtime guard (M9): drupal_static only (never cache_set), SHOW TRIGGERS.
 * ====================================================================== */

function donation_versioning_state()
{
	$s = &drupal_static(__FUNCTION__);
	if (!isset($s))
	{
		$s = array('tables' => FALSE, 'd_fy' => FALSE, 'triggers' => array());
		try
		{
			$s['tables'] = db_table_exists('dh_donor_version') && db_table_exists('dh_donor_fy')
				&& db_field_exists('dh_donor_version', 'dv_course');          // rev-3 shape (U1 columns)
			$s['d_fy'] = db_field_exists('dh_donation', 'd_fy');
			foreach (db_query("SHOW TRIGGERS LIKE 'dh_do%'") as $t)          // LIKE matches the TABLE name
			{
				$t = (array) $t;
				$s['triggers'][$t['Trigger']] = TRUE;
			}
		}
		catch (Exception $e)
		{
			watchdog_exception('donation', $e);
		}
	}
	return $s;
}

function donation_versioning_freeze_active()
{
	$s = donation_versioning_state();
	return !empty($s['triggers']['dh_donor_bu_freeze']);
}

/**
 * 'read':  d_fy + both tables (rev-3 shape). Enough for every versioned read, also in phase 2.
 * 'write': additionally the two d_fy triggers, and -- once DONATION_DONORVER_REQUIRE_FREEZE is TRUE -- the freeze
 *          trigger and dh_donor_au. Every donor/donation/bridge write path uses 'write'.
 */
function donation_versioning_ready($mode = 'write')
{
	$s = donation_versioning_state();
	if (!$s['tables'] || !$s['d_fy'])
		return FALSE;
	if ($mode === 'read')
		return TRUE;
	if (empty($s['triggers']['dh_don_bi_fy']) || empty($s['triggers']['dh_don_bu_fy']))
		return FALSE;
	if (DONATION_DONORVER_REQUIRE_FREEZE && (empty($s['triggers']['dh_donor_bu_freeze']) || empty($s['triggers']['dh_donor_au'])))
		return FALSE;
	return TRUE;
}

/** Call before ANY versioned read ($mode 'read') or write (default). Fails closed: never falls back to HEAD queries. */
function donation_versioning_require($element = NULL, $mode = 'write')
{
	if (donation_versioning_ready($mode))
		return TRUE;
	$msg = ($mode === 'read')   // pages, downloads and sends
		? t('Donor versioning is not completely installed on this site, so this cannot be done now. Please inform the Dana administrator.')
		: t('Donor versioning is not completely installed on this site, so nothing was saved. Please inform the Dana administrator.');
	watchdog('donation', 'Donor versioning objects missing: @s', array('@s' => json_encode(donation_versioning_state())), WATCHDOG_ERROR);
	if ($element !== NULL)
		form_set_error($element, $msg);
	else
		drupal_set_message($msg, 'error');
	return FALSE;
}

/* ======================================================================
 * C. Projection and comparators.
 * ====================================================================== */

/** The 19 data columns of a snapshot: 15 donor fields + 4 resolved names (U1). */
function donor_dv_cols()
{
	return array('dv_name', 'dv_gender', 'dv_address', 'dv_pincode', 'dv_contact', 'dv_email', 'dv_occupation',
		'dv_country_code', 'dv_country', 'dv_state_code', 'dv_state', 'dv_city_id', 'dv_city',
		'dv_id_type', 'dv_id_type_name', 'dv_id_no', 'dv_address_proof', 'dv_course', 'dv_confirmation');
}

/** dv column -> dh_donor column for the 15 stored fields (resolved names are not stored on dh_donor). */
function donor_dv_live_map()
{
	return array('dv_name' => 'd_name', 'dv_gender' => 'd_gender', 'dv_address' => 'd_address', 'dv_pincode' => 'd_pincode',
		'dv_contact' => 'd_contact', 'dv_email' => 'd_email', 'dv_occupation' => 'd_occupation',
		'dv_country_code' => 'd_country', 'dv_state_code' => 'd_state', 'dv_city_id' => 'd_city',
		'dv_id_type' => 'd_id_type', 'dv_id_no' => 'd_id_no', 'dv_address_proof' => 'd_address_proof',
		'dv_course' => 'd_course', 'dv_confirmation' => 'd_confirmation');
}

/**
 * VERBATIM projection of a raw dh_donor row (SELECT * FROM dh_donor) into dv_*: exactly what the freeze trigger
 * stores (no trim, no upper-casing; NULL -> ''; names resolved with the trigger's own lookups, same ORDER BY).
 */
function donor_snapshot_raw(array $r)
{
	$co = (string) $r['d_country'];
	$st = (string) $r['d_state'];
	$ci = (int) $r['d_city'];
	$it = (int) $r['d_id_type'];
	return array(
		'dv_name' => (string) $r['d_name'], 'dv_gender' => (string) $r['d_gender'], 'dv_address' => (string) $r['d_address'],
		'dv_pincode' => (string) $r['d_pincode'], 'dv_contact' => (string) $r['d_contact'], 'dv_email' => (string) $r['d_email'],
		'dv_occupation' => (string) $r['d_occupation'],
		'dv_country_code' => $co,
		'dv_country' => (string) db_query('SELECT c_name FROM dh_country WHERE c_code = :c ORDER BY c_id LIMIT 1', array(':c' => $co))->fetchField(),
		'dv_state_code' => $st,
		'dv_state' => (string) db_query('SELECT s_name FROM dh_state WHERE s_code = :s AND s_country = :c ORDER BY s_id LIMIT 1', array(':s' => $st, ':c' => $co))->fetchField(),
		'dv_city_id' => $ci,
		'dv_city' => (string) db_query('SELECT c_city FROM dh_city WHERE c_id = :i LIMIT 1', array(':i' => $ci))->fetchField(),
		'dv_id_type' => $it,
		'dv_id_type_name' => (string) db_query('SELECT it_name FROM dh_id_types WHERE it_code = :t ORDER BY it_id LIMIT 1', array(':t' => $it))->fetchField(),
		'dv_id_no' => (string) $r['d_id_no'], 'dv_address_proof' => (string) $r['d_address_proof'],
		'dv_course' => (int) $r['d_course'], 'dv_confirmation' => (string) $r['d_confirmation'],
	);
}

/** Live donor as dv. $for_update locks the dh_donor row (call first in a transaction, L3). Deleted donors included. */
function donor_live_dv($donor_id, $for_update = FALSE)
{
	$row = db_query('SELECT * FROM dh_donor WHERE d_id = :d' . ($for_update ? ' FOR UPDATE' : ''), array(':d' => (int) $donor_id))->fetchAssoc();
	return $row ? donor_snapshot_raw($row) : FALSE;
}

/**
 * What the donations of (donor, fy) show: the bridged snapshot (same-donor check, I2) else live.
 * Returns array('dv' => dv, 'own' => bool, 'since' => df_updated|NULL, 'vid' => int) or FALSE (no such donor).
 * Same resolution as every SQL read (no fy < CUR filter), so screen = receipt.
 */
function donor_effective_dv($donor_id, $fy)
{
	$r = db_query('SELECT dv.*, dfy.df_updated FROM dh_donor_fy dfy
			JOIN dh_donor_version dv ON (dv.dv_id = dfy.df_version AND dv.dv_donor = dfy.df_donor)
			WHERE dfy.df_donor = :d AND dfy.df_fy = :f', array(':d' => (int) $donor_id, ':f' => (int) $fy))->fetchAssoc();
	if ($r)
	{
		$dv = array();
		foreach (donor_dv_cols() as $c)
			$dv[$c] = $r[$c];
		$dv['dv_city_id'] = (int) $dv['dv_city_id'];
		$dv['dv_id_type'] = (int) $dv['dv_id_type'];
		$dv['dv_course'] = (int) $dv['dv_course'];
		return array('dv' => $dv, 'own' => TRUE, 'since' => $r['df_updated'], 'vid' => (int) $r['dv_id']);
	}
	$live = donor_live_dv($donor_id);
	return $live ? array('dv' => $live, 'own' => FALSE, 'since' => NULL, 'vid' => 0) : FALSE;
}

/**
 * THE ONE donor-ID comparator (L18, L21). Two IDs are the same iff their keys are equal.
 *  exact (default): type as int, number byte-exact. Used by the form delta, correction/apply no-op tests, R16x,
 *                   conflict tests; mirrors the trigger's ID compare.
 *  normalised:      number as donor_id_canonical() stores it (no spaces, dots or dashes; upper case).
 *                   ONLY for Donor Master / student-CSV no-op tests.
 * Both modes: type 0 or 99 with an EMPTY number is the one "no ID" value (0/'' == 99/''). 1/'' and ' ' stay distinct (exact).
 */
function donor_id_key($type, $no, $normalise = FALSE)
{
	$type = (int) $type;
	$no = (string) $no;
	if ($normalise)
		$no = donor_id_canonical($no);
	if ($no === '' && ($type === 0 || $type === 99))
		$type = 99;
	return $type . '|' . $no;
}

/** Canonical form of a TYPED ID number (form input, CSV): upper-case, no whitespace (L18: PAN upper-cased on input). */
function donor_id_canonical($no)
{
	// No spaces (also no-break and invisible ones), dots or dashes; capital letters. The same rule as the page
	// script (donation_id_check_js) and the DIPI sync (dipi_id_clean), so a typed "1234 5678-9012" is stored as one number.
	return strtoupper(preg_replace('/[\s\x{00A0}\x{200B}-\x{200D}\x{2060}\x{FEFF}.\-]+/u', '', (string) $no));
}

/** A "real" ID for the never-downgrade rule (L19): a 10BD type 1-7 with a non-blank number. */
function donor_id_is_real($type, $no)
{
	$type = (int) $type;
	return $type >= 1 && $type <= 7 && donor_id_canonical($no) !== '';
}

/**
 * THE equality of two dv records (register N2; no normalising) = the freeze trigger's change test: byte-exact over
 * the 15 STORED donor fields (codes for country/state/city/ID type, never the resolved names), NULL == '' (already
 * '' / 0 in a dv record), with the L21 ID rule (0/'' == 99/''). The resolved names (dv_country, dv_state, dv_city,
 * dv_id_type_name) are derived from the codes and not compared: a master rename alone is never a difference
 * (L34/P3: frozen years keep the names they had). Used for: correction and apply no-op tests, R16x (via the read-path
 * donor_10bd_key, which applies the same per-field rule to the printed subset), conflict tests.
 * A row listed by R16x is therefore never an apply no-op (its fields are a subset compared the same way).
 */
function donor_dv_equal(array $a, array $b, array $cols = NULL)
{
	if ($cols === NULL)
		$cols = donor_dv_cols();
	$id = FALSE;
	foreach ($cols as $c)
	{
		if ($c === 'dv_id_type' || $c === 'dv_id_no' || $c === 'dv_id_type_name')
		{
			$id = TRUE;
			continue;
		}
		if ($c === 'dv_country' || $c === 'dv_state' || $c === 'dv_city')
			continue;                                       // resolved names: not stored on dh_donor, not compared (N2)
		if ((string) $a[$c] !== (string) $b[$c])
			return FALSE;
	}
	if ($id && donor_id_key($a['dv_id_type'], $a['dv_id_no']) !== donor_id_key($b['dv_id_type'], $b['dv_id_no']))
		return FALSE;
	return TRUE;
}

/** Rev-2 names used by the read-path spec (R16x): thin aliases. */
function donor_live_snapshot($donor_id)
{
	return donor_live_dv($donor_id);
}

function donor_snapshots_equal(array $a, array $b)
{
	return donor_dv_equal($a, $b);
}

/** The donor values a receipt prints (generate_receipt via R4); used by the regeneration gate and the reprint offer. */
function donor_printed_key(array $dv)
{
	$no_id = (strtoupper((string) $dv['dv_id_type_name']) === 'NO ID');
	return json_encode(array((string) $dv['dv_gender'], (string) $dv['dv_name'], (string) $dv['dv_address'], (string) $dv['dv_pincode'],
		(string) $dv['dv_city'], (string) $dv['dv_state_code'], (string) $dv['dv_country_code'],
		$no_id ? '' : (string) $dv['dv_id_type_name'], $no_id ? '' : (string) $dv['dv_id_no'], (string) $dv['dv_contact']));
}

/* ======================================================================
 * D. Groups, display, baseline (M4, L22, L26).
 * ====================================================================== */

/** Field groups. A group is written as a whole or not at all; sources are never mixed inside a group (M4). */
function donor_groups($form = 'donation')
{
	$g = array(
		'name' => array('dv_name'),
		'gender' => array('dv_gender'),
		'contact' => array('dv_contact'),
		'email' => array('dv_email'),
		'id' => array('dv_id_type', 'dv_id_type_name', 'dv_id_no'),
		'location' => array('dv_address', 'dv_pincode', 'dv_country_code', 'dv_country', 'dv_state_code', 'dv_state', 'dv_city_id', 'dv_city'),
	);
	if ($form === 'donor')
		$g['occupation'] = array('dv_occupation');          // only /donors shows occupation
	return $g;
}

/** Form input name -> dv column (display value; codes are resolved from names). */
function donor_form_inputs($form = 'donation')
{
	$m = array('d_name' => 'dv_name', 'd_gender' => 'dv_gender', 'd_contact' => 'dv_contact', 'd_email' => 'dv_email',
		'd_id_type' => 'dv_id_type', 'd_id_no' => 'dv_id_no', 'd_address' => 'dv_address', 'd_pincode' => 'dv_pincode',
		'd_country' => 'dv_country', 'd_state' => 'dv_state', 'd_city' => 'dv_city');
	if ($form === 'donor')
		$m['d_occupation'] = 'dv_occupation';
	return $m;
}

/**
 * L22: what a donor form SHOWS for a dv record, keyed by form input name. Country/state/city NAMES go under
 * d_country/d_state/d_city (never codes). ID type: 0 or 99 with an empty number shows as 99 "No ID" (L21); an
 * unknown type shows as itself (donor_id_type_options() adds an "UNKNOWN (n)" option so it posts back unchanged).
 * $defaults: per-form display defaults for empty values (/donors: array('d_gender' => 'male'); donation form: none).
 * Input is ALWAYS a dv record (donor_snapshot_raw / donor_effective_dv), never donor_id_load() output.
 */
function donor_display_from_row(array $dv, array $defaults = array())
{
	$t = (int) $dv['dv_id_type'];
	if ($t === 0 && (string) $dv['dv_id_no'] === '')
		$t = 99;
	$out = array('d_name' => (string) $dv['dv_name'], 'd_gender' => (string) $dv['dv_gender'], 'd_contact' => (string) $dv['dv_contact'],
		'd_email' => (string) $dv['dv_email'], 'd_occupation' => (string) $dv['dv_occupation'], 'd_id_type' => (string) $t,
		'd_id_no' => (string) $dv['dv_id_no'], 'd_address' => (string) $dv['dv_address'], 'd_pincode' => (string) $dv['dv_pincode'],
		'd_country' => (string) $dv['dv_country'], 'd_state' => (string) $dv['dv_state'], 'd_city' => (string) $dv['dv_city']);
	foreach ($defaults as $k => $v)
		if ($out[$k] === '')
			$out[$k] = $v;
	return $out;
}

/** ID type select options; adds "UNKNOWN (n)" for a shown type that has no dh_id_types row (L20/L21). */
function donor_id_type_options($shown_type = NULL)
{
	$o = array();
	foreach (db_query('SELECT it_code, it_name FROM dh_id_types ORDER BY it_code') as $r)
		$o[$r->it_code] = $r->it_name;
	if ($shown_type !== NULL && $shown_type !== '' && !isset($o[(int) $shown_type]))
		$o[(int) $shown_type] = 'UNKNOWN (' . (int) $shown_type . ')';
	return $o;
}

/** Server check for the '#validated' ID type select: a dh_id_types code, or exactly the type the form showed. */
function donor_id_type_allowed($posted, array $shown_dv)
{
	$t = (int) $posted;
	if ($t === (int) $shown_dv['dv_id_type'])
		return TRUE;
	return (bool) db_query('SELECT 1 FROM dh_id_types WHERE it_code = :t', array(':t' => $t))->fetchField();
}

/**
 * S6: dh_donor is utf8mb3, so a 4-byte character (emoji) can never be stored (strict sessions fail with 1366).
 * Typed donor text drops them, exactly like the cancel reason (cancel-stamp.inc:21), and is trimmed.
 */
function donor_clean_text($s)
{
	$r = preg_replace('/[\x{10000}-\x{10FFFF}]/u', '', (string) $s);
	return trim($r === NULL ? (string) $s : $r);       // NULL only for invalid UTF-8, which Drupal's form API already rejects
}

/** Form compare (changed-group detection only): trim, CRLF->LF; place names case-insensitive. ID: see donor_id_key. */
function donor_form_same($dvcol, $posted, $shown)
{
	$p = trim(str_replace("\r\n", "\n", (string) $posted));
	$s = trim(str_replace("\r\n", "\n", (string) $shown));
	if ($dvcol === 'dv_country' || $dvcol === 'dv_state' || $dvcol === 'dv_city')
		return drupal_strtolower($p) === drupal_strtolower($s);
	return $p === $s;
}

/** Groups whose POSTED values differ from what the form SHOWED ($shown_dv = baseline view). */
function donor_changed_groups(array $in, array $shown_dv, $form = 'donation', array $defaults = array())
{
	$show = donor_display_from_row($shown_dv, $defaults);
	$inputs = donor_form_inputs($form);
	$out = array();
	foreach (donor_groups($form) as $g => $cols)
	{
		if ($g === 'id')
		{
			$pt = isset($in['d_id_type']) ? $in['d_id_type'] : 0;
			$pn = isset($in['d_id_no']) ? (string) $in['d_id_no'] : '';
			if (donor_id_key($pt, $pn) !== donor_id_key($shown_dv['dv_id_type'], $shown_dv['dv_id_no']))
				$out[] = 'id';
			continue;
		}
		foreach ($inputs as $input => $col)
		{
			if (!in_array($col, $cols, TRUE))
				continue;
			if (!donor_form_same($col, isset($in[$input]) ? $in[$input] : '', $show[$input]))
			{
				$out[] = $g;
				break;
			}
		}
	}
	return $out;
}

/**
 * L24: new dv values for the CHANGED groups only. Untouched sub-fields of a changed group are copied VERBATIM from
 * $shown_dv (the bytes the clerk saw), never re-resolved; typed sub-fields are canonicalised and resolved.
 * $resolve FALSE (validate): no dh_city insert, city id left NULL. TRUE (submit, inside the transaction): get_city().
 * Returns array($new_partial_dv, $errors keyed by input name).
 */
function donor_form_new_values(array $in, array $shown_dv, array $groups, $resolve, $form = 'donation')
{
	$new = array();
	$err = array();
	$v = function ($k) use ($in) { return isset($in[$k]) ? (string) $in[$k] : ''; };
	foreach ($groups as $g)
	{
		switch ($g)
		{
			case 'name':
				$new['dv_name'] = donor_clean_text($v('d_name'));
				if ($new['dv_name'] === '')                  // S6: e.g. only emoji, which are dropped
					$err['d_name'] = t('Donor name is required (emoji cannot be stored).');
				break;
			case 'gender':     $new['dv_gender'] = $v('d_gender'); break;
			case 'contact':
				$new['dv_contact'] = donor_clean_text($v('d_contact'));
				if ($new['dv_contact'] === '')
					$err['d_contact'] = t('Contact number is required (emoji cannot be stored).');
				break;
			case 'email':      $new['dv_email'] = donor_clean_text($v('d_email')); break;
			case 'occupation': $new['dv_occupation'] = donor_clean_text($v('d_occupation')); break;
			case 'id':
				$t = (int) $v('d_id_type');
				$new['dv_id_type'] = $t;
				$new['dv_id_type_name'] = ($t === (int) $shown_dv['dv_id_type']) ? (string) $shown_dv['dv_id_type_name']
					: (string) db_query('SELECT it_name FROM dh_id_types WHERE it_code = :t ORDER BY it_id LIMIT 1', array(':t' => $t))->fetchField();
				$new['dv_id_no'] = donor_id_canonical($v('d_id_no'));
				break;
			case 'location':
				$addr_ch = !donor_form_same('dv_address', $v('d_address'), $shown_dv['dv_address']);
				$pin_ch = !donor_form_same('dv_pincode', $v('d_pincode'), $shown_dv['dv_pincode']);
				$co_ch = !donor_form_same('dv_country', $v('d_country'), $shown_dv['dv_country']);
				$st_ch = $co_ch || !donor_form_same('dv_state', $v('d_state'), $shown_dv['dv_state']);
				$ci_ch = $st_ch || !donor_form_same('dv_city', $v('d_city'), $shown_dv['dv_city']);
				$new['dv_address'] = $addr_ch ? donor_clean_text(str_replace("\r\n", "\n", $v('d_address'))) : $shown_dv['dv_address'];
				$new['dv_pincode'] = $pin_ch ? donor_clean_text($v('d_pincode')) : $shown_dv['dv_pincode'];
				$new['dv_country_code'] = $shown_dv['dv_country_code'];
				$new['dv_country'] = $shown_dv['dv_country'];
				$new['dv_state_code'] = $shown_dv['dv_state_code'];
				$new['dv_state'] = $shown_dv['dv_state'];
				$new['dv_city_id'] = (int) $shown_dv['dv_city_id'];
				$new['dv_city'] = $shown_dv['dv_city'];
				if ($co_ch)
				{
					$code = (string) get_country(trim($v('d_country')));
					if ($code === '')
					{
						$err['d_country'] = t('Unknown country "@c". Please pick it from the list.', array('@c' => $v('d_country')));
						break;
					}
					$new['dv_country_code'] = $code;
					$new['dv_country'] = (string) db_query('SELECT c_name FROM dh_country WHERE c_code = :c ORDER BY c_id LIMIT 1', array(':c' => $code))->fetchField();
				}
				if ($st_ch)
				{
					$code = (string) get_state(trim($v('d_state')), $new['dv_country_code']);
					if ($code === '')
					{
						$err['d_state'] = t('Unknown state "@s" for @c. Please pick it from the list.', array('@s' => $v('d_state'), '@c' => $new['dv_country']));
						break;
					}
					$new['dv_state_code'] = $code;
					$new['dv_state'] = (string) db_query('SELECT s_name FROM dh_state WHERE s_code = :s AND s_country = :c ORDER BY s_id LIMIT 1',
						array(':s' => $code, ':c' => $new['dv_country_code']))->fetchField();
				}
				if ($ci_ch)
				{
					$name = donor_clean_text($v('d_city'));
					if ($name === '')
					{
						$new['dv_city_id'] = 0;
						$new['dv_city'] = '';
					}
					elseif ($resolve)
					{
						$new['dv_city_id'] = get_city($name, $new['dv_state_code'], $new['dv_country_code']);
						$new['dv_city'] = (string) db_query('SELECT c_city FROM dh_city WHERE c_id = :i', array(':i' => $new['dv_city_id']))->fetchField();
					}
					else
					{
						$new['dv_city_id'] = NULL;
						$new['dv_city'] = $name;
					}
				}
				break;
		}
	}
	return array($new, $err);
}

function donor_baseline_key()
{
	return drupal_get_private_key() . drupal_get_hash_salt() . 'dana-donor-baseline';
}

/**
 * The self-describing, signed baseline (M4): which donor, which FY, the FY view the form SHOWED and the live view.
 * $eff = donor_effective_dv($donor_id, $fy); for the /donors form pass fy 0 and the live record as 'dv'.
 * Returns array('json' => ..., 'sig' => ...) for the hidden fields donor_baseline / donor_baseline_sig.
 */
function donor_baseline_encode($donor_id, $fy, array $eff, array $live_dv)
{
	$p = array('v' => 3, 'donor' => (int) $donor_id, 'fy' => (int) $fy, 'own' => empty($eff['own']) ? 0 : 1,
		'since' => empty($eff['own']) ? '' : substr((string) $eff['since'], 0, 10),
		'ts' => (string) db_query('SELECT NOW()')->fetchField(),
		'fyv' => $eff['dv'], 'live' => $live_dv);
	$json = json_encode($p);
	if ($json === FALSE)
		return array('json' => '', 'sig' => '');
	return array('json' => $json, 'sig' => drupal_hmac_base64($json, donor_baseline_key()));
}

/** Verified baseline array, or NULL (missing, bad signature, wrong shape). */
function donor_baseline_decode($json, $sig)
{
	$json = (string) $json;
	$sig = (string) $sig;
	if ($json === '' || $sig === '' || !hash_equals(drupal_hmac_base64($json, donor_baseline_key()), $sig))
		return NULL;
	$b = json_decode($json, TRUE);
	if (!is_array($b) || !isset($b['v'], $b['donor'], $b['fy'], $b['fyv'], $b['live']) || (int) $b['v'] !== 3)
		return NULL;
	return $b;
}

/**
 * L26 four-case baseline rule. Returns array('case' => 'new'|'donor'|NULL, 'baseline' => array|NULL, 'error' => string|NULL).
 *   d_donor -1, no baseline                 -> 'new' (insert a donor)
 *   d_donor -1, baseline of a donor         -> error (inconsistent clear)
 *   d_donor >0, no baseline, create         -> error (details never loaded)
 *   d_donor >0, no baseline, edit, same donor as stored -> server baseline (page from before the deploy)
 *   d_donor >0, baseline for another donor / bad signature -> error
 */
function donor_baseline_case(array $in, $is_edit, $stored_donor, $stored_fy)
{
	$d = isset($in['d_donor']) ? (int) $in['d_donor'] : -1;
	$has = array_key_exists('donor_baseline', $in);
	$json = $has ? (string) $in['donor_baseline'] : '';
	$sig = isset($in['donor_baseline_sig']) ? (string) $in['donor_baseline_sig'] : '';
	if ($d <= 0)
	{
		if ($json === '')
			return array('case' => 'new', 'baseline' => NULL, 'error' => NULL);
		return array('case' => NULL, 'baseline' => NULL, 'error' => t('The donor name was cleared, but the details of a previously picked donor are still loaded. Please clear the Donor Name again, or pick the donor from the list.'));
	}
	if ($json === '')
	{
		if (!$is_edit || $d !== (int) $stored_donor)
			return array('case' => NULL, 'baseline' => NULL, 'error' => t('The details of the selected donor were not loaded. Please pick the donor again from the list.'));
		$eff = donor_effective_dv($d, $stored_fy);
		$live = donor_live_dv($d);
		if (!$eff || !$live)
			return array('case' => NULL, 'baseline' => NULL, 'error' => t('This donor no longer exists. Please pick the donor again.'));
		$enc = donor_baseline_encode($d, $stored_fy, $eff, $live);
		return array('case' => 'donor', 'baseline' => json_decode($enc['json'], TRUE), 'error' => NULL);
	}
	$b = donor_baseline_decode($json, $sig);
	if (!$b)
		return array('case' => NULL, 'baseline' => NULL, 'error' => t('The loaded donor details could not be verified. Please reload the page.'));
	if ((int) $b['donor'] !== $d)
		return array('case' => NULL, 'baseline' => NULL, 'error' => t('The donor details on the form were loaded for another donor. Please pick the donor again.'));
	return array('case' => 'donor', 'baseline' => $b, 'error' => NULL);
}

/* ======================================================================
 * E. Writes. Every function below runs INSIDE the caller's db_transaction(), after donor_live_dv($id, TRUE).
 * ====================================================================== */

/**
 * Delta write of $groups to dh_donor (M4, L23). $new: donor_form_new_values(); $now: donor_live_dv($id, TRUE);
 * $seen_live: the live view the form was based on (baseline 'live'). Writes only the columns of the listed groups
 * whose new bytes differ from the DB now, so the live group ends up exactly equal to $new (never a mix).
 * A conflict is recorded only for a group that actually writes a column AND changed in the DB since the form loaded.
 */
function donor_write_live_groups($donor_id, array $now, array $new, array $groups, array $seen_live, $uid, $form = 'donation')
{
	$map = donor_dv_live_map();
	$all = donor_groups($form);
	$f = array();
	$conf = array();
	foreach ($groups as $g)
	{
		$wrote = FALSE;
		if ($g === 'id')
		{
			if (donor_id_key($new['dv_id_type'], $new['dv_id_no']) !== donor_id_key($now['dv_id_type'], $now['dv_id_no']))
			{
				$f['d_id_type'] = (int) $new['dv_id_type'];
				$f['d_id_no'] = (string) $new['dv_id_no'];
				$wrote = TRUE;
			}
		}
		else
		{
			foreach ($all[$g] as $c)
				if (isset($map[$c]) && (string) $new[$c] !== (string) $now[$c])
				{
					$f[$map[$c]] = $new[$c];
					$wrote = TRUE;
				}
		}
		if ($wrote && !donor_dv_equal($now, $seen_live, $all[$g]))
			$conf[] = $g;
	}
	if ($f)
	{
		if (isset($f['d_country']) || isset($f['d_state']) || isset($f['d_city']))
			$f['d_error'] = '';
		$f['d_updatedby'] = (int) $uid;                  // the trigger credits the freeze to NEW.d_updatedby
		db_update('dh_donor')->fields($f)->expression('d_updated', 'NOW()')->condition('d_id', (int) $donor_id)->execute();
	}
	return array('cols' => array_keys($f), 'conflicts' => $conf);
}

/**
 * L24: $base (the FY's effective bytes NOW) with the columns of $groups copied verbatim from $src. Every other
 * field -- including occupation, course, confirmation and address proof -- stays byte-identical.
 */
function donor_correction_target(array $base, array $src, array $groups, $form = 'donation')
{
	$all = donor_groups($form);
	$t = $base;
	foreach ($groups as $g)
		foreach ($all[$g] as $c)
			$t[$c] = $src[$c];
	return $t;
}

/** Reuse a byte-identical 'correct'/'apply' snapshot of the same donor (L35), else INSERT. Snapshots are never UPDATEd. */
function donor_snapshot_find_or_create($donor_id, array $dv, $uid, $source)
{
	$cols = donor_dv_cols();
	$rows = db_query("SELECT * FROM dh_donor_version WHERE dv_donor = :d AND dv_source IN ('correct', 'apply') ORDER BY dv_id DESC",
		array(':d' => (int) $donor_id));
	foreach ($rows as $r)
	{
		$r = (array) $r;
		$same = TRUE;
		foreach ($cols as $c)
			if ((string) $r[$c] !== (string) $dv[$c])
			{
				$same = FALSE;
				break;
			}
		if ($same)
			return (int) $r['dv_id'];
	}
	$f = array('dv_donor' => (int) $donor_id, 'dv_source' => $source, 'dv_updatedby' => (int) $uid,
		'dv_created' => donation_db_now());                        // I20: the trigger's IST clock, not the session time zone
	foreach ($cols as $c)
		$f[$c] = $dv[$c];
	return (int) db_insert('dh_donor_version')->fields($f)->execute();
}

/** Upsert the ONE bridge row (donor, past fy) and log it under dh_donor (I8). Refuses fy <= 0 and fy >= CUR (I3). */
function donor_fy_point($donor_id, $fy, $vid, $uid, array $old, array $new, $event, array $extra = array())
{
	$fy = (int) $fy;
	if ($fy <= 0 || $fy >= donation_cur_fy())
		throw new Exception('donor_fy_point: only past FYs can be pinned (fy ' . $fy . ')');
	db_merge('dh_donor_fy')->key(array('df_donor' => (int) $donor_id, 'df_fy' => $fy))
		->fields(array('df_version' => (int) $vid, 'df_updatedby' => (int) $uid, 'df_updated' => donation_db_now()))->execute();   // I20 clock
	$o = array();
	$n = array();
	foreach (donor_dv_live_map() as $dvc => $dk)                       // codes under d_*, like dh_donor_au rows
		if ((string) $old[$dvc] !== (string) $new[$dvc])
		{
			$o[$dk] = (string) $old[$dvc];
			$n[$dk] = (string) $new[$dvc];
		}
	db_insert('dh_log')->fields(array('l_module' => 'dh_donor', 'l_identifier' => (int) $donor_id, 'l_event' => substr($event, 0, 20),
		'l_msg' => json_encode(array('old' => (object) $o, 'new' => (object) $n, 'version' => (int) $vid, 'fy' => $fy) + $extra),
		'l_user' => (int) $uid))->execute();
}

/**
 * Point (donor, past fy) at $target (built by donor_correction_target) unless the FY already shows exactly that.
 * Logs 'FY yyyy-yy' (dh_donor) and 'Donor FY corrected' (dh_donation, shown by view_log_form). dh_donor untouched.
 */
function donor_past_fy_correct($donor_id, $fy, array $target, $uid, $donation_id, $scope, array $groups)
{
	$fy = (int) $fy;
	if ($fy <= 0 || $fy >= donation_cur_fy())
		return FALSE;
	$eff = donor_effective_dv($donor_id, $fy);
	if (!$eff || donor_dv_equal($eff['dv'], $target))
		return FALSE;
	$vid = donor_snapshot_find_or_create($donor_id, $target, $uid, 'correct');
	$extra = array('scope' => (string) $scope, 'groups' => implode(',', $groups));
	if ($donation_id)
		$extra['donation'] = (int) $donation_id;
	$late = donation_10bd_due_passed($fy);
	if ($late)
		$extra['after_10bd_due'] = 1;
	donor_fy_point($donor_id, $fy, $vid, $uid, $eff['dv'], $target, 'FY ' . donation_fy_label($fy), $extra);
	if ($donation_id)
		db_insert('dh_log')->fields(array('l_module' => 'dh_donation', 'l_identifier' => (int) $donation_id, 'l_event' => 'Donor FY corrected',
			'l_msg' => 'Donor ' . (int) $donor_id . ' details for FY ' . donation_fy_label($fy) . ' corrected (' . implode(', ', $groups)
				. '; scope ' . $scope . '; snapshot ' . $vid . ')' . ($late ? ' after the 10BD due date' : ''),
			'l_user' => (int) $uid))->execute();
	return TRUE;
}

/**
 * "Use the current details for FY X": point (donor, past fy) at a verbatim snapshot of LIVE (all 15 fields).
 * Used by /donors apply-to-FY (after the donor UPDATE, same transaction) and by the R16x bulk apply
 * ($extra = array('via' => '10bd-review')). No row when the FY already shows exactly these details (donor_dv_equal).
 */
function donor_apply_to_fy($donor_id, $fy, $uid, array $extra = array())
{
	$fy = (int) $fy;
	if ($fy <= 0 || $fy >= donation_cur_fy())
		return FALSE;                                   // I3: only past FYs (a stray current-FY bridge is reported by D3, never deleted here)
	$eff = donor_effective_dv($donor_id, $fy);
	$live = donor_live_dv($donor_id);
	if (!$eff || !$live || donor_dv_equal($eff['dv'], $live))
		return FALSE;
	$vid = donor_snapshot_find_or_create($donor_id, $live, $uid, 'apply');
	if (donation_10bd_due_passed($fy))
		$extra['after_10bd_due'] = 1;
	donor_fy_point($donor_id, $fy, $vid, $uid, $eff['dv'], $live, 'FY ' . donation_fy_label($fy) . ' apply', $extra);
	return TRUE;
}

/**
 * Conflict note (L23): who else changed this donor after the form was loaded. Called after COMMIT, so the clerk's
 * own rows ('FY freeze', 'Donor Updated', 'FY yyyy-yy', all with l_user = this clerk) are excluded.
 */
function donor_conflict_message($donor_id, array $groups, $since)
{
	global $user;
	$r = db_query("SELECT l.l_user, l.l_tstamp, u.name FROM dh_log l LEFT JOIN users u ON u.uid = l.l_user
			WHERE l.l_module = 'dh_donor' AND l.l_identifier = :d AND l.l_tstamp >= :t AND l.l_user <> :me ORDER BY l.l_id DESC LIMIT 1",
		array(':d' => (int) $donor_id, ':t' => (string) $since, ':me' => (int) $user->uid))->fetchAssoc();
	$who = !$r ? t('another user') : ((int) $r['l_user'] === 0 ? t('System / Sync (DIPI)') : (string) $r['name']);
	return t('While this form was open, @who also changed this donor (@when). Your values were saved for: @g. Please check the donor.',
		array('@who' => $who, '@when' => $r ? $r['l_tstamp'] : '?', '@g' => implode(', ', $groups)));
}

/* ======================================================================
 * F. Reprint (M12) and receipt archive (P1).
 * ====================================================================== */

/** Per-FY printed key for every FY this donor has donations in, plus the current FY. */
function donor_fy_printed_map($donor_id)
{
	$fys = db_query('SELECT DISTINCT d_fy FROM dh_donation WHERE d_donor = :d', array(':d' => (int) $donor_id))->fetchCol();
	$fys[] = donation_cur_fy();
	$map = array();
	foreach (array_unique(array_map('intval', $fys)) as $fy)
	{
		$e = donor_effective_dv($donor_id, $fy);
		if ($e)
			$map[$fy] = donor_printed_key($e['dv']);
	}
	return $map;
}

/** Active, already-receipted donations of (donor, fy) the current user may reprint; $exclude = just-saved donation. */
function donation_reprint_ids($donor_id, $fy, $exclude = 0)
{
	global $user;
	$q = db_select('dh_donation', 'd')->fields('d', array('d_id'))
		->condition('d.d_donor', (int) $donor_id)->condition('d.d_fy', (int) $fy)->condition('d.d_deleted', '0')
		->condition('d.d_receipt_num', 0, '>')->condition('d.d_id', (int) $exclude, '<>')
		->condition('d.d_type', 'razorpay', '<>')->orderBy('d.d_id');           // online receipts are not re-rendered here (as donation_id_load)
	if (user_access('access own donation') && !user_access('administer donation'))
		$q->condition('d.d_createdBy', $user->uid);
	return array_map('intval', $q->execute()->fetchCol());
}

/** After a donor-changing save: offer "Reprint N receipts of FY X" for every FY whose printed details changed. */
function donor_reprint_offer($donor_id, array $before, $exclude_donation = 0)
{
	$links = array();
	foreach (donor_fy_printed_map($donor_id) as $fy => $key)
	{
		if (isset($before[$fy]) && $before[$fy] === $key)
			continue;
		$n = count(donation_reprint_ids($donor_id, $fy, $exclude_donation));
		if ($n > 0 && donation_reprint_access())
			$links[] = l(format_plural($n, 'Reprint 1 receipt of FY @f', 'Reprint @count receipts of FY @f', array('@f' => donation_fy_label($fy))),
				'donors/reprint/' . (int) $donor_id . '/' . (int) $fy,
				$exclude_donation ? array('query' => array('skip' => (int) $exclude_donation)) : array());   // the page lists the same N
	}
	if ($links)
		drupal_set_message(t('The donor details printed on receipts have changed. Receipts already issued still show the old details:') . ' ' . implode(' &middot; ', $links), 'warning');
}

/** W5: the existing edit-donation permissions (HEAD donation_edit_access :298-304); own-only and Razorpay filters are applied per receipt. */
function donation_reprint_access()
{
	return user_access('administer donation') || user_access('edit own donation');
}

/**
 * P1: copy the current S3 objects to <key minus .pdf>-<YYYYmmddHHMMSS>[-n].pdf before an overwrite.
 * $required TRUE (amend/reprint): any failure other than "source missing" returns FALSE and the caller must NOT overwrite.
 * $required FALSE (new receipt number): failures are logged only (normally nothing exists under a new number).
 */
function donation_receipt_archive($donation_id, array $keys, $required = TRUE)
{
	global $user;
	$stamp = (string) db_query("SELECT DATE_FORMAT(UTC_TIMESTAMP() + INTERVAL 330 MINUTE, '%Y%m%d%H%i%s')")->fetchField();
	$done = array();
	foreach ($keys as $key)
	{
		$base = preg_replace('/\.pdf$/', '', $key) . '-' . $stamp;
		$dst = $base . '.pdf';
		for ($i = 1; $i < 10 && s3_object_exists('vri-dana', $dst) === TRUE; $i++)
			$dst = $base . '-' . $i . '.pdf';                        // never overwrite an earlier archive (same-second reprints)
		$r = s3_copy_file('vri-dana', $key, $dst);
		if ($r['success'])
			$done[] = $dst;
		elseif (empty($r['missing']))
		{
			watchdog('S3-archive', 'Receipt @k not archived: @m', array('@k' => $key, '@m' => $r['message']), WATCHDOG_ERROR);
			if ($required)
				return FALSE;
		}
	}
	if ($done)
		db_insert('dh_log')->fields(array('l_module' => 'dh_donation', 'l_identifier' => (int) $donation_id, 'l_event' => 'Receipt archived',
			'l_msg' => 'Previous receipt kept as ' . implode(', ', $done), 'l_user' => (int) $user->uid))->execute();
	return TRUE;
}

/* ======================================================================
 * G. Shared bridge join (R4 receipt, R5 sends, every read path). Sends themselves: read-path spec R5/R5b/R5c (U2).
 * ====================================================================== */

/** Shared bridge join (I2 ownership check). Same text as the read-path spec. */
function donation_donor_fy_join($d = 'd')
{
	return ' LEFT JOIN dh_donor_fy dfy ON (dfy.df_donor = ' . $d . '.d_donor AND dfy.df_fy = ' . $d . '.d_fy)
		LEFT JOIN dh_donor_version dv ON (dv.dv_id = dfy.df_version AND dv.dv_donor = dfy.df_donor) ';
}

/* ======================================================================
 * H. Donor Master (M7, L19, L20).
 * ====================================================================== */

function donation_site_key()
{
	return basename(conf_path());                 // e.g. 'ahobilam.dana.vridhamma.org'; unique per site
}

function donor_master_key()
{
	return drupal_get_private_key() . drupal_get_hash_salt() . 'dana-donor-master';
}

function donor_master_sig($site, $exported)
{
	return substr(hash_hmac('sha256', 'donor-master|' . $site . '|' . $exported, donor_master_key()), 0, 16);
}

/** Per-row "Check": proves a row's ID is unchanged since export (normalised ID, L18). 'k' prefix keeps Excel from making it a number. */
function donor_master_row_check($donor_id, $type, $no)
{
	return 'k' . substr(hash_hmac('sha256', donation_site_key() . '|' . (int) $donor_id . '|' . donor_id_key($type, $no, TRUE), donor_master_key()), 0, 12);
}

/** L20: never export an unknown type as 'PAN'. */
function donor_master_type_label($code, $it_name, $no)
{
	if ((string) $it_name !== '')
		return (string) $it_name;
	if ((int) $code === 0 && (string) $no === '')
		return 'No ID';                                    // L21 no-ID equivalence
	return 'UNKNOWN(' . (int) $code . ')';
}

function donor_name_key($s)
{
	return drupal_strtolower(trim(preg_replace('/\s+/u', ' ', (string) $s)));
}

/** M7 identity anchor: the row's name equals the donor's live name or any of its snapshot names. */
function donor_master_identity_ok($donor_id, $name, $live_name)
{
	$k = donor_name_key($name);
	if ($k === '')
		return FALSE;
	if ($k === donor_name_key($live_name))
		return TRUE;
	foreach (db_query('SELECT DISTINCT dv_name FROM dh_donor_version WHERE dv_donor = :d', array(':d' => (int) $donor_id))->fetchCol() as $n)
		if ($k === donor_name_key($n))
			return TRUE;
	return FALSE;
}

/** Format check of an uploaded ID (catches Excel's 9.99999E+11 and blank PANs). 'No ID' accepts anything. */
function donor_master_id_format_ok($type, $no)
{
	$type = (int) $type;
	if ($type === 99)
		return TRUE;
	if ($type === 1)
		return (bool) preg_match('/^[A-Z]{5}[0-9]{4}[A-Z]$/', $no);
	if ($type === 2)
		return (bool) preg_match('/^[2-9][0-9]{11}$/', $no);
	return $no !== '' && (bool) preg_match('/[0-9]/', $no);
}

/* ======================================================================
 * I. Aid: U5(a) April-May note (the L28 per-year table and badge are the read-path spec's, section 6).
 * ====================================================================== */

/** U5(a): only in April-May, and (for a donor) only when the donor has donations in the just-closed FY. '' otherwise. */
function donor_apr_may_note($donor_id = 0)
{
	if (!donation_in_apr_may())
		return '';
	$prev = donation_cur_fy() - 1;
	if ($donor_id && !db_query("SELECT 1 FROM dh_donation WHERE d_donor = :d AND d_fy = :f AND d_deleted = '0' AND d_foreign = '0' LIMIT 1",
			array(':d' => (int) $donor_id, ':f' => $prev))->fetchField())
		return '';
	return t('FY @f closed on 31 March @y. Its Form 10BD (due @due) uses the donor details FY @f had when it closed: changes saved now apply from now on only. If a change must also appear in the FY @f 10BD, also apply it to FY @f (tick FY @f below, or correct it from one of its donations, or use the review list on the Income Tax 10BD page).',
		array('@f' => donation_fy_label($prev), '@y' => $prev + 1, '@due' => '31 May ' . ($prev + 1)));
}

/* ======================================================================
 * J. Donation form glue: edit prefill (R1) and the donor-detail baseline (R2 addition).
 * ====================================================================== */

/** Donor part of the donation form for (donor, fy): FY view shown, signed baseline with FY + live views (M4), L28 banner. */
function donation_prefill_donor(array $row, $donor_id, $fy)
{
	$eff = donor_effective_dv($donor_id, $fy);
	$live = donor_live_dv($donor_id);
	if (!$eff || !$live)
		return $row;                                  // orphan donation: HEAD behaviour (saved as a new donor)
	$show = donor_display_from_row($eff['dv']);
	$row['donor_id'] = (int) $donor_id;
	foreach (array('d_name', 'd_gender', 'd_contact', 'd_email', 'd_id_type', 'd_id_no', 'd_address', 'd_pincode') as $k)
		$row[$k] = $show[$k];
	$row['c_name'] = $show['d_country'];
	$row['s_name'] = $show['d_state'];
	$row['c_city'] = $show['d_city'];
	$enc = donor_baseline_encode($donor_id, $fy, $eff, $live);
	$row['donor_baseline'] = $enc['json'];
	$row['donor_baseline_sig'] = $enc['sig'];
	$row['donor_fycounts'] = donor_fy_counts($donor_id);
	return $row;
}

function donor_fy_counts($donor_id)
{
	// same format as R2 'fycounts' {fy: n}; cancelled donations included (they count for freezing, D6)
	return array_map('intval', db_query('SELECT d_fy, COUNT(*) FROM dh_donation WHERE d_donor = :d GROUP BY d_fy',
		array(':d' => (int) $donor_id))->fetchAllKeyed());
}

/**
 * Called by _get_donor_detail() right before drupal_json_output() when a donor was found:
 *   if ($matches) $matches[0] += donation_donor_detail_baseline($did, $fy);
 * The JS fills the donor inputs from baseline.fyv (donor_display_from_row rules), so the values shown and the
 * baseline can never disagree. Values are RAW (no check_plain): the comparison is always raw vs raw.
 */
function donation_donor_detail_baseline($did, $fy)
{
	$cur = donation_cur_fy();
	$fy = (int) $fy;
	if ($fy <= 0)
		$fy = $cur;                               // R2 treats fy 0 as "current"; the baseline must name the real FY
	// a future FY (date typed past today) is kept: it reads live like the current FY, and the baseline FY must equal
	// FY(date) or validate would rebuild the form once more (L25)
	$eff = donor_effective_dv($did, $fy);
	$live = donor_live_dv($did);
	if (!$eff || !$live)
		return array();
	$enc = donor_baseline_encode($did, $fy, $eff, $live);
	return array('baseline' => $enc['json'], 'baseline_sig' => $enc['sig'], 'cur_fy' => $cur, 'baseline_fy' => $fy);
}

/* ======================================================================
 * K. Read paths: shared per-donation effective donor (R5 email, R5b WhatsApp, R5c form send).
 * ====================================================================== */

/** The donation row plus the donor details of its financial year (snapshot, else current); live_* for messages only. */
function donation_effective_donor($donation_id) {
  return db_query("SELECT d.*, DATE_FORMAT(d.d_date, '%D %M, %Y') AS d_date_c,
      COALESCE(dv.dv_name, don.d_name) AS d_name, COALESCE(dv.dv_gender, don.d_gender) AS d_gender,
      COALESCE(dv.dv_email, don.d_email) AS d_email, COALESCE(dv.dv_contact, don.d_contact) AS d_contact,
      don.d_email AS live_email, don.d_contact AS live_contact,
      (dv.dv_id IS NOT NULL) AS fy_own
    FROM dh_donation d LEFT JOIN dh_donor don ON d.d_donor = don.d_id" . donation_donor_fy_join('d') . "
    WHERE d.d_id = :id", array(':id' => (int) $donation_id))->fetchAssoc();
}

/** R6/R8: bound WHERE fragment for the report and its Excel export. $versioned joins dh_donor_fy/dh_donor_version as dfy/dv. */
function donation_report_where(array $p, $versioned, $receipt_mode, array &$args) {
  global $user;
  $v = function ($k) use ($p) { return isset($p[$k]) ? $p[$k] : ''; };
  $w = '';
  if (user_access('access own donation') && !user_access('administer donation')) {
    $w .= ' AND d.d_createdBy = :own_uid'; $args[':own_uid'] = (int) $user->uid;
  }
  if ($v('with_logs') == 1)  $w .= ' AND d.d_update_count > 0';
  if ($v('synced') == 1)     $w .= ' AND d.d_synced = 1';
  if ($v('not_synced') == 1) $w .= ' AND d.d_synced = 0';
  if ($v('deleted') == 1)    $w .= " AND d.d_deleted = '1'";
  if ((string) $v('min_amount') !== '') { $w .= ' AND d.d_amount >= :min_amt'; $args[':min_amt'] = (int) $v('min_amount'); }
  if ((string) $v('max_amount') !== '') { $w .= ' AND d.d_amount <= :max_amt'; $args[':max_amt'] = (int) $v('max_amount'); }
  $pid = trim((string) $v('photo_id'));
  if ($pid !== '') {
    if ($versioned) { $w .= ' AND (don.d_id_no = :pid1 OR dv.dv_id_no = :pid2)'; $args[':pid1'] = $pid; $args[':pid2'] = $pid; }
    else            { $w .= ' AND don.d_id_no = :pid1'; $args[':pid1'] = $pid; }
  }
  if ((string) $v('from') !== '') { $w .= ' AND d.d_date >= :from_d'; $args[':from_d'] = (string) $v('from'); }
  if ((string) $v('to') !== '')   { $w .= ' AND d.d_date <= :to_d';   $args[':to_d']   = (string) $v('to'); }
  if ($v('foreign') === 'local')       $w .= " AND d.d_foreign = '0'";
  elseif ($v('foreign') === 'foreign') $w .= " AND d.d_foreign = '1'";
  if ((string) $v('category') !== 'all') { $w .= ' AND d.d_category = :cat'; $args[':cat'] = (string) $v('category'); }
  if ((string) $v('id_type') !== 'all' && (string) $v('id_type') !== '') {
    $w .= $versioned ? ' AND COALESCE(dv.dv_id_type, don.d_id_type) = :itype' : ' AND don.d_id_type = :itype';
    $args[':itype'] = (int) $v('id_type');
  }
  if (is_array($v('txn_type')) && $v('txn_type')) { $w .= ' AND d.d_type IN (:txn_types)'; $args[':txn_types'] = array_values($v('txn_type')); }
  if (is_array($v('don_tags')) && $v('don_tags')) { $w .= ' AND dt.dt_tag IN (:don_tags)'; $args[':don_tags'] = array_values($v('don_tags')); }
  if ((string) $v('course') !== '') { $w .= ' AND d.d_course = :course'; $args[':course'] = (int) $v('course'); }
  $rc = (string) $v('receipt');
  if ($rc !== '') {
    if ($receipt_mode === 'id')  { $w .= ' AND d.d_id = :rcpt'; }                       // XLS (HEAD semantics)
    elseif (is_numeric($rc))     { $w .= ' AND d.d_receipt_num = :rcpt'; }              // report
    else                         { $w .= ' AND d.d_receipt_f = :rcpt'; }
    $args[':rcpt'] = $rc;
  }
  $name = (string) $v('name');
  if ($name !== '') {
    if ($versioned) {
      $ids = donor_name_filter_ids($name);
      if (!$ids) $w .= ' AND 1 = 0';
      else {
        $w .= ' AND d.d_donor IN (:rname_ids) AND (don.d_name = :rname1 OR dv.dv_name = :rname2)';
        $args[':rname_ids'] = $ids; $args[':rname1'] = $name; $args[':rname2'] = $name;
      }
    }
    else { $w .= ' AND don.d_name = :rname1'; $args[':rname1'] = $name; }
  }
  if ((string) $v('user') !== '') { $w .= ' AND d.d_createdBy = :ruser'; $args[':ruser'] = (int) $v('user'); }
  return $w;
}
function donor_name_filter_ids($name) {
  return array_map('intval', db_query('SELECT d_id FROM dh_donor WHERE d_name = :n1
      UNION SELECT dv_donor FROM dh_donor_version WHERE dv_name = :n2',
    array(':n1' => $name, ':n2' => $name))->fetchCol());
}

/* ======================================================================
 * L. 10BD review list (R16x): comparator, data, form parts on the Income Tax 10BD page.
 * ====================================================================== */

// ===== S3 the 10BD comparator (R16x list). N2 + N3: the trigger's per-field rule (byte-exact, NULL == '',
// L21 no-ID equivalence) applied to the stored fields behind the printed 10BD columns: ID type, ID no, name,
// address, city (id), state (code), pincode. Never sanitize_10bd / case-folding (normalising is Donor Master only, L18).
function donor_10bd_key(array $v) {
  $t = (int) $v['dv_id_type'];
  if ($t < 1 || $t > 7) return array('out');            // not in any 10BD (covers 0/'' == 99/'', L21)
  return array('id' => donor_id_key($t, $v['dv_id_no']), 'name' => (string) $v['dv_name'], 'address' => (string) $v['dv_address'],
    'city' => (string) (int) $v['dv_city_id'], 'state' => (string) $v['dv_state_code'], 'pincode' => (string) $v['dv_pincode']);
}
function donor_fy_10bd_differs(array $fy_view, array $live_view) {
  return donor_10bd_key($fy_view) !== donor_10bd_key($live_view);
}
// apply no-op test = !donor_dv_equal($eff, $live) (helpers spec). A row listed here is never a no-op: its key fields are a
// subset of the 15 fields donor_dv_equal compares, with the same per-field rule.
function donor_fy_needs_apply(array $eff, array $live) {
  return !donor_snapshots_equal($eff, $live);
}
function donor_10bd_review_hash(array $fy_view, array $live_view) {
  return md5(serialize(array(donor_10bd_key($fy_view), donor_10bd_key($live_view), (int) $fy_view['dv_id'])));
}

// ===== S4 R16x data =====
function donation_10bd_review_data($fy) {
  $fy = (int) $fy;
  $start = $fy . '-04-01'; $end = ($fy + 1) . '-03-31';
  $out = array('differ' => array(), 'drop' => array(), 'dupes' => array());
  $res = db_query("SELECT dfy.df_donor AS donor_id, dfy.df_updated AS fy_since, uf.name AS fy_user, dfy.df_updatedby AS fy_uid,
        dv.dv_id, dv.dv_source, dv.dv_name, dv.dv_id_type, dv.dv_id_type_name, dv.dv_id_no,
        dv.dv_address, dv.dv_city_id, dv.dv_city, dv.dv_state_code, dv.dv_state, dv.dv_pincode, dv.dv_country,
        don.d_name, don.d_id_type, it.it_name AS live_id_type_name, don.d_id_no, don.d_address,
        don.d_city, ci.c_city, don.d_state, st.s_name, don.d_pincode, co.c_name,
        don.d_updated AS live_updated, ul.name AS live_user, don.d_updatedby AS live_uid
      FROM dh_donor_fy dfy
        JOIN dh_donor_version dv ON (dv.dv_id = dfy.df_version AND dv.dv_donor = dfy.df_donor)
        JOIN dh_donor don ON don.d_id = dfy.df_donor
        LEFT JOIN dh_id_types it ON it.it_code = don.d_id_type
        LEFT JOIN dh_city ci ON ci.c_id = don.d_city
        LEFT JOIN dh_state st ON (st.s_code = don.d_state AND st.s_country = don.d_country)
        LEFT JOIN dh_country co ON co.c_code = don.d_country
        LEFT JOIN users uf ON uf.uid = dfy.df_updatedby
        LEFT JOIN users ul ON ul.uid = don.d_updatedby
      WHERE dfy.df_fy = :fy
        AND EXISTS (SELECT 1 FROM dh_donation d WHERE d.d_donor = dfy.df_donor AND d.d_fy = dfy.df_fy
                     AND d.d_date >= :s AND d.d_date <= :e AND d.d_deleted = '0' AND d.d_foreign = '0')
      ORDER BY don.d_name, dfy.df_donor", array(':fy' => $fy, ':s' => $start, ':e' => $end));
  foreach ($res as $r) {
    $r = (array) $r;
    $live = array('dv_id' => 0, 'dv_name' => (string) $r['d_name'], 'dv_id_type' => (int) $r['d_id_type'], 'dv_id_no' => (string) $r['d_id_no'],
      'dv_id_type_name' => (string) $r['live_id_type_name'], 'dv_address' => (string) $r['d_address'],
      'dv_city_id' => (int) $r['d_city'], 'dv_city' => (string) $r['c_city'], 'dv_state_code' => (string) $r['d_state'],
      'dv_state' => (string) $r['s_name'], 'dv_pincode' => (string) $r['d_pincode']);
    if (!donor_fy_10bd_differs($r, $live)) continue;
    $r['hash'] = donor_10bd_review_hash($r, $live);
    $r['fy_key'] = donor_10bd_key($r); $r['live_key'] = donor_10bd_key($live); $r['live_view'] = $live;
    if ($r['live_key'] === array('out')) $out['drop'][$r['donor_id']] = $r; else $out['differ'][$r['donor_id']] = $r;
  }
  // Duplicate UINs among the FY's 10BD donors (effective ID)
  $res = db_query("SELECT x.d_donor AS donor_id, COALESCE(dv.dv_id_type, don.d_id_type) AS eff_id_type,
        COALESCE(dv.dv_id_no, don.d_id_no) AS eff_id_no, COALESCE(dv.dv_name, don.d_name) AS eff_name,
        (dv.dv_id IS NOT NULL) AS fy_own
      FROM (SELECT DISTINCT d.d_donor FROM dh_donation d
             WHERE d.d_date >= :s AND d.d_date <= :e AND d.d_deleted = '0' AND d.d_foreign = '0') x
        JOIN dh_donor don ON don.d_id = x.d_donor
        LEFT JOIN dh_donor_fy dfy ON (dfy.df_donor = x.d_donor AND dfy.df_fy = :fy)
        LEFT JOIN dh_donor_version dv ON (dv.dv_id = dfy.df_version AND dv.dv_donor = dfy.df_donor)
      WHERE COALESCE(dv.dv_id_type, don.d_id_type) BETWEEN 1 AND 7", array(':s' => $start, ':e' => $end, ':fy' => $fy));
  $by_uin = array();
  foreach ($res as $r) {
    $u = strtoupper(preg_replace('/[^A-Za-z0-9]/', '', (string) $r->eff_id_no));
    if ($u !== '') $by_uin[$u][(int) $r->donor_id] = (array) $r;
  }
  foreach ($by_uin as $u => $donors) if (count($donors) > 1) $out['dupes'][$u] = $donors;
  return $out;
}

// ===== S8 R16x form parts, added to donation_income_tax_form() (no new route) =====
function donation_10bd_review_form_parts(array &$form, array &$form_state) {
  $cur = donation_cur_fy();
  $fys = array();
  foreach (db_query("SELECT DISTINCT d_fy FROM dh_donation WHERE d_fy > 0 AND d_fy < :cur ORDER BY d_fy DESC", array(':cur' => $cur)) as $r)
    $fys[(int) $r->d_fy] = 'FY ' . donation_fy_label((int) $r->d_fy);
  if (!$fys) return;
  $fy = isset($form_state['storage']['review_fy']) ? (int) $form_state['storage']['review_fy'] : ($cur - 1);
  if (!isset($fys[$fy])) { reset($fys); $fy = key($fys); }

  $form['review'] = array('#type' => 'fieldset', '#title' => t('Donor details review for the 10BD'), '#weight' => -50,
    '#collapsible' => TRUE, '#collapsed' => FALSE);
  $form['review']['review_fy'] = array('#type' => 'select', '#title' => t('Financial year'), '#options' => $fys, '#default_value' => $fy);
  $form['review']['review_show'] = array('#type' => 'submit', '#value' => t('Show review list'),
    '#limit_validation_errors' => array(array('review_fy')), '#submit' => array('donation_10bd_review_show_submit'));

  $data = donation_10bd_review_data($fy);
  $hashes = array();
  $mk = function ($r) use (&$hashes) {
    $hashes[$r['donor_id']] = $r['hash'];
    return array(
      'donor' => l($r['d_name'], 'donors/edit/' . $r['donor_id']) . ' (#' . (int) $r['donor_id'] . ')',
      'diff' => donation_10bd_review_diff_html($r, $r['live_view']),
      'fy_since' => check_plain(date('d/m/Y', strtotime($r['fy_since'])) . ' - ' . ($r['fy_user'] ? $r['fy_user'] : t('System / Sync')) . ' (' . $r['dv_source'] . ')'),
      'live_changed' => check_plain(($r['live_updated'] ? date('d/m/Y', strtotime($r['live_updated'])) : '-') . ' - ' . ($r['live_user'] ? $r['live_user'] : t('System / Sync'))),
    );
  };
  $header = array('donor' => t('Donor'), 'diff' => t('FY @f details -> current record', array('@f' => donation_fy_label($fy))),
    'fy_since' => t('Year details since / by'), 'live_changed' => t('Current record last changed'));
  $can_apply = user_access('edit donors');
  $opts = array(); foreach ($data['differ'] as $id => $r) $opts[$id] = $mk($r);
  $drop = array(); foreach ($data['drop'] as $id => $r) $drop[$id] = $mk($r);
  $form['review']['summary'] = array('#markup' => '<p>'
    . format_plural(count($opts), '1 donor: FY @f details differ from the current record in a printed 10BD field.',
      '@count donors: FY @f details differ from the current record in a printed 10BD field.', array('@f' => donation_fy_label($fy))) . ' '
    . format_plural(count($drop), '1 more: applying would remove this donor from this 10BD.', '@count more: applying would remove them from this 10BD.') . ' '
    . format_plural(count($data['dupes']), '1 UIN is shared by more than one donor.', '@count UINs are shared by more than one donor.') . '</p>'
    . (donation_10bd_due_passed($fy) ? '<p class="warning">' . t('The 10BD for FY @f was due by 31/05/@y. If it was filed, applying changes may need a revised 10BD.', array('@f' => donation_fy_label($fy), '@y' => $fy + 1)) . '</p>' : ''));
  $form['review']['review_rows'] = $can_apply
    ? array('#type' => 'tableselect', '#header' => $header, '#options' => $opts, '#empty' => t('Nothing to review.'))
    : array('#markup' => theme('table', array('header' => array_values($header), 'rows' => array_values($opts), 'empty' => t('Nothing to review.'))));
  if ($drop)
    $form['review']['review_rows_drop'] = $can_apply
      ? array('#type' => 'tableselect', '#header' => $header, '#options' => $drop, '#js_select' => FALSE,
          '#prefix' => '<h4>' . t('Applying current details would REMOVE these donors from the FY @f 10BD (the current record has no 10BD ID)', array('@f' => donation_fy_label($fy))) . '</h4>')
      : array('#markup' => theme('table', array('header' => array_values($header), 'rows' => array_values($drop))));
  $form['review']['review_hashes'] = array('#type' => 'hidden', '#value' => json_encode($hashes));
  $form['review']['review_dupes'] = array('#markup' => donation_10bd_dupes_html($data['dupes']));
  if ($can_apply && ($opts || $drop))
    $form['review']['review_apply'] = array('#type' => 'submit', '#value' => t('Apply current details to FY @f', array('@f' => donation_fy_label($fy))),
      '#limit_validation_errors' => array(array('review_fy'), array('review_rows'), array('review_rows_drop'), array('review_hashes')),
      '#validate' => array('donation_10bd_review_apply_validate'), '#submit' => array('donation_10bd_review_apply_submit'),
      '#attributes' => array('onclick' => 'return confirm(' . drupal_json_encode(t('Apply the current donor details to FY @f for the ticked donors? Its 10BD will change.', array('@f' => donation_fy_label($fy)))) . ');'));
}
function donation_10bd_review_show_submit($form, &$form_state) {
  $form_state['storage']['review_fy'] = (int) $form_state['values']['review_fy'];
  $form_state['rebuild'] = TRUE;
}
function donation_10bd_review_apply_validate($form, &$form_state) {
  $fy = (int) $form_state['values']['review_fy'];
  if ($fy <= 0 || $fy >= donation_cur_fy()) form_set_error('review_fy', t('Only a past financial year can be changed.'));
  if (!user_access('edit donors')) form_set_error('review_rows', t('You are not allowed to change donor details.'));
  if (!donation_versioning_require('review_fy')) return;
}
function donation_10bd_review_apply_submit($form, &$form_state) {
  global $user;
  $fy = (int) $form_state['values']['review_fy'];
  $ids = array();
  foreach (array('review_rows', 'review_rows_drop') as $k)
    if (!empty($form_state['values'][$k])) foreach ($form_state['values'][$k] as $id => $on) if ($on) $ids[] = (int) $id;
  $seen = json_decode((string) $form_state['input']['review_hashes'], TRUE);
  $now = donation_10bd_review_data($fy);
  $all = $now['differ'] + $now['drop'];
  $applied = $stale = $gone = 0;
  $applied_ids = array();
  $txn = db_transaction();
  try {
    foreach ($ids as $id) {
      if (!isset($all[$id])) { $gone++; continue; }                                  // no longer differs
      if (!isset($seen[$id]) || $seen[$id] !== $all[$id]['hash']) { $stale++; continue; }   // changed since shown
      if (donor_apply_to_fy($id, $fy, $user->uid, array('via' => '10bd-review'))) { $applied++; $applied_ids[] = $id; }
    }
  }
  catch (Throwable $e) {   // PHP 7: also TypeError (same rule as the write paths)
    $txn->rollback();
    watchdog('donation', 'R16x apply rolled back: @m', array('@m' => $e->getMessage()), WATCHDOG_ERROR);
    drupal_set_message(t('Nothing was applied: @m', array('@m' => $e->getMessage())), 'error');
    $form_state['rebuild'] = TRUE; return;
  }
  unset($txn);
  drupal_set_message(t('FY @f: current details applied to @a; @s skipped because they changed after the list was shown; @g no longer differed.',
    array('@f' => donation_fy_label($fy), '@a' => format_plural($applied, '1 donor', '@count donors'), '@s' => $stale, '@g' => $gone)));
  // Receipts already issued for that year still show the old details: offer their reprint, as after a donor save.
  $links = array();
  if (donation_reprint_access())
    foreach ($applied_ids as $id) {
      $n = count(donation_reprint_ids($id, $fy, 0));
      if ($n > 0)
        $links[] = l(format_plural($n, 'Reprint 1 receipt of @d', 'Reprint @count receipts of @d', array('@d' => $all[$id]['d_name'])),
          'donors/reprint/' . $id . '/' . $fy);
    }
  if ($links) {
    $more = count($links) - 20;
    drupal_set_message(t('Receipts already issued for FY @f still show the old donor details:', array('@f' => donation_fy_label($fy))) . ' '
      . implode(' &middot; ', array_slice($links, 0, 20))
      . ($more > 0 ? ' ' . format_plural($more, 'and 1 more donor: use the Reprint link of their receipts in the Donation Report.',
          'and @count more donors: use the Reprint link of their receipts in the Donation Report.') : ''),
      'warning');
  }
  $form_state['storage']['review_fy'] = $fy;
  $form_state['rebuild'] = TRUE;
}
// $fy / $live: dv-keyed views (codes for the compare, names for the display). Shows raw values (check_plain only).
function donation_10bd_review_diff_html(array $fy, array $live) {
  $fk = donor_10bd_key($fy); $lk = donor_10bd_key($live);
  if ($fk === array('out') || $lk === array('out'))
    return $fk === array('out') ? t('FY: not in the 10BD (no ID) -> current: @t @n', array('@t' => $live['dv_id_type_name'], '@n' => $live['dv_id_no']))
                                : t('FY: @t @n -> current: no 10BD ID', array('@t' => $fy['dv_id_type_name'], '@n' => $fy['dv_id_no']));   // t() @ escapes once
  $show = array('id' => array(t('ID'), 'dv_id_type_name', 'dv_id_no'), 'name' => array(t('Name'), 'dv_name'), 'address' => array(t('Address'), 'dv_address'),
    'city' => array(t('City'), 'dv_city'), 'state' => array(t('State'), 'dv_state'), 'pincode' => array(t('Pincode'), 'dv_pincode'));
  $out = array();
  foreach ($show as $k => $d) {
    if ($fk[$k] === $lk[$k]) continue;
    $a = $fy[$d[1]] . (isset($d[2]) ? ' ' . $fy[$d[2]] : ''); $b = $live[$d[1]] . (isset($d[2]) ? ' ' . $live[$d[2]] : '');
    $out[] = '<strong>' . $d[0] . '</strong>: "' . check_plain($a) . '" -> "' . check_plain($b) . '"'
      . (drupal_strtolower(preg_replace('/\s+/', ' ', trim($a))) === drupal_strtolower(preg_replace('/\s+/', ' ', trim($b)))
          ? ' ' . t('(looks the same: spacing, capitals or a duplicate master entry)') : '');
  }
  return implode('<br>', $out);
}
function donation_10bd_dupes_html(array $dupes) {
  if (!$dupes) return '';
  $rows = array();
  foreach ($dupes as $uin => $donors) {
    $list = array();
    foreach ($donors as $id => $r) $list[] = l($r['eff_name'], 'donors/edit/' . $id) . ' (#' . $id . ($r['fy_own'] ? ', ' . t('year details') : '') . ')';
    $rows[] = array(check_plain($uin), implode('<br>', $list));
  }
  return '<h4>' . t('UINs shared by more than one donor in this 10BD') . '</h4>' . theme('table', array('header' => array(t('UIN'), t('Donors')), 'rows' => $rows));
}

/* ======================================================================
 * M. Donor pages (R13/R14, L28): per-year summary table and change-log rendering of FY events.
 * ====================================================================== */

/** The current record in the combined display format donor_fy_summary_table() compares against. */
function donor_fy_summary_live(array $live)
{
	$lv = $live;
	$lv['dv_id_no'] = trim($live['dv_id_type_name'] . ' ' . $live['dv_id_no']);
	$lv['dv_address'] = trim($live['dv_address'] . ', ' . $live['dv_city'] . ', ' . $live['dv_state'] . ' ' . $live['dv_pincode'] . ', ' . $live['dv_country'], ', ');
	return $lv;
}

// ===== S6 L28 per-FY summary (donor.inc; used by donor_form and donor_changelog) =====
function donor_fy_rows($donor_id) {
  return db_query("SELECT x.d_fy, x.n_all, x.n_active, (dv.dv_id IS NOT NULL) AS fy_own,
        dfy.df_updated, u.name AS fy_user, dv.*
      FROM (SELECT d_fy, COUNT(*) AS n_all, SUM(d_deleted = '0') AS n_active
              FROM dh_donation WHERE d_donor = :id GROUP BY d_fy) x
        LEFT JOIN dh_donor_fy dfy ON (dfy.df_donor = :id2 AND dfy.df_fy = x.d_fy)
        LEFT JOIN dh_donor_version dv ON (dv.dv_id = dfy.df_version AND dv.dv_donor = dfy.df_donor)
        LEFT JOIN users u ON u.uid = dfy.df_updatedby
      ORDER BY x.d_fy DESC", array(':id' => (int) $donor_id, ':id2' => (int) $donor_id))->fetchAll(PDO::FETCH_ASSOC);
}
function donor_fy_summary_table($donor_id, array $live) {
  $cur = donation_cur_fy();
  $cols = array('dv_name' => t('Name'), 'dv_id_no' => t('ID'), 'dv_address' => t('Address'), 'dv_contact' => t('Contact'), 'dv_email' => t('Email'));
  $rows = array();
  foreach (donor_fy_rows($donor_id) as $r) {
    $fy = (int) $r['d_fy'];
    $label = $fy > 0 ? 'FY ' . donation_fy_label($fy) : t('No valid date');
    $count = (int) $r['n_active'] . ' / ' . ((int) $r['n_all'] - (int) $r['n_active']);
    if (!$r['fy_own']) {
      $uses = ($fy >= $cur) ? t('Current details (current year)') : t('Current details');
      $rows[] = array($label, $count, $uses, '', array('data' => '', 'colspan' => count($cols)));
      continue;
    }
    $since = t('since @d by @u (@s)', array('@d' => date('d/m/Y', strtotime($r['df_updated'])),
      '@u' => $r['fy_user'] !== NULL && $r['fy_user'] !== '' ? $r['fy_user'] : t('System / Sync'), '@s' => $r['dv_source']));
    $row = array($label, $count, t('Own details'), $since);
    $r['dv_id_no'] = trim($r['dv_id_type_name'] . ' ' . $r['dv_id_no']);
    $r['dv_address'] = trim($r['dv_address'] . ', ' . $r['dv_city'] . ', ' . $r['dv_state'] . ' ' . $r['dv_pincode'] . ', ' . $r['dv_country'], ', ');
    foreach ($cols as $k => $title) {
      $differs = isset($live[$k]) && (string) $r[$k] !== (string) $live[$k];     // exact (one rule, N2): what differs is shown
      $row[] = array('data' => check_plain($r[$k]), 'class' => $differs ? array('fy-differs') : array(),
        'title' => $differs ? t('Current record: !v', array('!v' => $live[$k])) : '');   // !v: drupal_attributes() escapes it
    }
    $rows[] = $row;
  }
  return theme('table', array('header' => array_merge(array(t('Financial year'), t('Donations (active / cancelled)'), t('Details used'), t('Since')), array_values($cols)),
    'rows' => $rows, 'empty' => t('No donations.')));
}

// ===== S7 changelog FY branch (donor.inc:donor_changelog_table) =====
// Returns HTML: every value is escaped exactly once, by t()'s @ placeholders. Callers must not escape it again.
function donor_changelog_fy_text($event, array $data, $donor_id) {
  $v = isset($data['version']) ? (int) $data['version'] : 0;
  $snap = $v ? db_query('SELECT * FROM dh_donor_version WHERE dv_id = :v AND dv_donor = :d',
    array(':v' => $v, ':d' => (int) $donor_id))->fetchAssoc() : FALSE;
  $fmt = function ($s) {
    if (!$s) return t('(snapshot not found)');
    return t('Name "@n", ID @t "@i", address "@a, @c, @st @p, @co", contact "@ph", email "@e"', array(
      '@n' => $s['dv_name'], '@t' => $s['dv_id_type_name'], '@i' => $s['dv_id_no'], '@a' => $s['dv_address'], '@c' => $s['dv_city'],
      '@st' => $s['dv_state'], '@p' => $s['dv_pincode'], '@co' => $s['dv_country'], '@ph' => $s['dv_contact'], '@e' => $s['dv_email']));
  };
  $fy_list = function ($fys) {                     // "2022,2023" -> "FY 2022-23, FY 2023-24"
    $labels = array();
    foreach (is_array($fys) ? $fys : explode(',', (string) $fys) as $f) if ((int) $f > 0) $labels[] = 'FY ' . donation_fy_label((int) $f);
    return implode(', ', $labels);
  };
  if ($event === 'FY freeze skipped')               // checked first: the skipped row also carries 'fys'
    return t('Earlier-year freeze skipped (maintenance bypass, reason "@r"): @fys now follow the current details.',
      array('@r' => isset($data['reason']) ? $data['reason'] : '', '@fys' => isset($data['fys']) ? $fy_list($data['fys']) : ''));
  // !s: $fmt() output is already escaped
  if (isset($data['fys']) && isset($data['source']) && $data['source'] === 'seed')
    return t('Seed: @fys pinned to the details current at seeding: !s', array('@fys' => $fy_list($data['fys']), '!s' => $fmt($snap)));
  if (isset($data['fys'])) {                       // trigger freeze
    $out = t('Earlier years @fys keep the details they had before this edit: !s', array('@fys' => $fy_list($data['fys']), '!s' => $fmt($snap)));
    if (!empty($data['changed'])) {
      $ch = is_array($data['changed']) ? $data['changed'] : explode(',', (string) $data['changed']);
      $out .= ' ' . t('(changed: @c)', array('@c' => implode(', ', array_map('donor_changelog_field_label', $ch))));
    }
    return $out;
  }
  $what = (substr($event, -6) === ' apply') ? t('@ev: current details applied to that year', array('@ev' => $event))
                                            : t('@ev: details for that year corrected', array('@ev' => $event));
  $out = $what . ' - ' . $fmt($snap);
  if (!empty($data['after_10bd_due'])) $out .= ' ' . t('(after the 10BD due date)');
  return $out;
}

/* ======================================================================
 * N. Donation form write helpers (rebuild, new donor, receipt gate, sending) and Reprint (M12).
 * ====================================================================== */

/** Rebuild the donation form (no write): non-kept donor inputs are dropped so the builder's fresh defaults show (D7 rebuild keeps input). */
function donation_form_reload(&$form_state, $message, array $keep = array())
{
	$inputs = array('name' => array('d_name'), 'gender' => array('d_gender'), 'contact' => array('d_contact'), 'email' => array('d_email'),
		'id' => array('d_id_type', 'd_id_no'), 'location' => array('d_address', 'd_pincode', 'd_country', 'd_state', 'd_city'));
	$donor = isset($form_state['input']['d_donor']) ? (int) $form_state['input']['d_donor'] : -1;
	if ($donor > 0)
		foreach ($inputs as $g => $ks)
			if (!in_array($g, $keep, TRUE))
				foreach ($ks as $k)
					unset($form_state['input'][$k]);
	foreach (array('donor_baseline', 'donor_baseline_sig', 'donor_scope', 'form_cur_fy') as $k)
		unset($form_state['input'][$k]);
	$form_state['storage']['donor_reload'] = array('donor' => $donor, 'date' => isset($form_state['input']['d_date']) ? $form_state['input']['d_date'] : '');
	$form_state['rebuild'] = TRUE;
	drupal_set_message($message, 'warning');
}

/** F1: HEAD :1591-1620 for a new donor, with bound lookups, trimmed text and the canonical ID. */
function donation_insert_donor(array $in, $uid)
{
	$v = function ($k) use ($in) { return isset($in[$k]) ? (string) $in[$k] : ''; };
	$co = get_country($v('d_country'));
	$st = get_state($v('d_state'), $co);
	$ci = get_city($v('d_city'), $st, $co);
	$err = '';
	if ($co === '')
		$err .= 'No Country ' . $v('d_country') . "\n";
	if ($st === '')
		$err .= 'No State ' . $v('d_state') . "\n";
	if ($ci === 0 && trim($v('d_city')) !== '')
		$err .= 'No City ' . $v('d_city') . "\n";
	return (int) db_insert('dh_donor')->fields(array(
		'd_name' => donor_clean_text($v('d_name')), 'd_gender' => $v('d_gender'), 'd_id_type' => (int) $v('d_id_type'),
		'd_id_no' => donor_id_canonical($v('d_id_no')), 'd_contact' => donor_clean_text($v('d_contact')), 'd_email' => donor_clean_text($v('d_email')),
		'd_address' => donor_clean_text(str_replace("\r\n", "\n", $v('d_address'))), 'd_pincode' => donor_clean_text($v('d_pincode')),   // S6
		'd_country' => $co, 'd_state' => $st, 'd_city' => $ci, 'd_error' => $err, 'd_occupation' => '',
		'd_course' => (int) $v('d_course'), 'd_created' => date('Y-m-d H:i:s'), 'd_createdby' => (int) $uid))->execute();
}

/** D22 gate + M12: regenerate an existing receipt only when needed, or when the clerk asks. */
function donation_receipt_needs_regen($old, $donation_id, array $plan, array $before, array $in)
{
	if (!$old || (string) $old['d_receipt'] === '' || (int) $old['d_receipt_num'] <= 0)
		return TRUE;
	if (!empty($in['reprint']))
		return TRUE;                                           // explicit "Reprint receipt" tick
	$new = db_query('SELECT * FROM dh_donation WHERE d_id = :id', array(':id' => (int) $donation_id))->fetchAssoc();
	foreach (array('d_date', 'd_amount', 'd_currency', 'd_type', 'd_bank', 'd_ref_no', 'd_category', 'd_donor') as $k)
		if ((string) $old[$k] !== (string) $new[$k])
			return TRUE;                                       // a printed donation field
	$e = donor_effective_dv($new['d_donor'], $new['d_fy']);
	if ($e && isset($before[(int) $new['d_fy']]) && $before[(int) $new['d_fy']] !== donor_printed_key($e['dv']))
		return TRUE;                                           // the donor details this receipt prints changed
	foreach ($old as $k => $v)
		if (!in_array($k, array('d_lastupdated', 'd_updatedby', 'd_update_count'), TRUE) && (string) $v !== (string) $new[$k])
			return FALSE;                                      // something unprinted changed (comment, tags, branch...)
	return !$plan['groups'];                                   // nothing changed at all = the clerk's "Update Receipt" = reprint
}

/** R5c (read-path spec) wrapped as a function; body copied from there. */
function donation_send_after_save($donation_id, $receipt, array $in)
{
	$eff = donation_effective_donor($donation_id);
	$fy_note = ($eff && $eff['fy_own'])
		? ' ' . t('(the details recorded for FY @f)', array('@f' => donation_fy_label((int) $eff['d_fy']))) : '';
	if (!empty($in['email']))
	{
		$r = send_donor_email($donation_id);
		if (!empty($r['success']))
			drupal_set_message(t('Receipt emailed to @to', array('@to' => $r['to'])) . $fy_note);
		elseif ($r['to'] !== '')   // a send was attempted and failed (a missing address is already reported inside)
			drupal_set_message(t('The donation is saved, but the receipt could NOT be emailed to @to: @why. You can send it again from the report.',
				array('@to' => $r['to'], '@why' => $r['msg'])), 'warning');
	}
	if (!empty($in['whatsapp']))
	{
		$r = donation_whatsapp_send_receipt($donation_id, $receipt);
		if (!empty($r['success']))
			drupal_set_message(t('Receipt sent on WhatsApp to @to', array('@to' => $r['to'])) . $fy_note);
		else
			drupal_set_message(t('The donation is saved, but the receipt could NOT be sent on WhatsApp@to: @why. You can send it again from the report.',
				array('@to' => $r['to'] !== '' ? ' to ' . $r['to'] : '', '@why' => $r['msg'])), 'warning');
	}
}

function donation_log_reprint($donation_id)
{
	global $user;
	db_insert('dh_log')->fields(array('l_module' => 'dh_donation', 'l_identifier' => (int) $donation_id, 'l_event' => 'Receipt reprinted',
		'l_msg' => 'Receipt reprinted with the donor details of its financial year', 'l_user' => (int) $user->uid))->execute();
}

function donation_reprint_describe($donor_id, $fy)
{
	$e = donor_effective_dv($donor_id, $fy);
	if (!$e)
		return '';
	$dv = $e['dv'];
	$id = (strtoupper($dv['dv_id_type_name']) === 'NO ID') ? '' : $dv['dv_id_type_name'] . ' ' . $dv['dv_id_no'];
	return '<p>' . t('It will be printed with the donor details @which FY @f:', array('@which' => $e['own'] ? t('recorded for') : t('current for'), '@f' => donation_fy_label($fy))) . '<br>'
		. check_plain($dv['dv_name'] . ', ' . $dv['dv_address'] . ' - ' . $dv['dv_pincode'] . ' ' . $dv['dv_city'] . ' ' . $dv['dv_state_code'] . ' ' . $dv['dv_country_code']
			. ($id !== '' ? ', ' . $id : '') . ', ' . $dv['dv_contact']) . '</p>'
		. '<p>' . t('The receipt number does not change.') . '</p>';    // the archive copy (P1) is kept but not mentioned to centres
}

function donation_reprint_form($form, &$form_state, $donation_id)
{
	if (!donation_versioning_require())
		return array('#markup' => '');
	$row = db_query('SELECT d_id, d_donor, d_fy, d_receipt_f, d_type FROM dh_donation WHERE d_id = :id', array(':id' => (int) $donation_id))->fetchAssoc();
	if (!$row || $row['d_type'] == 'razorpay')
	{
		drupal_set_message(t('Online (Razorpay) receipts cannot be reprinted here.'), 'error');
		drupal_goto('donation-report');
	}
	$form['donation_id'] = array('#type' => 'value', '#value' => (int) $donation_id);
	return confirm_form($form, t('Reprint receipt @r?', array('@r' => $row['d_receipt_f'])), 'donation-report',
		donation_reprint_describe($row['d_donor'], $row['d_fy']), t('Reprint receipt'), t('Cancel'));
}

function donation_reprint_form_submit($form, &$form_state)
{
	$id = (int) $form_state['values']['donation_id'];
	$form_state['redirect'] = 'donation-report';
	if (!donation_versioning_require())
		return;
	$num = (int) db_query("SELECT d_receipt_num FROM dh_donation WHERE d_id = :id AND d_deleted = '0' AND d_type <> 'razorpay'", array(':id' => $id))->fetchField();
	if ($num <= 0)
	{
		drupal_set_message(t('This receipt cannot be reprinted.'), 'error');
		return;
	}
	$r = generate_receipt($id, 1);
	if (!$r)
	{
		drupal_set_message(t('The receipt was NOT reprinted (storage error). Please try again later.'), 'error');
		return;
	}
	donation_log_reprint($id);
	drupal_set_message(t('Receipt @r reprinted.', array('@r' => $r)));
}

function donation_reprint_fy_form($form, &$form_state, $donor, $fy)
{
	if (!donation_versioning_require())
		return array('#markup' => '');
	$fy = (int) $fy;
	$skip = isset($_GET['skip']) ? (int) $_GET['skip'] : 0;          // the donation just saved: its save already reprinted it
	$ids = donation_reprint_ids($donor['d_id'], $fy, $skip);
	if (!$ids)
	{
		drupal_set_message(t('There are no receipts to reprint.'));
		drupal_goto('donors/edit/' . (int) $donor['d_id']);
	}
	$nums = db_query('SELECT d_receipt_f FROM dh_donation WHERE d_id IN (:ids) ORDER BY d_id', array(':ids' => $ids))->fetchCol();
	$form['donor_id'] = array('#type' => 'value', '#value' => (int) $donor['d_id']);
	$form['fy'] = array('#type' => 'value', '#value' => $fy);
	$form['skip'] = array('#type' => 'value', '#value' => $skip);
	$desc = donation_reprint_describe($donor['d_id'], $fy) . '<p>' . check_plain(implode(', ', $nums)) . '</p>';
	return confirm_form($form, format_plural(count($ids), 'Reprint 1 receipt of @n for FY @f?', 'Reprint @count receipts of @n for FY @f?',
		array('@n' => $donor['d_name'], '@f' => donation_fy_label($fy))), 'donors/edit/' . (int) $donor['d_id'], $desc, t('Reprint'), t('Cancel'));
}

function donation_reprint_fy_form_submit($form, &$form_state)
{
	$donor = (int) $form_state['values']['donor_id'];
	$fy = (int) $form_state['values']['fy'];
	$form_state['redirect'] = 'donors/edit/' . $donor;
	if (!donation_versioning_require())
		return;
	$ops = array();
	foreach (donation_reprint_ids($donor, $fy, (int) $form_state['values']['skip']) as $id)   // recomputed at submit
		$ops[] = array('donation_reprint_batch_op', array($id));
	batch_set(array('title' => t('Reprinting receipts of FY @f', array('@f' => donation_fy_label($fy))),
		'operations' => $ops, 'finished' => 'donation_reprint_batch_finished'));
}

function donation_reprint_batch_op($id, &$context)
{
	$r = generate_receipt($id, 1);
	if (!$r)
		$context['results']['failed'][] = $id;
	else
	{
		donation_log_reprint($id);
		$context['results']['done'][] = $r;
	}
	$context['message'] = t('Reprinted @r', array('@r' => $r ? $r : $id));
}

function donation_reprint_batch_finished($success, $results, $operations)
{
	$done = isset($results['done']) ? count($results['done']) : 0;
	$failed = isset($results['failed']) ? count($results['failed']) : 0;
	drupal_set_message(format_plural($done, '1 receipt reprinted.', '@count receipts reprinted.'));
	if ($failed || !$success)
		drupal_set_message(t('@n receipt(s) were not reprinted (storage error). Please try again later.', array('@n' => $failed)), 'error');
}

