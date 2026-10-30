<?php
// Help page: Common problems. Simple English, no technical details. Checked against the live code on 2026-10-30.
return array(
  'title' => 'Common problems',
  'summary' => 'What a message means and what to do.',
  'rights' => array(),
  'sections' => array(
    array(
      'id' => 'invalid-aadhaar',
      'title' => 'A message says "Invalid Aadhaar no!"',
      'rights' => array('access donation', 'edit donors'),
      'body' => '<p>Dana checks the Aadhaar number when you save. The message tells you what is wrong:</p>'
        . '<table>'
        . '<tr><th>The message says</th><th>What to do</th></tr>'
        . '<tr><td><i>It must have 12 digits. You typed ...</i></td><td>An Aadhaar number has exactly 12 digits. Count them again. Check with the donor.</td></tr>'
        . '<tr><td><i>An Aadhaar number never starts with 0 or 1.</i></td><td>The first digit is wrong. Check with the donor.</td></tr>'
        . '<tr><td><i>An Aadhaar number has only digits.</i></td><td>Remove the letters or other signs.</td></tr>'
        . '<tr><td><i>One of the digits may be wrong.</i></td><td>12 digits were typed, but they do not make a real Aadhaar number. Often two digits are swapped or one digit is mistyped. Check the card with the donor.</td></tr>'
        . '<tr><td><i>This looks like a PAN number, not an Aadhaar.</i></td><td>The donor gave a PAN. Change <b class="ui">ID Type</b> to PAN.</td></tr>'
        . '</table>'
        . '<p>Spaces, dots and dashes are not a problem. Dana removes them by itself.</p>',
    ),
    array(
      'id' => 'invalid-pan',
      'title' => 'A message says "Invalid PAN card no!"',
      'rights' => array('access donation', 'edit donors'),
      'body' => '<p>A PAN has 10 characters in this order: 5 letters, 4 digits, then 1 letter. For example: ABCDE1234F.</p>'
        . '<ul>'
        . '<li>If the message says <i>You typed ...</i>, a character is missing or extra. Check with the donor.</li>'
        . '<li>Look for a 0 (zero) typed in place of the letter O, or a 1 (one) in place of the letter I.</li>'
        . '<li>If the message says <i>This looks like an Aadhaar number, not a PAN</i>, change <b class="ui">ID Type</b> to Aadhaar.</li>'
        . '</ul>'
        . '<p>Small letters are not a problem. Dana changes them to capital letters by itself.</p>',
    ),
    array(
      'id' => 'not-sent',
      'title' => 'The receipt could NOT be emailed or sent on WhatsApp',
      'rights' => array('access donation', 'access donation report'),
      'body' => '<p>If sending fails, Dana says so in a yellow message. The donation itself is saved.'
        . ' The end of the message gives the reason.</p>'
        . '<table>'
        . '<tr><th>The reason says</th><th>What to do</th></tr>'
        . '<tr><td><i>No email address to send e-Receipt</i></td><td>The donor has no email address in Dana. Add it to the donor details first.</td></tr>'
        . '<tr><td><i>there is no mobile number for this donor</i></td><td>The donor has no mobile number in Dana. Add it to the donor details first.</td></tr>'
        . '<tr><td>Something about the email address or the number</td><td>The email address or mobile number may be wrong. Check it with the donor and correct it.</td></tr>'
        . '<tr><td><i>could not reach the WhatsApp service</i>, or the receipt could not be prepared</td><td>A short break in the service. Try again after a few minutes.</td></tr>'
        . '</table>'
        . '<p>Then send the receipt again from the report: {help:report#send-again}.</p>'
        . '<p>If it still fails, tell the person who looks after Dana at your centre. Give them the receipt number and the full message.</p>',
    ),
    array(
      'id' => 'cannot-see',
      'title' => 'I cannot see a tab, a button or a link',
      'body' => '<p>Each person sees only the parts of Dana that their work needs. This is set for your login.'
        . ' This help also shows only those parts.</p>'
        . '<p>If you need more, ask the person who manages Dana users at your centre.</p>',
    ),
    array(
      'id' => 'password',
      'title' => 'My new password is not accepted',
      'body' => '<p>A new password must have all of these:</p>'
        . '<ul>'
        . '<li>at least 2 capital letters (A to Z)</li>'
        . '<li>at least 2 small letters (a to z)</li>'
        . '<li>at least 2 numbers (0 to 9)</li>'
        . '<li>at least 1 symbol, like @ # $ % or !</li>'
        . '<li>at least 10 letters and numbers in all (symbols are not counted in these 10)</li>'
        . '</ul>'
        . '<p>Dana shows what is missing. Change the password and click <b class="ui">Save</b> again.</p>'
        . '<p>To change your password: click <b class="ui">My account</b> at the top right, then the <b class="ui">Edit</b> tab.'
        . ' Type your old password in <b class="ui">Current password</b>.'
        . ' Type the new one in <b class="ui">Password</b> and <b class="ui">Confirm password</b>. Click <b class="ui">Save</b>.</p>',
    ),
    array(
      'id' => 'forgot',
      'title' => 'I forgot my password',
      'body' => '<ol>'
        . '<li>Ask the person who manages Dana users at your centre. They give you a new temporary password.</li>'
        . '<li>Log in with your username and the temporary password.</li>'
        . '<li>Dana asks you to choose your own new password. Follow the password rules above.</li>'
        . '</ol>',
    ),
    array(
      'id' => 'logged-out',
      'title' => 'Dana logged me out',
      'body' => '<p>If you do not use Dana for 30 minutes, a box asks: <i>Your session is about to expire. Do you want to reset it?</i>'
        . ' Click <b class="ui">Yes</b> quickly to stay logged in. If you do not answer, Dana logs you out. Log in again.</p>'
        . '<p class="note">Anything you typed but did not save is lost when Dana logs you out. Save a donation before you leave your desk.</p>',
    ),
  ),
);
