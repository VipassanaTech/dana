<?php
/**
 * Duplicate donors (2026-11): one donor record per person for new donations. The look-alike copies are hidden
 * (d_deleted = 1, as Delete) by the central clean-up tool and remembered in dh_donor_merge / dh_donor_merge_hidden
 * (sql/DM-donor-merge.sql). A merge moves no donation and changes no year details. The DIPI sync, Upload CSV and the
 * online import match only active donors, so they never write to a hidden record (2026-11-03).
 * Hidden = d_deleted = 1 AND a row in dh_donor_merge_hidden of a merge not undone (mg_undone_at IS NULL).
 */

/** TRUE when this site has the merge tables. */
function donor_merge_ready()
{
	$ok = &drupal_static(__FUNCTION__);
	if ($ok === NULL)
		$ok = db_table_exists('dh_donor_merge') && db_table_exists('dh_donor_merge_hidden');
	return $ok;
}

/** The record new donations of $donor_id go to: itself, or the kept record of the merge that hid it (through a chain). */
function donor_merge_kept_of($donor_id)
{
	$id = (int) $donor_id;
	if (!donor_merge_ready())
		return $id;
	for ($i = 0; $i < 20; $i++)
	{
		$k = db_query('SELECT mg.mg_kept FROM dh_donor_merge_hidden mh JOIN dh_donor_merge mg ON mg.mg_id = mh.mh_merge
			WHERE mh.mh_donor = :d AND mg.mg_undone_at IS NULL ORDER BY mg.mg_id DESC LIMIT 1', array(':d' => $id))->fetchField();
		if (!$k)
			return $id;
		$id = (int) $k;
	}
	return $id;
}

/** TRUE when other records were combined into $donor_id (merge not undone): it cannot be deleted. */
function donor_merge_is_kept($donor_id)
{
	return donor_merge_ready() && (bool) db_query('SELECT 1 FROM dh_donor_merge WHERE mg_kept = :d AND mg_undone_at IS NULL LIMIT 1',
		array(':d' => (int) $donor_id))->fetchField();
}

/** The History line of a merge or its undo, for the kept or a hidden record (escaped). */
function donor_merge_log_text($event, array $data, $donor_id)
{
	$kept = isset($data['kept']) ? (int) $data['kept'] : 0;
	$hidden = isset($data['hidden']) ? implode(', ', array_map('intval', (array) $data['hidden'])) : '';
	if ($event === 'Donors merged')
		return $kept === (int) $donor_id
			? check_plain(t('Duplicate records of this donor (donor @h, with no donations) were combined into this record.', array('@h' => $hidden)))
			: check_plain(t('Duplicate record: combined into donor @k. It had no donations.', array('@k' => $kept)));
	return $kept === (int) $donor_id
		? check_plain(t('Combining undone: donor @h is back in the lists.', array('@h' => $hidden)))
		: check_plain(t('Combining undone: this record is back in the lists.'));
}
