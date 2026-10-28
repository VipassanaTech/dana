<?php
/**
 * User Admin role: centre-level user management without Drupal's 'administer users'.
 * Spec: "User Admin Role: Design Spec" (2026-10-28).
 *   centre-users               list (administrators hidden, User Admins read-only)
 *   centre-users/add           add a user (temporary password, forced change at first login)
 *   centre-users/%user/edit    edit a user
 *   centre-users/%user/status  block / unblock (confirm form)
 *   centre-users/log           the user admin log (dh_user_log)
 * Every account change by anyone is logged by donation_user_insert/update/cancel/delete (section C).
 * PHP 7.3-safe.
 */

define('UA_ROLE', 'User Admin');
define('UA_PERM_MANAGE', 'manage centre users');
define('UA_PERM_LOG', 'view user admin log');

/* ======================================================================
 * A. Roles and rules
 * ====================================================================== */

/** The administrator role(s): the configured admin role plus any role named 'administrator'. rid => rid */
function ua_admin_rids()
{
	$rids = array();
	$admin = (int) variable_get('user_admin_role', 0);
	if ($admin > 0)
		$rids[$admin] = $admin;
	$r = user_role_load_by_name('administrator');
	if ($r)
		$rids[(int) $r->rid] = (int) $r->rid;
	return $rids;
}

/** rid of the User Admin role, 0 when the role does not exist on this site. */
function ua_role_rid()
{
	$r = user_role_load_by_name(UA_ROLE);
	return $r ? (int) $r->rid : 0;
}

/** Roles these pages may give or remove: rid => name; never administrator, User Admin or the built-in roles. */
function ua_allowed_roles()
{
	$skip = ua_admin_rids();
	$ua = ua_role_rid();
	$out = array();
	foreach (user_roles(TRUE) as $rid => $name)
		if ($rid != DRUPAL_AUTHENTICATED_RID && !isset($skip[$rid]) && $rid != $ua)
			$out[(int) $rid] = $name;
	return $out;
}

/** Why $account may not be managed on these pages ('' = it may). */
function ua_protected_reason($account)
{
	if (empty($account->uid))
		return 'anonymous';
	if ((int) $account->uid === 1)
		return 'user #1';
	$roles = isset($account->roles) ? $account->roles : array();
	foreach (ua_admin_rids() as $rid)
		if (isset($roles[$rid]))
			return 'administrator';
	$ua = ua_role_rid();
	if ($ua && isset($roles[$ua]))
		return UA_ROLE;
	return '';
}

/** Loads user $uid for a change on these pages; a protected user or your own account logs a refusal and returns NULL. */
function ua_target($uid, $what)
{
	global $user;
	$account = $uid ? user_load((int) $uid, TRUE) : FALSE;
	if (!$account)
		return NULL;
	$why = ua_protected_reason($account);
	if ($why === '' && (int) $account->uid === (int) $user->uid)
		$why = 'own account';
	if ($why !== '')
	{
		ua_log('refused', $account->uid, $account->name, array('tried to ' . $what . ' (' . $why . ')'), 'users-pages');
		return NULL;
	}
	return $account;
}

/* ======================================================================
 * B. The log (dh_user_log)
 * ====================================================================== */

/** Where the current request comes from: users-pages, people-pages, my-account or system. */
function ua_via($target_uid)
{
	global $user;
	if (empty($user->uid))
		return 'system';
	$path = current_path();
	if (strpos($path, 'centre-users') === 0)
		return 'users-pages';
	if (strpos($path, 'user/') === 0 || strpos($path, 'admin/people') === 0)
		return ((int) $user->uid === (int) $target_uid) ? 'my-account' : 'people-pages';
	return 'system';
}

/** Writes one log line. $details: plain strings, never a password. A missing table never breaks an account save. */
function ua_log($action, $target_uid, $target_name, array $details = array(), $via = NULL)
{
	global $user;
	$ok = &drupal_static(__FUNCTION__);
	if ($ok === NULL)
		$ok = db_table_exists('dh_user_log');
	if (!$ok)
	{
		watchdog('user-admin', 'dh_user_log is missing: %a on %u not logged', array('%a' => $action, '%u' => $target_name), WATCHDOG_ERROR);
		return;
	}
	db_insert('dh_user_log')->fields(array(
		'ul_actor_uid' => (int) $user->uid,
		'ul_actor_name' => empty($user->uid) ? 'System' : (string) $user->name,
		'ul_target_uid' => (int) $target_uid,
		'ul_target_name' => (string) $target_name,
		'ul_action' => (string) $action,
		'ul_details' => implode('; ', $details),
		'ul_via' => $via === NULL ? ua_via($target_uid) : $via,
		'ul_ip' => (string) ip_address(),
	))->execute();
}

/* ======================================================================
 * C. Account events: every route (these pages, Drupal's People pages, My account, cron, drush)
 * ====================================================================== */

/** Role names of an account (built-in roles left out), sorted. */
function ua_role_names($account)
{
	$all = user_roles();
	$names = array();
	foreach ((isset($account->roles) ? $account->roles : array()) as $rid => $v)
		if ($v && $rid != DRUPAL_AUTHENTICATED_RID && $rid != DRUPAL_ANONYMOUS_RID && isset($all[$rid]))
			$names[] = $all[$rid];
	sort($names);
	return $names;
}

/** What changed in the five tracked fields (username, email, status, roles, password), as log details. */
function ua_diff($old, $new)
{
	$d = array();
	if ((string) $old->name !== (string) $new->name)
		$d[] = 'username ' . $old->name . ' → ' . $new->name;
	if ((string) $old->mail !== (string) $new->mail)
		$d[] = 'email ' . $old->mail . ' → ' . $new->mail;
	if ((int) $old->status !== (int) $new->status)
		$d[] = (int) $new->status ? 'unblocked' : 'blocked';
	$or = ua_role_names($old);
	$nr = ua_role_names($new);
	$add = array_diff($nr, $or);
	$del = array_diff($or, $nr);
	if ($add)
		$d[] = 'roles + ' . implode(', ', $add);
	if ($del)
		$d[] = 'roles − ' . implode(', ', $del);
	if ((string) $old->pass !== (string) $new->pass)
		$d[] = 'password changed';
	return $d;
}

/** Implements hook_user_insert(). */
function donation_user_insert(&$edit, $account, $category)
{
	$r = ua_role_names($account);
	$d = array('email ' . $account->mail, $account->status ? 'active' : 'blocked', 'roles: ' . ($r ? implode(', ', $r) : '(none)'));
	if (!empty($edit['force_password_change']))
		$d[] = 'must change password at next login';
	ua_log('created', $account->uid, $account->name, $d);
}

/** Implements hook_user_update(): one line when username, email, status, roles or password changed. */
function donation_user_update(&$edit, $account, $category)
{
	global $user;
	if (empty($account->original))
		return;
	$o = $account->original;
	$d = ua_diff($o, $account);
	if (!$d)
		return;
	if ((int) $o->status !== (int) $account->status)
		$action = $account->status ? 'unblocked' : 'blocked';
	elseif (ua_role_names($o) !== ua_role_names($account))
		$action = 'roles';
	elseif (count($d) === 1 && (string) $o->pass !== (string) $account->pass)
		$action = ((int) $user->uid === (int) $account->uid) ? 'password-own' : 'password';
	else
		$action = 'edited';
	if (!empty($edit['force_password_change']) && (int) $user->uid !== (int) $account->uid)
		$d[] = 'must change password at next login';
	ua_log($action, $account->uid, $account->name, $d);
}

/** Implements hook_user_cancel(). */
function donation_user_cancel($edit, $account, $method)
{
	ua_log('cancelled', $account->uid, $account->name, array('method ' . $method));
}

/** Implements hook_user_delete(). */
function donation_user_delete($account)
{
	ua_log('cancelled', $account->uid, $account->name, array('account deleted'));
}

/* ======================================================================
 * D. Users list and block / unblock
 * ====================================================================== */

/** Filter form above the list; submits to the list URL as a query (search, role, status). */
function ua_list_filter_form($form, &$form_state)
{
	$roles = array('' => t('- Any -')) + user_roles(TRUE);
	unset($roles[DRUPAL_AUTHENTICATED_RID]);
	foreach (ua_admin_rids() as $rid)
		unset($roles[$rid]);
	$form['f'] = array('#type' => 'fieldset', '#title' => t('Filter'), '#attributes' => array('class' => array('container-inline')));
	$form['f']['search'] = array('#type' => 'textfield', '#title' => t('Name or email'), '#size' => 25,
		'#default_value' => isset($_GET['search']) ? $_GET['search'] : '');
	$form['f']['role'] = array('#type' => 'select', '#title' => t('Role'), '#options' => $roles,
		'#default_value' => isset($_GET['role']) ? $_GET['role'] : '');
	$form['f']['status'] = array('#type' => 'select', '#title' => t('Status'),
		'#options' => array('' => t('- Any -'), '1' => t('Active'), '0' => t('Blocked')),
		'#default_value' => isset($_GET['status']) ? $_GET['status'] : '');
	$form['f']['go'] = array('#type' => 'submit', '#value' => t('Filter'));
	return $form;
}

function ua_list_filter_form_submit($form, &$form_state)
{
	$v = $form_state['values'];
	$q = array_filter(array('search' => trim($v['search']), 'role' => (string) $v['role'], 'status' => (string) $v['status']), 'strlen');
	$form_state['redirect'] = array('centre-users', array('query' => $q));
}

/** The Users list: everyone but user #1 and administrators; rows of User Admins are read-only. */
function ua_list_page()
{
	$admin = ua_admin_rids();
	$ua = ua_role_rid();
	$header = array(
		array('data' => t('Username'), 'field' => 'u.name', 'sort' => 'asc'),
		array('data' => t('Email'), 'field' => 'u.mail'),
		t('Roles'),
		array('data' => t('Status'), 'field' => 'u.status'),
		array('data' => t('Last access'), 'field' => 'u.access'),
		t('Actions'));
	$q = db_select('users', 'u')->extend('PagerDefault')->extend('TableSort');
	$q->fields('u', array('uid', 'name', 'mail', 'status', 'access'));
	$q->condition('u.uid', 1, '>');
	if ($admin)
	{
		$sub = db_select('users_roles', 'ar')->fields('ar', array('uid'))->condition('ar.rid', array_values($admin), 'IN');
		$q->condition('u.uid', $sub, 'NOT IN');
	}
	if (isset($_GET['search']) && trim($_GET['search']) !== '')
	{
		$like = '%' . db_like(trim($_GET['search'])) . '%';
		$q->condition(db_or()->condition('u.name', $like, 'LIKE')->condition('u.mail', $like, 'LIKE'));
	}
	if (isset($_GET['role']) && ctype_digit((string) $_GET['role']))
	{
		$sub = db_select('users_roles', 'fr')->fields('fr', array('uid'))->condition('fr.rid', (int) $_GET['role']);
		$q->condition('u.uid', $sub, 'IN');
	}
	if (isset($_GET['status']) && ($_GET['status'] === '0' || $_GET['status'] === '1'))
		$q->condition('u.status', (int) $_GET['status']);
	$q->limit(50)->orderByHeader($header);
	$users = $q->execute()->fetchAllAssoc('uid');

	$roles_of = array();
	if ($users)
	{
		$all = user_roles();
		foreach (db_query('SELECT uid, rid FROM {users_roles} WHERE uid IN (:u)', array(':u' => array_keys($users))) as $r)
			$roles_of[$r->uid][(int) $r->rid] = isset($all[$r->rid]) ? $all[$r->rid] : (string) $r->rid;
	}
	$rows = array();
	foreach ($users as $uid => $u)
	{
		$rr = isset($roles_of[$uid]) ? $roles_of[$uid] : array();
		if ($ua && isset($rr[$ua]))
			$actions = t('Managed by administrators');
		else
			$actions = l(t('Edit'), 'centre-users/' . $uid . '/edit') . ' &middot; ' . l($u->status ? t('Block') : t('Unblock'), 'centre-users/' . $uid . '/status');
		$rows[] = array(check_plain($u->name), check_plain($u->mail), check_plain(implode(', ', $rr)),
			$u->status ? t('Active') : t('Blocked'), $u->access ? format_date($u->access, 'short') : t('Never'), $actions);
	}
	return array(
		'filter' => drupal_get_form('ua_list_filter_form'),
		'table' => array('#theme' => 'table', '#header' => $header, '#rows' => $rows, '#empty' => t('No users found.')),
		'pager' => array('#theme' => 'pager'),
	);
}

function ua_status_page($account)
{
	if (!ua_target($account->uid, 'open block/unblock'))
		return MENU_ACCESS_DENIED;
	return drupal_get_form('ua_status_form', $account);
}

function ua_status_form($form, &$form_state, $account)
{
	$form['#ua_uid'] = (int) $account->uid;
	$n = array('@n' => $account->name);
	if ($account->status)
		return confirm_form($form, t('Block @n?', $n), 'centre-users',
			t('@n will be logged out at once and cannot log in until unblocked.', $n), t('Block'), t('Cancel'));
	return confirm_form($form, t('Unblock @n?', $n), 'centre-users',
		t('@n will be able to log in again and will get the standard "account activated" email.', $n), t('Unblock'), t('Cancel'));
}

function ua_status_form_submit($form, &$form_state)
{
	$form_state['redirect'] = 'centre-users';
	$account = ua_target($form['#ua_uid'], 'block or unblock');
	if (!$account)
	{
		drupal_set_message(t('This user cannot be changed here.'), 'error');
		return;
	}
	$block = (int) $account->status === 1;
	user_save($account, array('status' => $block ? 0 : 1));
	drupal_set_message($block ? t('%n is blocked and has been logged out.', array('%n' => $account->name))
		: t('%n is unblocked.', array('%n' => $account->name)));
}
