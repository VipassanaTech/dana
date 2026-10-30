<?php
// Help page: Managing users. Simple English, no technical details. Checked against the live code on 2026-10-30.
return array(
  'title' => 'Managing users',
  'summary' => 'Add, change and block the users of your centre, and choose what each one can do.',
  'rights' => array('manage centre users'),
  'sections' => array(
    array(
      'id' => 'list',
      'title' => 'The list of users',
      'rights' => array(),
      'body' => '<p>Open {tab:users}. You see the users of your centre, 50 on each page.</p> '
        . '<ul>'
        . '<li>Each row shows <b class="ui">Username</b>, <b class="ui">Email</b>, <b class="ui">Roles</b>, '
        . '<b class="ui">Status</b> and <b class="ui">Last access</b>.</li>'
        . '<li>Administrators of Dana are not in this list.</li>'
        . '</ul> '
        . '<p>To find a user:</p> '
        . '<ol>'
        . '<li>Type part of the name or email in <b class="ui">Name or email</b>.</li>'
        . '<li>If you want, choose a <b class="ui">Role</b> or a <b class="ui">Status</b>.</li>'
        . '<li>Click <b class="ui">Filter</b>.</li>'
        . '</ol> '
        . '<p>Your own row shows <b class="ui">You: use My account</b> instead of <b class="ui">Edit</b> and <b class="ui">Block</b>. '
        . 'Click it to change your own password.</p> '
        . '<p>If you can only manage users, Dana opens {tab:users} when you log in.</p> ',
    ),
    array(
      'id' => 'add',
      'title' => 'Add a user',
      'rights' => array(),
      'body' => '<ol>'
        . '<li>Open {tab:users} and click <b class="ui">Add user</b>.</li>'
        . '<li>Type the <b class="ui">Username</b>. The person logs in with it.</li>'
        . '<li>Type the <b class="ui">Email</b>. Each user needs a different email address.</li>'
        . '<li>Type a temporary <b class="ui">Password</b>. Type it again in the box below it.</li>'
        . '<li>Choose one <b class="ui">Role</b>. See {help:users#roles}.</li>'
        . '<li>Tick <b class="ui">User Admin: can add, edit and block users</b> only if this person should also manage users.</li>'
        . '<li>Click <b class="ui">Create user</b>.</li>'
        . '</ol> '
        . '<p>The new user can log in at once.</p> '
        . '<p>For a person who only manages users, choose <b class="ui">None (user management only; needs User Admin below)</b>. '
        . 'Then tick <b class="ui">User Admin: can add, edit and block users</b>.</p> '
        . '<p class="note">Dana does not send the password by email. '
        . 'Give the person their username and temporary password yourself. '
        . 'At the first login, they must choose their own password.</p> '
        . '<p>The password must follow the password rules: see {help:problems#password}. '
        . 'If it does not, Dana shows <b class="ui">The password does not meet the site rules:</b> and a list of what is wrong.</p> ',
    ),
    array(
      'id' => 'edit',
      'title' => 'Change a user',
      'rights' => array(),
      'body' => '<ol>'
        . '<li>Open {tab:users}. Click <b class="ui">Edit</b> in the row of the user.</li>'
        . '<li>Change what you need: <b class="ui">Username</b>, <b class="ui">Email</b>, <b class="ui">Role</b>, '
        . 'the User Admin box or <b class="ui">Status</b>.</li>'
        . '<li>Leave <b class="ui">Password</b> empty to keep the current password.</li>'
        . '<li>Click <b class="ui">Save</b>.</li>'
        . '</ol> '
        . '<ul>'
        . '<li>If you change the username, the person logs in with the new one.</li>'
        . '<li>If you type a new password, it is a temporary one. The person is logged out. '
        . 'They must choose their own password at the next login.</li>'
        . '<li>If a user has more than one role, Dana shows a warning. Choose the role to keep. Saving removes the others.</li>'
        . '</ul> ',
    ),
    array(
      'id' => 'block',
      'title' => 'Block or unblock a user',
      'rights' => array(),
      'body' => '<p>Users are not deleted here. Block a person who should no longer use Dana. '
        . 'Blocking does not change any donation.</p> '
        . '<ol>'
        . '<li>Open {tab:users}. Click <b class="ui">Block</b> in the row of the user.</li>'
        . '<li>Dana asks you to confirm. Click <b class="ui">Block</b>.</li>'
        . '</ol> '
        . '<p>The person is logged out at once. They cannot log in until you unblock them.</p> '
        . '<p>To let them in again, click <b class="ui">Unblock</b> in their row. Then click <b class="ui">Unblock</b> again to confirm.</p> '
        . '<p>You can also set <b class="ui">Status</b> to <b class="ui">Active</b> or <b class="ui">Blocked</b> when you change a user.</p> ',
    ),
    array(
      'id' => 'roles',
      'title' => 'Roles',
      'rights' => array(),
      'body' => '<p>Each user has exactly one role. The role decides what the person can do in Dana. '
        . 'The usual roles are shown below.</p> '
        . '<table>'
        . '<tr><th>Role</th> <th>What they can do</th> </tr> '
        . '<tr><td>Donation Admin</td> <td>Almost everything: entering, changing and cancelling donations, the report, '
        . 'donors, Form 10BD, tags and {tab:settings}.</td> </tr> '
        . '<tr><td>Donation Restricted</td> <td>Enter donations. Change the donations they entered, but not cancel them. '
        . 'In the report, see only their own donations. See donors.</td> </tr> '
        . '<tr><td>Dhamma Server</td> <td>Enter donations. In the report, see only their own donations. '
        . 'They cannot change or cancel donations. See donors.</td> </tr> '
        . '<tr><td>Auditor</td> <td>See the report of all donations, donors and Form 10BD. They cannot enter donations.</td> </tr> '
        . '</table>'
        . '<p>Your centre may also have other roles in the list.</p> '
        . '<p><b class="ui">User Admin: can add, edit and block users</b> is a box, not a role. '
        . 'Tick it to let a person manage users, as on this page.</p> ',
    ),
    array(
      'id' => 'log',
      'title' => 'User admin log',
      'rights' => array('view user admin log'),
      'body' => '<p>On {tab:users}, click the <b class="ui">User admin log</b> tab. '
        . 'It shows each change to a user: when, who did it, and what changed.</p> '
        . '<ul>'
        . '<li><b class="ui">When</b>: the date and time.</li>'
        . '<li><b class="ui">Done by</b>: the person who made the change.</li>'
        . '<li><b class="ui">User</b>: the user who was changed.</li>'
        . '<li><b class="ui">Action</b>: for example <b class="ui">Created</b>, <b class="ui">Blocked</b> or <b class="ui">Roles changed</b>.</li>'
        . '<li><b class="ui">Details</b>: what was changed.</li>'
        . '</ul> '
        . '<p><b class="ui">Refused</b> means someone tried a change that is not allowed.</p> '
        . '<p>To search, fill in <b class="ui">User</b>, <b class="ui">Done by</b>, <b class="ui">Action</b>, '
        . '<b class="ui">From</b> or <b class="ui">To</b>, and click <b class="ui">Filter</b>. '
        . 'Type dates like 2026-04-01.</p> ',
    ),
    array(
      'id' => 'limits',
      'title' => 'What you cannot do here',
      'rights' => array(),
      'body' => '<ul>'
        . '<li>You cannot see or change the administrators of Dana.</li>'
        . '<li>You cannot change your own account here. Use <b class="ui">You: use My account</b> for your own password.</li>'
        . '<li>You cannot give the administrator role. The <b class="ui">Role</b> list does not offer it.</li>'
        . '<li>You cannot give a user more than one role.</li>'
        . '<li>You cannot delete a user. Block the user instead.</li>'
        . '</ul> ',
    ),
  ),
);
