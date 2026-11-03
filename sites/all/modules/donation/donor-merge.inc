<?php
/**
 * Duplicate donors (2026-11): one donor record per person for new donations. The look-alike copies are hidden
 * (d_deleted = 1, as Delete) by the central clean-up tool and remembered in dh_donor_merge / dh_donor_merge_hidden
 * (sql/DM-donor-merge.sql). A merge moves no donation and changes no year details. The DIPI sync, Upload CSV and the
 * online import match only active donors, so they never write to a hidden record (2026-11-03).
 * Hidden = d_deleted = 1 AND a row in dh_donor_merge_hidden of a merge not undone (mg_undone_at IS NULL).
 * A kept record follows the normal Delete rule (no donations: it can be deleted; user 2026-11-03).
 */

/** The History line of a merge or its undo, for the kept or a hidden record (escaped). */
function donor_merge_log_text($event, array $data, $donor_id)
{
	$kept = isset($data['kept']) ? (int) $data['kept'] : 0;
	$hidden = isset($data['hidden']) ? implode(', ', array_map('intval', (array) $data['hidden'])) : '';
	if ($event === 'Donors merged')
		return $kept === (int) $donor_id
			? check_plain(t('Duplicates of this donor were removed (donor @h, with no donations). This record was kept.', array('@h' => $hidden)))
			: check_plain(t('Removed as a duplicate of donor @k. It had no donations.', array('@k' => $kept)));
	return $kept === (int) $donor_id
		? check_plain(t('Removal of duplicates undone: donor @h is back in the lists.', array('@h' => $hidden)))
		: check_plain(t('Removal undone: this record is back in the lists.'));
}
