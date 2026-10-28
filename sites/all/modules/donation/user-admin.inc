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
