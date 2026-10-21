<?php
/**
 * Form 10BE certificate mailer helpers (process-10be.php), donor versioning rev 3 (R12).
 * A certificate is matched to the donor of its CERTIFIED financial year (that year's details: snapshot, else
 * current) by the normalised UIN, with a mandatory name match when the UIN is shared; a single UIN match whose
 * year name differs is held for staff (N5). Only when nobody in that year has the UIN does it fall back to exactly
 * one current donor with the same UIN AND name (L31). Recipients are that year's email/contact (U2, N6).
 * PHP 7.3-compatible.
 */

function tenbe_norm($s) { return strtoupper(preg_replace('/[^A-Za-z0-9]/', '', (string) $s)); }
function tenbe_load_fy($fy, $versioned) {
  $map = array();
  $sql = $versioned
    ? "SELECT x.d_donor AS donor_id, COALESCE(dv.dv_id_no, don.d_id_no) AS id_no, COALESCE(dv.dv_name, don.d_name) AS name,
         COALESCE(dv.dv_email, don.d_email) AS email, COALESCE(dv.dv_contact, don.d_contact) AS contact
       FROM (SELECT DISTINCT d.d_donor FROM dh_donation d
              WHERE d.d_date >= :s AND d.d_date <= :e AND d.d_deleted = '0' AND d.d_foreign = '0') x
         JOIN dh_donor don ON don.d_id = x.d_donor
         LEFT JOIN dh_donor_fy dfy ON (dfy.df_donor = x.d_donor AND dfy.df_fy = :fy)
         LEFT JOIN dh_donor_version dv ON (dv.dv_id = dfy.df_version AND dv.dv_donor = dfy.df_donor)
       WHERE COALESCE(dv.dv_id_type, don.d_id_type) BETWEEN 1 AND 7"
    : "SELECT x.d_donor AS donor_id, don.d_id_no AS id_no, don.d_name AS name, don.d_email AS email, don.d_contact AS contact
       FROM (SELECT DISTINCT d.d_donor FROM dh_donation d
              WHERE d.d_date >= :s AND d.d_date <= :e AND d.d_deleted = '0' AND d.d_foreign = '0') x
         JOIN dh_donor don ON don.d_id = x.d_donor
       WHERE don.d_id_type BETWEEN 1 AND 7";
  $args = array(':s' => $fy . '-04-01', ':e' => ($fy + 1) . '-03-31');
  if ($versioned) $args[':fy'] = (int) $fy;
  foreach (db_query($sql, $args) as $r) {
    $k = tenbe_norm($r->id_no);
    if ($k !== '') $map[$k][(int) $r->donor_id] = (array) $r;
  }
  return $map;
}
function tenbe_load_live() {
  $map = array();
  foreach (db_query("SELECT d_id AS donor_id, d_id_no AS id_no, d_name AS name, d_email AS email, d_contact AS contact
                       FROM dh_donor WHERE d_deleted = 0 AND d_id_no <> ''") as $r) {
    $k = tenbe_norm($r->id_no);
    if ($k !== '') $map[$k][(int) $r->donor_id] = (array) $r;
  }
  return $map;
}
// Returns array($row|NULL, $step, $reason)
function tenbe_pick(array $fy_map, $live_loader, $pan, $file_name) {
  $k = tenbe_norm($pan); $n = tenbe_norm($file_name);
  if ($k === '') return array(NULL, '-', 'no-uin');
  if (!empty($fy_map[$k])) {
    $c = $fy_map[$k];
    if (count($c) === 1) {                                  // N5: a single match whose year name differs is HELD, not sent
      $row = reset($c);
      return tenbe_norm($row['name']) === $n ? array($row, 'fy', 'ok') : array(NULL, 'fy', 'name-differs');
    }
    $m = array();
    foreach ($c as $id => $row) if ($n !== '' && tenbe_norm($row['name']) === $n) $m[$id] = $row;
    if (count($m) === 1) return array(reset($m), 'fy', 'ok-tiebreak');
    if (count($m) > 1 && tenbe_one_recipient($m)) return array(reset($m), 'fy', 'ok-same-recipient');
    return array(NULL, 'fy', count($m) ? 'ambiguous-same-name' : 'ambiguous-no-name-match');
  }
  $live = call_user_func($live_loader);
  if (empty($live[$k])) return array(NULL, 'live', 'not-found');
  $m = array();
  foreach ($live[$k] as $id => $row) if ($n !== '' && tenbe_norm($row['name']) === $n) $m[$id] = $row;
  if (count($m) === 1) return array(reset($m), 'live', 'ok-live-name');
  if (count($m) > 1 && tenbe_one_recipient($m)) return array(reset($m), 'live', 'ok-same-recipient');
  return array(NULL, 'live', count($m) ? 'live-ambiguous' : 'live-name-mismatch');
}
function tenbe_one_recipient(array $rows) {
  $seen = array();
  foreach ($rows as $r) $seen[strtolower(trim($r['email'])) . '|' . preg_replace('/\D/', '', $r['contact'])] = 1;
  return count($seen) === 1;
}
function tenbe_parse_fy($arg, $cur) {
  if ($arg === NULL || $arg === '') return $cur - 1;
  if (!preg_match('/^(\d{4})(?:-(\d{2}|\d{4}))?$/', trim($arg), $m)) return 0;
  $fy = (int) $m[1];
  if (isset($m[2]) && $m[2] !== '' && (int) substr($m[2], -2) !== ($fy + 1) % 100) return 0;
  return ($fy >= 2000 && $fy < $cur) ? $fy : 0;
}
