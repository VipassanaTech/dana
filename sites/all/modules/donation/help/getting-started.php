<?php
// Help page: Getting started. Simple English, no technical details. Checked against the live code on 2026-10-30.
return array(
  'title' => 'Getting started',
  'summary' => 'Log in and out, find your way around, and change or reset your password.',
  'rights' => array(),
  'sections' => array(
    array(
      'id' => 'log-in',
      'title' => 'Log in',
      'body' => '<ol>'
        . '<li>Open the Dana address of your centre in your web browser.</li>'
        . '<li>In the <b class="ui">User login</b> box, type your <b class="ui">Username</b> and <b class="ui">Password</b>.</li>'
        . '<li>Click <b class="ui">Log in</b>.</li>'
        . '</ol>'
        . '<p>Before you log in, the page may say <i>Access denied</i>. This is normal. It goes away after you log in.</p>'
        . '<p class="note">Your username and password are only for you. Do not share them. Each person at the centre should have their own login.</p>',
    ),
    array(
      'id' => 'menu',
      'title' => 'The tabs at the top',
      'body' => '<p>The tabs at the top of the page take you to each part of Dana.'
        . ' You see only the tabs for the work you do. So your tabs may be different from a colleague\'s tabs.</p>'
        . '<p>The {tab:help} tab is for everyone. It shows only the help topics for the parts of Dana you can use.</p>',
    ),
    array(
      'id' => 'help-links',
      'title' => 'Help on each screen',
      'body' => '<p>Many screens have a <b class="ui">Help for this page</b> link at the top right.'
        . ' It opens the help in a new browser tab.'
        . ' Your screen stays open in the old tab, so you do not lose what you typed.</p>',
    ),
    array(
      'id' => 'password',
      'title' => 'Change your password',
      'body' => '<ol>'
        . '<li>Click <b class="ui">My account</b> at the top right of the page.</li>'
        . '<li>Click the <b class="ui">Edit</b> tab.</li>'
        . '<li>Type your old password in <b class="ui">Current password</b>.</li>'
        . '<li>Type the new password in <b class="ui">Password</b> and again in <b class="ui">Confirm password</b>.</li>'
        . '<li>Click <b class="ui">Save</b> at the bottom of the page.</li>'
        . '</ol>'
        . '<p>A new password must have all of these:</p>'
        . '<ul>'
        . '<li>at least 2 capital letters (A to Z)</li>'
        . '<li>at least 2 small letters (a to z)</li>'
        . '<li>at least 2 numbers (0 to 9)</li>'
        . '<li>at least 1 symbol, like @ # $ % or !</li>'
        . '<li>at least 10 letters and numbers in all (symbols are not counted in these 10)</li>'
        . '</ul>'
        . '<p>If the new password does not follow these rules, Dana shows what is missing. Change it and click <b class="ui">Save</b> again.</p>',
    ),
    array(
      'id' => 'forgot',
      'title' => 'If you forgot your password',
      'body' => '<ol>'
        . '<li>Under the <b class="ui">User login</b> box, click <b class="ui">Request new password</b>.</li>'
        . '<li>Type your username or the email address of your Dana account in <b class="ui">Username or e-mail address</b>.</li>'
        . '<li>Click <b class="ui">E-mail new password</b>.</li>'
        . '<li>Open the email from Dana and click the link in it. The link works only once, and only for 24 hours.</li>'
        . '<li>On the page that opens, click <b class="ui">Log in</b>.</li>'
        . '<li>Your account page opens. Type a new password in <b class="ui">Password</b> and <b class="ui">Confirm password</b>, then click <b class="ui">Save</b>.</li>'
        . '</ol>'
        . '<p>If the email does not come, look in the spam folder. If it is not there, ask the person who manages Dana users at your centre.</p>',
    ),
    array(
      'id' => 'auto-logout',
      'title' => 'Dana logs you out after 30 minutes',
      'body' => '<p>If you do not use Dana for 30 minutes, a box asks: <i>Your session is about to expire. Do you want to reset it?</i></p>'
        . '<ul>'
        . '<li>Click <b class="ui">Yes</b> quickly to stay logged in.</li>'
        . '<li>If you do not answer, Dana logs you out. Log in again.</li>'
        . '</ul>'
        . '<p class="note">Anything you typed but did not save is lost when Dana logs you out. Save a donation before you leave your desk.</p>',
    ),
    array(
      'id' => 'log-out',
      'title' => 'Log out',
      'body' => '<p>When you finish, click <b class="ui">Log out</b> at the top right of the page.'
        . ' Always log out on a computer that other people also use.</p>',
    ),
  ),
);
