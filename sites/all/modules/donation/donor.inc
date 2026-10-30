<?php

/**
 * @file
 * Donor management: VIEW (list) and EDIT donors.
 *
 * No create (donors are created from the create-donation page) and no delete.
 * The edit form shows a read-only, paginated table of the donor's mapped
 * donations at the bottom, so the editor can see which donations are impacted
 * by a change to the shared donor record.
 *
 * Wired into donation.module via include_once, donation_permission()
 * ('view donors', 'edit donors') and donation_menu().
 *
 * NOTE: dh_donor now shares column names with dh_donation (d_created,
 * d_createdby, d_updatedby, d_deleted, d_deleted_by). Any query joining both
 * MUST table-qualify those columns.
 */

/**
 * Menu loader for %donor_id. Returns the donor row (with resolved country /
 * state / city names) or FALSE for an unknown / deleted donor.
 */
function donor_id_load($id)
{
	if (!is_numeric($id))
		return FALSE;

	$q = "select d.*, co.c_name as country, s.s_name as state, ci.c_city as city
		from dh_donor d
		left join dh_country co on d.d_country = co.c_code
		left join dh_state s on (d.d_state = s.s_code and d.d_country = s.s_country)
		left join dh_city ci on d.d_city = ci.c_id
		where d.d_id = :id and d.d_deleted = 0";
	$row = db_query($q, array(':id' => $id))->fetchAssoc();

	return $row ? $row : FALSE;
}

/**
 * Donor listing page: search form + paged results table.
 */
function donor_list()
{
	return drupal_get_form('donor_search_form');
}

function donor_search_form($form, &$form_state)
{
	// GET form so filters persist across pager navigation.
	$name    = isset($_GET['name'])    ? trim($_GET['name'])    : '';
	$email   = isset($_GET['email'])   ? trim($_GET['email'])   : '';
	$contact = isset($_GET['contact']) ? trim($_GET['contact']) : '';
	$idno    = isset($_GET['idno'])    ? trim($_GET['idno'])    : '';
	$id_type = isset($_GET['id_type']) ? $_GET['id_type']       : '';
	$has_don = !empty($_GET['has_don']);
	$no_don  = !empty($_GET['no_don']);
	$has_log = !empty($_GET['has_log']);
	$no_log  = !empty($_GET['no_log']);

	$form['#method'] = 'get';
	$form['#token'] = FALSE;
	$form['dana_help'] = array('#markup' => dana_help_link('donors'), '#weight' => -100);

	$form['name']    = array('#type' => 'textfield', '#title' => t('Name'),   '#size' => 20, '#default_value' => $name);
	$form['email']   = array('#type' => 'textfield', '#title' => t('Email'),  '#size' => 20, '#default_value' => $email);
	$form['contact'] = array('#type' => 'textfield', '#title' => t('Mobile'), '#size' => 15, '#default_value' => $contact);
	$form['idno']    = array('#type' => 'textfield', '#title' => t('ID No'),  '#size' => 15, '#default_value' => $idno);

	$id_type_options = array('' => t('- ID Type -'));
	foreach (db_query("select it_code, it_name from dh_id_types order by it_code") as $itr)
		$id_type_options[$itr->it_code] = $itr->it_name;
	$form['id_type'] = array('#type' => 'select', '#title' => t('ID Type'), '#options' => $id_type_options, '#default_value' => $id_type);

	$form['donations'] = array('#type' => 'fieldset', '#title' => t('Donations'), '#attributes' => array('style' => 'display:inline-block'));
	$form['donations']['has_don'] = array('#type' => 'checkbox', '#title' => t('Has donations'), '#default_value' => $has_don);
	$form['donations']['no_don']  = array('#type' => 'checkbox', '#title' => t('No donations'),  '#default_value' => $no_don);

	$form['logs'] = array('#type' => 'fieldset', '#title' => t('Change logs'), '#attributes' => array('style' => 'display:inline-block'));
	$form['logs']['has_log'] = array('#type' => 'checkbox', '#title' => t('Has change logs'), '#default_value' => $has_log);
	$form['logs']['no_log']  = array('#type' => 'checkbox', '#title' => t('No change logs'),  '#default_value' => $no_log);

	$form['go'] = array('#type' => 'submit', '#value' => t('Search'));
	$form['clear'] = array('#type' => 'markup', '#markup' => ' ' . l(t('Clear'), 'donors'));

	// Apply the search filters to a dh_donor query (alias 'd'); used for both
	// the count and the page query so they stay identical.
	$apply = function ($q) use ($name, $email, $contact, $idno, $id_type, $has_don, $no_don, $has_log, $no_log) {
		$q->condition('d.d_deleted', 0);
		if ($name != '')    $q->condition('d.d_name', '%' . db_like($name) . '%', 'LIKE');
		if ($email != '')   $q->condition('d.d_email', '%' . db_like($email) . '%', 'LIKE');
		if ($contact != '') $q->condition('d.d_contact', '%' . db_like($contact) . '%', 'LIKE');
		if ($idno != '')    $q->condition('d.d_id_no', '%' . db_like($idno) . '%', 'LIKE');
		if ($id_type !== '' && is_numeric($id_type))
			$q->condition('d.d_id_type', $id_type);
		if ($has_don && !$no_don)
			$q->where('EXISTS (SELECT 1 FROM dh_donation dd WHERE dd.d_donor = d.d_id)');
		elseif ($no_don && !$has_don)
			$q->where('NOT EXISTS (SELECT 1 FROM dh_donation dd WHERE dd.d_donor = d.d_id)');
		if ($has_log && !$no_log)
			$q->where("EXISTS (SELECT 1 FROM dh_log l WHERE l.l_module = 'dh_donor' AND l.l_identifier = d.d_id)");
		elseif ($no_log && !$has_log)
			$q->where("NOT EXISTS (SELECT 1 FROM dh_log l WHERE l.l_module = 'dh_donor' AND l.l_identifier = d.d_id)");
	};

	$limit = 50;

	// Cheap total for the pager: COUNT over dh_donor with filters only
	// (no display joins, no per-row donation subquery).
	$count_q = db_select('dh_donor', 'd');
	$apply($count_q);
	$count_q->addExpression('COUNT(*)');
	$total = (int) $count_q->execute()->fetchField();
	$page = pager_default_initialize($total, $limit);

	// Deferred join: first get just the donor IDs for this page from dh_donor
	// alone (joins would defeat the idx_deleted_name index-ordered LIMIT and
	// force a filesort over the whole result set on large centres).
	$id_query = db_select('dh_donor', 'd');
	$id_query->addField('d', 'd_id');
	$apply($id_query);
	$id_query->orderBy('d.d_name');
	$id_query->range($page * $limit, $limit);
	$page_ids = $id_query->execute()->fetchCol();

	$donors = array();
	$counts = array();
	if ($page_ids) {
		// Fetch display data (with the lookup joins) for just those <=50 donors.
		$query = db_select('dh_donor', 'd');
		$query->leftJoin('dh_country', 'co', 'd.d_country = co.c_code');
		$query->leftJoin('dh_state', 's', 'd.d_state = s.s_code AND d.d_country = s.s_country');
		$query->leftJoin('dh_city', 'ci', 'd.d_city = ci.c_id');
		$query->leftJoin('dh_id_types', 'it', 'd.d_id_type = it.it_code');
		$query->fields('d', array('d_id', 'd_name', 'd_gender', 'd_contact', 'd_email', 'd_id_no', 'd_pincode'));
		$query->addField('it', 'it_name', 'it_name');
		$query->addField('co', 'c_name', 'country');
		$query->addField('s', 's_name', 'state');
		$query->addField('ci', 'c_city', 'city');
		$query->condition('d.d_id', $page_ids, 'IN');
		$query->orderBy('d.d_name');
		$donors = $query->execute()->fetchAll();

		// Donation counts for the same donors (one grouped query).
		$cres = db_query("SELECT d_donor, COUNT(*) AS c FROM dh_donation WHERE d_donor IN (:ids) GROUP BY d_donor", array(':ids' => $page_ids));
		foreach ($cres as $cr)
			$counts[$cr->d_donor] = (int) $cr->c;
	}

	$can_edit = user_access('edit donors');
	$can_delete = user_access('delete donors');

	$header = array(
		t('Name'), t('Gender'), t('Contact'), t('Email'),
		t('ID Type'), t('ID No'), t('Location'), t('Donations'), t('Actions'),
	);

	$rows = array();
	foreach ($donors as $r) {
		$total_donations = isset($counts[$r->d_id]) ? $counts[$r->d_id] : 0;

		$location = trim(implode(', ', array_filter(array($r->city, $r->state, $r->country))));
		if ($r->d_pincode)
			$location .= ' - ' . $r->d_pincode;

		$actions = array();
		$actions[] = l(t('History'), 'donors/log/' . $r->d_id);
		if ($can_edit)
			$actions[] = l(t('Edit'), 'donors/edit/' . $r->d_id);
		if ($can_delete) {
			if ($total_donations > 0)
				$actions[] = '<span class="donor-delete-blocked" title="' . t('Has mapped donations') . '">' . t('Delete') . '</span>';
			else
				$actions[] = l(t('Delete'), 'donors/delete/' . $r->d_id, array('attributes' => array('class' => array('donor-delete'))));
		}

		// Donation count links to the donations page only for edit users.
		$donations_cell = $total_donations;
		if ($total_donations > 0 && $can_edit)
			$donations_cell = l($total_donations, 'donors/donations/' . $r->d_id,
				array('attributes' => array('title' => t('View mapped donations'))));

		$rows[] = array(
			check_plain($r->d_name),
			check_plain($r->d_gender),
			check_plain($r->d_contact),
			check_plain($r->d_email),
			check_plain($r->it_name),
			check_plain($r->d_id_no),
			check_plain($location),
			$donations_cell,
			implode(' | ', $actions),
		);
	}

	$out = theme('table', array(
		'header' => $header,
		'rows' => $rows,
		'empty' => t('No donors found.'),
	));
	$out .= theme('pager');

	$form['results'] = array('#type' => 'markup', '#markup' => $out, '#weight' => 100);

	return $form;
}

/**
 * Edit donor form. $donor is the loaded row (from the %donor_id loader).
 */
function donor_form($form, &$form_state, $donor = NULL)
{
	if (!$donor)
		return array('#markup' => t('Donor not found.'));
	if (!donation_versioning_require(NULL, 'read'))
		return array('#markup' => '');

	// Donor versioning: the form shows (and the signed baseline records) the CURRENT details as one dv record (L22).
	$live = donor_live_dv($donor['d_id']);
	$show = donor_display_from_row($live, array('d_gender' => 'male'));
	$id_types = donor_id_type_options($show['d_id_type']);
	$cur = donation_cur_fy();

	$d = function ($field, $default = '') use ($show) {
		return (isset($show[$field]) && $show[$field] !== '') ? $show[$field] : $default;
	};

	$form['#attributes'] = array('class' => array('donor-form'));
	$form['dana_help'] = array('#markup' => dana_help_link('donors', 'edit'), '#weight' => -100);

	$form['d_id'] = array('#type' => 'hidden', '#value' => $donor['d_id']);
	// M4: what this form showed (fy 0 = the current record); checked in donor_form_validate()
	$enc = donor_baseline_encode($donor['d_id'], 0, array('dv' => $live, 'own' => FALSE, 'since' => NULL), $live);
	$form['donor_baseline'] = array('#type' => 'hidden', '#value' => $enc['json']);
	$form['donor_baseline_sig'] = array('#type' => 'hidden', '#value' => $enc['sig']);
	$form['form_cur_fy'] = array('#type' => 'hidden', '#value' => $cur);
	$note = donor_apr_may_note($donor['d_id']);
	if ($note !== '')
		$form['apr_may_note'] = array('#markup' => '<div class="messages warning">' . check_plain($note) . '</div>', '#weight' => -10);

	$form['d_name'] = array('#type' => 'textfield', '#title' => t('Donor Name'), '#size' => 50,
		'#required' => TRUE, '#default_value' => $d('d_name'));

	$form['d_gender'] = array('#type' => 'radios', '#title' => t('Gender'), '#required' => TRUE,
		'#options' => array('male' => t('Male'), 'female' => t('Female'), 'ms' => t('Organization')),
		'#default_value' => $d('d_gender', 'male'));

	$form['d_id_type'] = array('#type' => 'select', '#title' => t('ID Type'), '#validated' => TRUE,
		'#options' => $id_types, '#required' => TRUE, '#default_value' => $d('d_id_type', '1'));

	$form['d_id_no'] = array('#type' => 'textfield', '#title' => t('ID No'), '#size' => 20,
		'#default_value' => $d('d_id_no'));

	$form['d_contact'] = array('#type' => 'textfield', '#title' => t('Contact No'), '#size' => 30,
		'#required' => TRUE, '#default_value' => $d('d_contact'));

	$form['d_email'] = array('#type' => 'textfield', '#title' => t('Email'), '#size' => 45,
		'#default_value' => $d('d_email'));

	$form['d_pincode'] = array('#type' => 'textfield', '#title' => t('Post/Pin/Zip Code'), '#size' => 15,
		'#required' => TRUE, '#default_value' => $d('d_pincode'));

	$form['d_country'] = array('#type' => 'textfield', '#title' => t('Country'), '#size' => 30,
		'#required' => TRUE, '#default_value' => $d('d_country'));

	$form['d_state'] = array('#type' => 'textfield', '#title' => t('State'), '#size' => 30,
		'#required' => TRUE, '#default_value' => $d('d_state'));

	$form['d_city'] = array('#type' => 'textfield', '#title' => t('City'), '#size' => 30,
		'#default_value' => $d('d_city'));

	$form['d_address'] = array('#type' => 'textarea', '#title' => t('Address'), '#rows' => 2,
		'#maxlength' => 200, '#default_value' => $d('d_address'));

	$form['d_occupation'] = array('#type' => 'textfield', '#title' => t('Occupation'), '#size' => 40,
		'#default_value' => $d('d_occupation'));

	// F10 apply-to-FY: past financial years with donations; ticking one makes that year use the details saved here.
	$past = array();
	$apply_opts = array();
	foreach (donor_fy_rows($donor['d_id']) as $r)
	{
		$fy = (int) $r['d_fy'];
		if ($fy <= 0 || $fy >= $cur)
			continue;
		$past[] = array('fy' => $fy, 'label' => donation_fy_label($fy), 'n' => (int) $r['n_all']);
		$apply_opts[$fy] = 'FY ' . donation_fy_label($fy) . ' - '
			. ($r['fy_own'] ? t('own details since @d', array('@d' => date('d/m/Y', strtotime($r['df_updated'])))) : t('follows current details'))
			. (donation_10bd_due_passed($fy) ? ' ' . t('(10BD due date passed)') : '');
	}
	if ($apply_opts)
		$form['apply_fy'] = array('#type' => 'checkboxes', '#title' => t('Also use these details for past financial years'),
			'#options' => $apply_opts, '#weight' => 45,
			'#description' => t('Only tick a year to replace the details that year keeps (for example to correct a mistake in it). Unticked years keep their own details.'));

	$form['submit'] = array('#type' => 'submit', '#value' => t('Update Donor'), '#weight' => 50);
	$form['cancel'] = array('#type' => 'markup', '#markup' => ' ' . l(t('Cancel'), 'donors'), '#weight' => 51);

	// Link to this donor's full change history.
	$form['history_link'] = array(
		'#type' => 'item',
		'#markup' => l(t('View change history'), 'donors/log/' . $donor['d_id']),
		'#weight' => 99,
	);

	// L28: which details each financial year uses
	$form['fy_summary'] = array('#type' => 'item', '#title' => t('Donor details by financial year'),
		'#markup' => donor_fy_summary_table($donor['d_id'], donor_fy_summary_live($live)), '#weight' => 98);

	// Read-only, paginated list of donations impacted by editing this donor.
	$form['mapped'] = array(
		'#type' => 'item',
		'#title' => t('Donations mapped to this donor (read-only)'),
		'#markup' => donor_mapped_donations_table($donor['d_id']),
		'#weight' => 100,
	);

	// Count of donations mapped to this donor -> "shared donor" warning.
	$don_count = (int) db_query("SELECT COUNT(*) FROM dh_donation WHERE d_donor = :id",
		array(':id' => $donor['d_id']))->fetchField();

	// Pincode autofill + PAN/Aadhaar ID validation + shared-donor warning.
	$js = '(function ($) {
		var donor_don_count = ' . (int) $don_count . ';
		var donor_past_fys = ' . drupal_json_encode($past) . ';
		var donor_freeze = ' . (donation_versioning_freeze_active() ? 1 : 0) . ';
		function donor_check_id() {
			if ($("#edit-d-id-type").val() == "99") return true;
			// the number as it will be saved (see donation_id_check_js)
			var idv = DanaId.clean($("#edit-d-id-no").val());
			if (idv == "") {
				alert("ID No is mandatory"); $("#edit-d-id-no").focus(); return false;
			}
			var problem = DanaId.problem($("#edit-d-id-type").val(), idv);
			if (problem != "") {
				alert(problem); $("#edit-d-id-no").focus(); return false;
			}
			return true;
		}
		$(document).ready(function () {
			DanaId.bind("#edit-d-id-no");
			$("#edit-d-pincode").on("blur", function () {
				var code = $.trim($(this).val());
				if (code == "") return;
				$.getJSON("/get-location-from-pincode", {code: code}, function (data) {
					if (data && data.length > 0) {
						if ($("#edit-d-country").val() == "") $("#edit-d-country").val(data[0].country);
						if ($("#edit-d-state").val() == "") $("#edit-d-state").val(data[0].state);
						if ($("#edit-d-city").val() == "") $("#edit-d-city").val(data[0].city);
					}
				});
			});
			$("#donor-form").submit(function (e) {
				if (! donor_check_id()) { e.preventDefault(); return false; }
				var changed = false;
				$("#donor-form").find("input[type=text], textarea").each(function () { if (this.value != this.defaultValue) changed = true; });
				$("#donor-form").find("select option").each(function () { if (this.selected != this.defaultSelected) changed = true; });
				$("#donor-form").find("input[type=radio]").each(function () { if (this.checked != this.defaultChecked) changed = true; });
				var ticked = [];
				$("#donor-form").find("input[type=checkbox][name^=apply_fy]:checked").each(function () { ticked.push(parseInt($(this).val(), 10)); });
				var msg = "";
				if (donor_freeze && donor_past_fys.length && (changed || ticked.length)) {
					var keep = [], use = [];
					$.each(donor_past_fys, function (i, p) {
						var s = "FY " + p.label + " (" + p.n + ")";
						if ($.inArray(p.fy, ticked) >= 0) use.push(s); else keep.push(s);
					});
					if (keep.length) msg += "These past years keep the details they have now: " + keep.join(", ") + ".\n";
					if (use.length) msg += "These past years will use the new details: " + use.join(", ") + ".\n";
					msg += "Receipts of this year show the new details. Receipts already issued are not changed and nothing is reprinted automatically; if any of them now differ, you will get an optional link to reprint them after saving.\n\nDo you want to continue?";
				}
				else if (!donor_freeze && changed && donor_don_count > 1)
					msg = "This donor is linked to " + donor_don_count + " donations. Your changes update the donor details shared by all of them. Existing receipts will NOT be changed.\n\nDo you want to continue?";
				if (msg && ! confirm(msg)) {
					e.preventDefault();
					return false;
				}
			});
		});
	})(jQuery);';
	drupal_add_js(donation_id_check_js() . $js, 'inline');

	return $form;
}

function donor_form_validate($form, &$form_state)
{
	$v = $form_state['values'];
	if (trim($v['d_name']) == '')
		form_set_error('d_name', t('Donor name is required.'));
	if (trim($v['d_contact']) == '')
		form_set_error('d_contact', t('Contact number is required.'));
	if (trim($v['d_email']) != '' && !valid_email_address(trim($v['d_email'])))
		form_set_error('d_email', t('Please enter a valid email address.'));
	if (!donation_versioning_require('d_name'))
		return;
	$in = $form_state['input'];
	$b = donor_baseline_decode(isset($in['donor_baseline']) ? $in['donor_baseline'] : '', isset($in['donor_baseline_sig']) ? $in['donor_baseline_sig'] : '');
	if (!$b || (int) $b['donor'] !== (int) $v['d_id'])
	{
		form_set_error('d_name', t('This form is out of date. Please reload the page and enter the changes again.'));
		return;
	}
	$cur = donation_cur_fy();
	if (isset($in['form_cur_fy']) && (int) $in['form_cur_fy'] !== $cur)
	{
		form_set_error('d_name', t('The financial year changed while this form was open. Please reload the page and enter the changes again.'));
		return;
	}
	$defaults = array('d_gender' => 'male');
	$groups = donor_changed_groups($v, $b['live'], 'donor', $defaults);
	if (in_array('id', $groups, TRUE) && !donor_id_type_allowed($v['d_id_type'], $b['live']))
		form_set_error('d_id_type', t('Please choose a valid ID type.'));
	list(, $errs) = donor_form_new_values($v, $b['live'], $groups, FALSE, 'donor');
	foreach ($errs as $k => $m)
		form_set_error($k, $m);
	$apply = array();
	if (!empty($v['apply_fy']))
		foreach ($v['apply_fy'] as $fy => $on)
		{
			if (!$on)
				continue;
			$fy = (int) $fy;
			$has = db_query('SELECT 1 FROM dh_donation WHERE d_donor = :d AND d_fy = :f LIMIT 1', array(':d' => (int) $v['d_id'], ':f' => $fy))->fetchField();
			if ($fy <= 0 || $fy >= $cur || !$has)
				form_set_error('apply_fy', t('FY @f cannot be changed here.', array('@f' => donation_fy_label($fy))));
			else
				$apply[] = $fy;
		}
	$form_state['donor_plan'] = array('donor' => (int) $v['d_id'], 'baseline' => $b, 'groups' => $groups, 'apply_fys' => $apply, 'cur' => $cur);
}

/** HEAD donor.inc:378-417. */
function donor_form_submit($form, &$form_state)
{
	global $user;
	$p = $form_state['donor_plan'];
	$id = $p['donor'];
	$applied = array();
	$w = array('cols' => array(), 'conflicts' => array());
	$txn = db_transaction();
	try
	{
		$now = donor_live_dv($id, TRUE);
		if (!$now)
			throw new Exception(t('This donor no longer exists.'));
		$before = donor_fy_printed_map($id);
		if ($p['groups'])
		{
			list($new) = donor_form_new_values($form_state['values'], $p['baseline']['live'], $p['groups'], TRUE, 'donor');
			$w = donor_write_live_groups($id, $now, $new, $p['groups'], $p['baseline']['live'], $user->uid, 'donor');
		}
		foreach ($p['apply_fys'] as $fy)                                   // after the UPDATE: the trigger froze the other years at OLD
			if (donor_apply_to_fy($id, $fy, $user->uid))
				$applied[] = donation_fy_label($fy);
		if (donation_cur_fy(TRUE) !== $p['cur'])
		{
			$txn->rollback();
			drupal_set_message(t('The financial year changed while saving. Nothing was saved; please check and save again.'), 'error');
			$form_state['rebuild'] = TRUE;
			return;
		}
	}
	catch (Throwable $e)   // PHP 7: also TypeError; never let $txn commit half a submit
	{
		$txn->rollback();
		watchdog('donation', 'Save rolled back: @m (@f:@l)', array('@m' => $e->getMessage(), '@f' => $e->getFile(), '@l' => $e->getLine()), WATCHDOG_ERROR);   // watchdog_exception() accepts Exception only
		drupal_set_message(t('The donor could not be saved; nothing was changed. (@m)', array('@m' => $e->getMessage())), 'error');
		$form_state['rebuild'] = TRUE;
		return;
	}
	unset($txn);
	drupal_set_message($w['cols'] ? t('Donor %name updated.', array('%name' => $form_state['values']['d_name'])) : t('No donor details were changed.'));
	if ($applied)
		drupal_set_message(t('These past years now use the saved details: FY @f.', array('@f' => implode(', FY ', $applied))));
	if ($w['conflicts'])
		drupal_set_message(donor_conflict_message($id, $w['conflicts'], $p['baseline']['ts']), 'warning');
	donor_reprint_offer($id, $before, 0);
	$form_state['redirect'] = 'donors';
}

/**
 * Read-only, paginated table of donations mapped to a donor (all statuses).
 */
function donor_mapped_donations_table($donor_id)
{
	$header = array(t('Receipt No'), t('Date'), t('Amount'), t('Currency'),
		t('Type'), t('Category'), t('Status'), t('FY'), t('Donor details'));

	$query = db_select('dh_donation', 'd')->extend('PagerDefault');
	$query->fields('d', array('d_id', 'd_receipt_f', 'd_date', 'd_amount', 'd_currency', 'd_type', 'd_category', 'd_deleted', 'd_fy'));
	// L28: which donor details the donation's financial year uses
	$query->leftJoin('dh_donor_fy', 'dfy', 'dfy.df_donor = d.d_donor AND dfy.df_fy = d.d_fy');
	$query->leftJoin('dh_donor_version', 'dv', 'dv.dv_id = dfy.df_version AND dv.dv_donor = dfy.df_donor');
	$query->addExpression('(dv.dv_id IS NOT NULL)', 'fy_own');
	$query->condition('d.d_donor', $donor_id);
	$query->orderBy('d.d_date', 'DESC')->orderBy('d.d_id', 'DESC');
	$query->limit(10);
	$result = $query->execute();

	$rows = array();
	foreach ($result as $r) {
		$receipt = check_plain($r->d_receipt_f);
		if ($r->d_receipt_f && user_access('download receipt'))
			$receipt = l($r->d_receipt_f, 'download-receipt/' . $r->d_id);

		$rows[] = array(
			$receipt,
			$r->d_date ? date('d/m/Y', strtotime($r->d_date)) : '',
			number_format($r->d_amount),
			check_plain($r->d_currency),
			check_plain(ucfirst($r->d_type)),
			check_plain($r->d_category),
			$r->d_deleted ? t('Cancelled') : t('Active'),
			$r->d_fy > 0 ? donation_fy_label((int) $r->d_fy) : '',
			$r->fy_own ? t('Own (that year)') : t('Current'),
		);
	}

	$out = '<div class="donor-mapped-donations">';
	$out .= theme('table', array('header' => $header, 'rows' => $rows,
		'empty' => t('No donations mapped to this donor.')));
	$out .= theme('pager');
	$out .= '</div>';

	return $out;
}

/**
 * Read-only page listing the donations mapped to a donor.
 */
function donor_donations($donor)
{
	if (!$donor)
		return MENU_NOT_FOUND;
	if (!donation_versioning_require(NULL, 'read'))   // the table joins the versioning tables
		return '';

	drupal_set_title(t('Donations for @name', array('@name' => $donor['d_name'])));
	$out = donor_mapped_donations_table($donor['d_id']);
	$out .= '<p>' . l(t('&laquo; Back to donors'), 'donors', array('html' => TRUE)) . '</p>';
	return $out;
}

/**
 * Soft-delete a donor. Blocked when ANY donation (including cancelled) is
 * mapped. Sets d_deleted = 1, d_deleted_by = uid.
 */
/**
 * Donors > Delete: a confirm page (2026-10-30). Opening the address only asks; the donor is hidden only by the page's
 * form, which carries Drupal's security code, so a link or page elsewhere cannot delete donors.
 */
function donor_delete($donor)
{
	if (!$donor) {
		drupal_set_message(t('Donor not found.'), 'error');
		drupal_goto('donors');
	}
	return drupal_get_form('donor_delete_confirm', $donor);
}

function donor_delete_donation_count($donor_id)
{
	return (int) db_query('SELECT COUNT(*) FROM dh_donation WHERE d_donor = :id', array(':id' => (int) $donor_id))->fetchField();
}

function donor_delete_confirm($form, &$form_state, $donor)
{
	$count = donor_delete_donation_count($donor['d_id']);
	if ($count > 0) {
		drupal_set_message(t('Cannot delete donor %name: @n donation(s) are mapped to this donor.',
			array('%name' => $donor['d_name'], '@n' => $count)), 'error');
		$form['back'] = array('#markup' => '<p>' . l(t('Back to the donors list'), 'donors') . '</p>');
		return $form;
	}
	$form['d_id'] = array('#type' => 'value', '#value' => (int) $donor['d_id']);
	$form['d_name'] = array('#type' => 'value', '#value' => $donor['d_name']);
	return confirm_form($form, t('Delete donor %name?', array('%name' => $donor['d_name'])), 'donors',
		t('Dana will hide this donor from the donors list and the donation form. Their details stay in the records.'),
		t('Delete'), t('Cancel'));
}

function donor_delete_confirm_submit($form, &$form_state)
{
	global $user;
	$id = (int) $form_state['values']['d_id'];
	$name = $form_state['values']['d_name'];
	$form_state['redirect'] = 'donors';
	$count = donor_delete_donation_count($id);
	if ($count > 0) {                                   // a donation was added in the meantime
		drupal_set_message(t('Cannot delete donor %name: @n donation(s) are mapped to this donor.', array('%name' => $name, '@n' => $count)), 'error');
		return;
	}
	db_update('dh_donor')
		->fields(array('d_deleted' => 1, 'd_deleted_by' => $user->uid))
		->condition('d_id', $id)
		->condition('d_deleted', 0)
		->execute();
	drupal_set_message(t('Donor %name deleted.', array('%name' => $name)));
}

/**
 * Human label for a dh_donor column (used in the change-log).
 */
function donor_changelog_field_label($col)
{
	$map = array(
		'd_name' => 'Name', 'd_gender' => 'Gender', 'd_address' => 'Address',
		'd_pincode' => 'Pincode', 'd_contact' => 'Contact', 'd_email' => 'Email',
		'd_occupation' => 'Occupation', 'd_country' => 'Country', 'd_state' => 'State',
		'd_city' => 'City', 'd_id_type' => 'ID Type', 'd_id_no' => 'ID No',
		'd_course' => 'Course', 'd_address_proof' => 'Address Proof',
		'd_confirmation' => 'Confirmation', 'd_error' => 'Error',
	);
	return isset($map[$col]) ? $map[$col] : $col;
}

/**
 * Read-only, paginated change-log table for a donor (What / When / Who).
 * Data comes from dh_log rows written by the dh_donor_au trigger
 * (l_msg is a JSON {"old":{...},"new":{...}} diff of changed columns).
 */
function donor_changelog_table($donor_id)
{
	$header = array(t('What changed'), t('When'), t('Who'));

	$query = db_select('dh_log', 'l')->extend('PagerDefault');
	$query->leftJoin('users', 'u', 'l.l_user = u.uid');
	$query->fields('l', array('l_id', 'l_msg', 'l_tstamp', 'l_user', 'l_event'));
	$query->addField('u', 'name', 'uname');
	$query->condition('l.l_module', 'dh_donor');
	$query->condition('l.l_identifier', $donor_id);
	$query->orderBy('l.l_id', 'DESC');
	$query->limit(20);
	$result = $query->execute();

	// ID-type code -> name, so the log shows "PAN" not "1".
	$id_types = array();
	foreach (db_query("select it_code, it_name from dh_id_types") as $it)
		$id_types[$it->it_code] = $it->it_name;

	// Internal/scratch columns that are not meaningful donor changes.
	$skip_cols = array('d_id' => 1, 'd_error' => 1);

	$rows = array();
	// Generic rows store codes for country/state/city: show their names (per-request caches).
	$country_names = $state_names = $city_names = array();
	$code_name = function ($col, $val, $row_country) use (&$country_names, &$state_names, &$city_names) {
		if ($val === '' || $val === NULL)
			return $val;
		if ($col == 'd_country') {
			if (!isset($country_names[$val]))
				$country_names[$val] = db_query('SELECT c_name FROM dh_country WHERE c_code = :c ORDER BY c_id LIMIT 1', array(':c' => $val))->fetchField();
			return $country_names[$val] ? $country_names[$val] : $val;
		}
		if ($col == 'd_state') {
			$k = $row_country . '|' . $val;
			if (!isset($state_names[$k]))
				$state_names[$k] = db_query('SELECT s_name FROM dh_state WHERE s_code = :s ORDER BY (s_country = :c) DESC, s_id LIMIT 1', array(':s' => $val, ':c' => (string) $row_country))->fetchField();
			return $state_names[$k] ? $state_names[$k] : $val;
		}
		if ($col == 'd_city') {
			if (!isset($city_names[$val]))
				$city_names[$val] = db_query('SELECT c_city FROM dh_city WHERE c_id = :i', array(':i' => (int) $val))->fetchField();
			return $city_names[$val] ? $city_names[$val] : $val;
		}
		return $val;
	};
	// A row logs only the changed columns, so a state code is resolved in the country the donor had then:
	// the "old" country of the first later row that changed the country, else the current country.
	$country_changes = array();
	foreach (db_query("SELECT l_id, l_msg FROM dh_log WHERE l_module = 'dh_donor' AND l_identifier = :d AND l_msg LIKE :p
		AND (l_event IS NULL OR l_event NOT LIKE 'FY %') ORDER BY l_id", array(':d' => $donor_id, ':p' => '%"d_country"%')) as $c) {
		$m = json_decode($c->l_msg, TRUE);
		if (isset($m['old']['d_country']))
			$country_changes[(int) $c->l_id] = (string) $m['old']['d_country'];
	}
	$cur_country = (string) db_query('SELECT d_country FROM dh_donor WHERE d_id = :d', array(':d' => $donor_id))->fetchField();
	$country_after = function ($l_id) use ($country_changes, $cur_country) {
		foreach ($country_changes as $id => $old)
			if ($id > $l_id)
				return $old;
		return $cur_country;
	};

	foreach ($result as $r) {
		$data = json_decode($r->l_msg, TRUE);
		// Donor versioning events (freeze, year correction, apply-to-FY, skipped freeze): one readable sentence (already escaped).
		if ($r->l_event !== NULL && strncmp($r->l_event, 'FY ', 3) === 0) {
			$what = donor_changelog_fy_text($r->l_event, is_array($data) ? $data : array(), $donor_id);
			$who = ($r->l_user == 0)
				? t('System / Sync')
				: ($r->uname !== NULL ? check_plain($r->uname) : t('uid @u', array('@u' => $r->l_user)));
			$rows[] = array($what, check_plain($r->l_tstamp), $who);
			continue;
		}
		if ($data && isset($data['new']) && is_array($data['new'])) {
			$parts = array();
			foreach ($data['new'] as $col => $newval) {
				if (isset($skip_cols[$col]))
					continue;
				$oldval = isset($data['old'][$col]) ? $data['old'][$col] : '';
				if ($col == 'd_id_type') {
					$oldval = isset($id_types[$oldval]) ? $id_types[$oldval] : $oldval;
					$newval = isset($id_types[$newval]) ? $id_types[$newval] : $newval;
				}
				elseif ($col == 'd_country' || $col == 'd_state' || $col == 'd_city') {
					$nc = isset($data['new']['d_country']) ? $data['new']['d_country'] : $country_after((int) $r->l_id);
					$oc = isset($data['old']['d_country']) ? $data['old']['d_country'] : $nc;
					$oldval = $code_name($col, $oldval, $oc);
					$newval = $code_name($col, $newval, $nc);
				}
				$parts[] = '<strong>' . check_plain(donor_changelog_field_label($col)) . '</strong>: "'
					. check_plain($oldval) . '" &rarr; "' . check_plain($newval) . '"';
			}
			// Nothing meaningful changed (e.g. only the internal d_error field) — skip.
			if (empty($parts))
				continue;
			$what = implode('<br>', $parts);
		}
		else {
			$what = check_plain($r->l_msg);
		}

		$who = ($r->l_user == 0)
			? t('System / Sync')
			: ($r->uname !== NULL ? check_plain($r->uname) : t('uid @u', array('@u' => $r->l_user)));

		$rows[] = array($what, check_plain($r->l_tstamp), $who);
	}

	$out = '<div class="donor-changelog">';
	$out .= theme('table', array('header' => $header, 'rows' => $rows,
		'empty' => t('No change history for this donor.')));
	$out .= theme('pager');
	$out .= '</div>';

	return $out;
}

/**
 * Donor change-history page.
 */
function donor_changelog($donor)
{
	if (!$donor)
		return MENU_NOT_FOUND;
	if (!donation_versioning_require(NULL, 'read'))   // FY log rows read the snapshots
		return '';

	drupal_set_title(t('Change history: @name', array('@name' => $donor['d_name'])));
	$out = '';
	$live = donor_live_dv($donor['d_id']);
	if ($live)
		$out .= '<h3>' . t('Donor details by financial year') . '</h3>' . donor_fy_summary_table($donor['d_id'], donor_fy_summary_live($live));
	$out .= donor_changelog_table($donor['d_id']);
	$out .= '<p>' . l(t('&laquo; Back to donors'), 'donors', array('html' => TRUE)) . '</p>';
	return $out;
}
