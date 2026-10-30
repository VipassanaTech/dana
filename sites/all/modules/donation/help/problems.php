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
      'body' => '<p>The password must follow some rules. See {help:getting-started#password}.</p>',
    ),
    array(
      'id' => 'forgot',
      'title' => 'I forgot my password',
      'body' => '<p>You can get a new one by email. See {help:getting-started#forgot}.</p>',
    ),
    array(
      'id' => 'logged-out',
      'title' => 'Dana logged me out',
      'body' => '<p>Dana logs you out after 30 minutes without use. Log in again. See {help:getting-started#auto-logout}.</p>',
    ),
  ),
);
