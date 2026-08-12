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
				$actions[] = l(t('Delete'), 'donors/delete/' . $r->d_id, array('attributes' => array('class' => array('donor-delete'), 'onclick' => 'return confirm("Delete this donor?");')));
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

	$id_types = array();
	foreach (db_query("select it_code, it_name from dh_id_types order by it_code") as $row)
		$id_types[$row->it_code] = $row->it_name;

	$d = function ($field, $default = '') use ($donor) {
		return (isset($donor[$field]) && $donor[$field] !== '') ? $donor[$field] : $default;
	};

	$form['#attributes'] = array('class' => array('donor-form'));

	$form['d_id'] = array('#type' => 'hidden', '#value' => $donor['d_id']);

	$form['d_name'] = array('#type' => 'textfield', '#title' => t('Donor Name'), '#size' => 50,
		'#required' => TRUE, '#default_value' => $d('d_name'));

	$form['d_gender'] = array('#type' => 'radios', '#title' => t('Gender'), '#required' => TRUE,
		'#options' => array('male' => t('Male'), 'female' => t('Female'), 'ms' => t('Organization')),
		'#default_value' => $d('d_gender', 'male'));

	$form['d_id_type'] = array('#type' => 'select', '#title' => t('ID Type'),
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
		'#required' => TRUE, '#default_value' => $d('country'));

	$form['d_state'] = array('#type' => 'textfield', '#title' => t('State'), '#size' => 30,
		'#required' => TRUE, '#default_value' => $d('state'));

	$form['d_city'] = array('#type' => 'textfield', '#title' => t('City'), '#size' => 30,
		'#default_value' => $d('city'));

	$form['d_address'] = array('#type' => 'textarea', '#title' => t('Address'), '#rows' => 2,
		'#maxlength' => 200, '#default_value' => $d('d_address'));

	$form['d_occupation'] = array('#type' => 'textfield', '#title' => t('Occupation'), '#size' => 40,
		'#default_value' => $d('d_occupation'));

	$form['submit'] = array('#type' => 'submit', '#value' => t('Update Donor'), '#weight' => 50);
	$form['cancel'] = array('#type' => 'markup', '#markup' => ' ' . l(t('Cancel'), 'donors'), '#weight' => 51);

	// Link to this donor's full change history.
	$form['history_link'] = array(
		'#type' => 'item',
		'#markup' => l(t('View change history'), 'donors/log/' . $donor['d_id']),
		'#weight' => 99,
	);

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
		var aadhaar_d = [
		  [0,1,2,3,4,5,6,7,8,9],[1,2,3,4,0,6,7,8,9,5],[2,3,4,0,1,7,8,9,5,6],
		  [3,4,0,1,2,8,9,5,6,7],[4,0,1,2,3,9,5,6,7,8],[5,9,8,7,6,0,4,3,2,1],
		  [6,5,9,8,7,1,0,4,3,2],[7,6,5,9,8,2,1,0,4,3],[8,7,6,5,9,3,2,1,0,4],
		  [9,8,7,6,5,4,3,2,1,0]
		];
		var aadhaar_p = [
		  [0,1,2,3,4,5,6,7,8,9],[1,5,7,6,2,8,3,0,9,4],[5,8,0,3,7,9,6,1,4,2],
		  [8,9,1,6,0,4,3,5,2,7],[9,4,5,3,1,2,6,8,7,0],[4,2,8,6,5,7,3,9,0,1],
		  [2,7,9,3,8,0,6,4,1,5],[7,0,4,6,9,1,3,2,5,8]
		];
		function inv_array(array) {
			if (Object.prototype.toString.call(array) === "[object Number]") array = String(array);
			if (Object.prototype.toString.call(array) === "[object String]") array = array.split("").map(Number);
			return array.reverse();
		}
		function validate_aadhaar(array) {
			var c = 0;
			var inverted = inv_array(array);
			for (var i = 0; i < inverted.length; i++) c = aadhaar_d[c][aadhaar_p[(i % 8)][inverted[i]]];
			return (c === 0);
		}
		function aadhaar_is_valid(aadhaar) {
			if (aadhaar == "" || aadhaar.length != 12) return false;
			if (! /^\d+$/.test(aadhaar)) return false;
			if (! /^[2-9]{1}[0-9]{3}[0-9]{4}[0-9]{4}$/.test(aadhaar)) return false;
			if (! validate_aadhaar(aadhaar)) return false;
			return true;
		}
		function donor_check_id() {
			if ($("#edit-d-id-type").val() == "99") return true;
			if ($.trim($("#edit-d-id-no").val()) == "") {
				alert("ID No is mandatory"); $("#edit-d-id-no").focus(); return false;
			}
			if ($("#edit-d-id-type").val() == "1") {
				var regpan = /^([a-zA-Z]){5}([0-9]){4}([a-zA-Z]){1}?$/;
				if (regpan.test($("#edit-d-id-no").val()) == false) {
					alert("Invalid PAN card no!"); $("#edit-d-id-no").focus(); return false;
				}
			}
			else if ($("#edit-d-id-type").val() == "2") {
				if (! aadhaar_is_valid($("#edit-d-id-no").val())) {
					alert("Invalid Aadhaar no!"); $("#edit-d-id-no").focus(); return false;
				}
			}
			return true;
		}
		$(document).ready(function () {
			if (donor_don_count > 1) {
				alert("Note: this donor is linked to " + donor_don_count + " donations. Your changes here update the donor details shared by all of them. Existing receipts will NOT be changed.");
			}
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
				if (donor_don_count > 1) {
					if (! confirm("This donor is linked to " + donor_don_count + " donations. Your changes update the donor details shared by all of them. Existing receipts will NOT be changed.\n\nDo you want to continue?")) {
						e.preventDefault();
						return false;
					}
				}
			});
		});
	})(jQuery);';
	drupal_add_js($js, 'inline');

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
}

function donor_form_submit($form, &$form_state)
{
	global $user;
	$v = $form_state['values'];

	$country_code = get_country($v['d_country']);
	$state_code = get_state($v['d_state'], $country_code);
	$city_id = ($v['d_city'] != '' && $country_code != '' && $state_code != '')
		? get_city($v['d_city'], $state_code, $country_code) : 0;

	$error = '';
	if ($country_code == '')
		$error .= 'No Country ' . $v['d_country'] . "\n";
	if ($state_code == '')
		$error .= 'No State ' . $v['d_state'] . "\n";
	if ($city_id == 0 && $v['d_city'] != '')
		$error .= 'No City ' . $v['d_city'] . "\n";

	$donor = array(
		'd_name' => $v['d_name'],
		'd_gender' => $v['d_gender'],
		'd_id_type' => $v['d_id_type'],
		'd_id_no' => strtoupper($v['d_id_no']),
		'd_contact' => $v['d_contact'],
		'd_email' => $v['d_email'],
		'd_pincode' => $v['d_pincode'],
		'd_country' => $country_code,
		'd_state' => $state_code,
		'd_city' => $city_id,
		'd_address' => $v['d_address'],
		'd_occupation' => $v['d_occupation'],
		'd_error' => $error,
		'd_updated' => date('Y-m-d H:i:s'),
		'd_updatedby' => $user->uid,
	);

	db_update('dh_donor')->fields($donor)->condition('d_id', $v['d_id'])->execute();
	drupal_set_message(t('Donor %name updated.', array('%name' => $v['d_name'])));
	$form_state['redirect'] = 'donors';
}

/**
 * Read-only, paginated table of donations mapped to a donor (all statuses).
 */
function donor_mapped_donations_table($donor_id)
{
	$header = array(t('Receipt No'), t('Date'), t('Amount'), t('Currency'),
		t('Type'), t('Category'), t('Status'));

	$query = db_select('dh_donation', 'd')->extend('PagerDefault');
	$query->fields('d', array('d_id', 'd_receipt_f', 'd_date', 'd_amount', 'd_currency', 'd_type', 'd_category', 'd_deleted'));
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

	drupal_set_title(t('Donations for @name', array('@name' => $donor['d_name'])));
	$out = donor_mapped_donations_table($donor['d_id']);
	$out .= '<p>' . l(t('&laquo; Back to donors'), 'donors', array('html' => TRUE)) . '</p>';
	return $out;
}

/**
 * Soft-delete a donor. Blocked when ANY donation (including cancelled) is
 * mapped. Sets d_deleted = 1, d_deleted_by = uid.
 */
function donor_delete($donor)
{
	global $user;

	if (!$donor) {
		drupal_set_message(t('Donor not found.'), 'error');
		drupal_goto('donors');
	}

	$count = (int) db_query("SELECT COUNT(*) FROM dh_donation WHERE d_donor = :id",
		array(':id' => $donor['d_id']))->fetchField();
	if ($count > 0) {
		drupal_set_message(t('Cannot delete donor %name: @n donation(s) are mapped to this donor.',
			array('%name' => $donor['d_name'], '@n' => $count)), 'error');
		drupal_goto('donors');
	}

	db_update('dh_donor')
		->fields(array('d_deleted' => 1, 'd_deleted_by' => $user->uid))
		->condition('d_id', $donor['d_id'])
		->execute();

	drupal_set_message(t('Donor %name deleted.', array('%name' => $donor['d_name'])));
	drupal_goto('donors');
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
	$query->fields('l', array('l_msg', 'l_tstamp', 'l_user'));
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

	$rows = array();
	foreach ($result as $r) {
		$data = json_decode($r->l_msg, TRUE);
		if ($data && isset($data['new']) && is_array($data['new'])) {
			$parts = array();
			foreach ($data['new'] as $col => $newval) {
				if ($col == 'd_id')
					continue;
				$oldval = isset($data['old'][$col]) ? $data['old'][$col] : '';
				if ($col == 'd_id_type') {
					$oldval = isset($id_types[$oldval]) ? $id_types[$oldval] : $oldval;
					$newval = isset($id_types[$newval]) ? $id_types[$newval] : $newval;
				}
				$parts[] = '<strong>' . check_plain(donor_changelog_field_label($col)) . '</strong>: "'
					. check_plain($oldval) . '" &rarr; "' . check_plain($newval) . '"';
			}
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

	drupal_set_title(t('Change history: @name', array('@name' => $donor['d_name'])));
	$out = donor_changelog_table($donor['d_id']);
	$out .= '<p>' . l(t('&laquo; Back to donors'), 'donors', array('html' => TRUE)) . '</p>';
	return $out;
}
