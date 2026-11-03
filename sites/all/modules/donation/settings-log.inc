<?php
/**
 * Settings history (2026-11-03): every Save on Settings (admin/donation) records each setting that changed
 * (who, when, value before, value after) in dh_log (l_module 'settings', l_event 'Setting changed', l_identifier 0).
 * Shown on admin/donation/history. Changes made outside the page (scripts, drush) do not go through Save and are
 * not recorded.
 */

/** Settings only people with 'access registration settings' see, on the form and in the history. */
function donation_settings_log_restricted()
{
	return array('reg_app' => 1, 'reg_app_settings' => 1);
}

/** Adds the recording to the Settings form (the form system_settings_form() returned). */
function donation_settings_log_attach(array &$form)
{
	array_unshift($form['#submit'], 'donation_settings_log_before');   // old values, before the save
	$form['#submit'][] = 'donation_settings_log_after';                // compare and record, after the save
}

/** The settings on the form: key => title on the screen. */
function donation_settings_log_fields(array $form)
{
	$out = array();
	foreach (element_children($form) as $k)
		if (isset($form[$k]['#type']) && in_array($form[$k]['#type'], array('checkbox', 'select', 'textfield', 'textarea', 'text_format', 'radios'), TRUE))
			$out[$k] = isset($form[$k]['#title']) ? (string) $form[$k]['#title'] : (string) $k;
	return $out;
}

/** A setting as text: a template gives its text; Windows line ends count as plain ones (no change by themselves). */
function donation_settings_log_text($v)
{
	if (is_array($v))
		$v = array_key_exists('value', $v) ? $v['value'] : json_encode($v);
	if (is_bool($v))
		$v = $v ? '1' : '0';
	return str_replace("\r\n", "\n", (string) $v);
}

function donation_settings_log_before($form, &$form_state)
{
	$old = array();
	foreach (donation_settings_log_fields($form) as $k => $title)
	{
		$raw = variable_get($k, NULL);
		// never saved yet: the page showed (and was compared with) the form's default
		$old[$k] = donation_settings_log_text($raw === NULL ? (isset($form[$k]['#default_value']) ? $form[$k]['#default_value'] : '') : $raw);
	}
	$form_state['donation_settings_old'] = $old;
}

function donation_settings_log_after($form, &$form_state)
{
	global $user;
	if (!isset($form_state['donation_settings_old']))
		return;
	$fields = donation_settings_log_fields($form);
	foreach ($form_state['donation_settings_old'] as $k => $old)
	{
		$new = donation_settings_log_text(variable_get($k, ''));
		if ($new === $old)
			continue;
		// "key" first: the history page leaves out restricted settings by how the record starts
		$msg = array('key' => (string) $k, 'title' => isset($fields[$k]) ? $fields[$k] : (string) $k, 'old' => $old, 'new' => $new);
		$json = json_encode($msg);
		if (strlen($json) > 60000)                         // dh_log.l_msg holds 64 KB: keep the new text, note the old one's size
		{
			$msg['old'] = NULL;
			$msg['old_len'] = drupal_strlen($old);
			$json = json_encode($msg);
		}
		if (strlen($json) > 60000)
		{
			$msg['new'] = NULL;
			$msg['new_len'] = drupal_strlen($new);
			$json = json_encode($msg);
		}
		db_insert('dh_log')->fields(array('l_module' => 'settings', 'l_identifier' => 0, 'l_event' => 'Setting changed',
			'l_msg' => $json, 'l_user' => (int) $user->uid))->execute();
	}
}

/** One value cell: short values as they are, long ones (templates) behind "Show". Always escaped. */
function donation_settings_history_cell($msg, $side)
{
	if (!array_key_exists($side, $msg) || $msg[$side] === NULL)
	{
		$len = isset($msg[$side . '_len']) ? (int) $msg[$side . '_len'] : 0;
		return '<i>' . t('(too long to keep: @n characters)', array('@n' => $len)) . '</i>';
	}
	$text = (string) $msg[$side];
	if ($text === '')
		return '<i>' . t('(empty)') . '</i>';
	if (drupal_strlen($text) > 200 || substr_count($text, "\n") > 8)
		return '<details><summary>' . t('Show') . '</summary><pre style="white-space: pre-wrap; max-height: 30em; overflow: auto;">'
			. check_plain($text) . '</pre></details>';
	return nl2br(check_plain($text));
}

/** Page admin/donation/history: who changed which setting, and when (newest first, 20 per page). */
function donation_settings_history_page()
{
	$query = db_select('dh_log', 'l')->extend('PagerDefault');
	$query->leftJoin('users', 'u', 'l.l_user = u.uid');
	$query->fields('l', array('l_id', 'l_msg', 'l_tstamp', 'l_user'));
	$query->addField('u', 'name', 'uname');
	$query->condition('l.l_module', 'settings');
	if (!user_access('access registration settings'))
		foreach (array_keys(donation_settings_log_restricted()) as $k)
			$query->condition('l.l_msg', db_like('{"key":"' . $k . '"') . '%', 'NOT LIKE');
	$query->orderBy('l.l_id', 'DESC');
	$query->limit(20);
	$rows = array();
	foreach ($query->execute() as $r)
	{
		$msg = json_decode($r->l_msg, TRUE);
		if (!is_array($msg))
			continue;
		$who = ($r->l_user == 0) ? t('System / Sync') : ($r->uname !== NULL ? check_plain($r->uname) : t('uid @u', array('@u' => $r->l_user)));
		$rows[] = array(check_plain($r->l_tstamp), $who, check_plain(isset($msg['title']) ? $msg['title'] : ''),
			donation_settings_history_cell($msg, 'old'), donation_settings_history_cell($msg, 'new'));
	}
	$out = dana_help_link('settings') . '<p>' . l(t('Back to Settings'), 'admin/donation') . '</p>'
		. '<p>' . t('Each time someone saves Settings, every setting that changed is listed here, newest first.') . '</p>';
	if (!$rows)
		return $out . '<p>' . t('No changes to Settings have been recorded yet.') . '</p>';
	return $out . theme('table', array('header' => array(t('When'), t('Who'), t('Setting'), t('Before'), t('After')), 'rows' => $rows))
		. theme('pager');
}
