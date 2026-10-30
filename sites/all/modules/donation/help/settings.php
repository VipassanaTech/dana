<?php
// Help page: Centre settings. Simple English, no technical details. Checked against the live code on 2026-10-30.
return array(
  'title' => 'Centre settings',
  'summary' => 'Set receipt defaults, categories, payment types, receipt and email templates, and trust details.',
  'rights' => array('administer donation'),
  'sections' => array(
    array(
      'id' => 'receipt-defaults',
      'title' => 'Receipt defaults',
      'rights' => array(),
      'body' => '<p>Open {tab:settings}. The first options decide how a new donation form starts.</p> '
        . '<ul>'
        . '<li><b class="ui">Set Paper as Default</b>: <b class="ui">Paper</b> is ticked at the start on the donation form.</li>'
        . '<li><b class="ui">Set Email as Default</b>: <b class="ui">Email</b> is ticked at the start.</li>'
        . '<li><b class="ui">Set Whatsapp as Default</b>: <b class="ui">Whatsapp</b> is ticked at the start.</li>'
        . '<li><b class="ui">Default Currency</b>: the currency chosen at the start in <b class="ui">Currency</b>.</li>'
        . '</ul> '
        . '<p>Staff can still change these for each donation.</p> '
        . '<p><b class="ui">Receipt Prefix</b> is put in front of every receipt number. '
        . 'For example, with ER- a receipt number looks like ER-0001234. You may leave it empty.</p> '
        . '<p class="note">If you change the prefix, new receipts get the new prefix. '
        . 'The number after it does not start again. '
        . 'A receipt made again later, for example after a donation is changed, also gets the new prefix.</p> ',
    ),
    array(
      'id' => 'lists',
      'title' => 'Categories and payment types',
      'rights' => array(),
      'body' => '<p><b class="ui">Cost Categories with Cost Centres</b> is the list for <b class="ui">Category</b> on the donation form and in the report.</p> '
        . '<ul>'
        . '<li>Write one category on each line.</li>'
        . '<li>Write the cost category, then a hyphen, then the cost centre.</li>'
        . '<li>Example: <i>Course - General</i></li>'
        . '</ul> '
        . '<p><b class="ui">Transaction Types</b> is the list for <b class="ui">Type of Transaction</b> on the donation form.</p> '
        . '<ul>'
        . '<li>Write one type on each line, as a short code, then =, then the name.</li>'
        . '<li>Example: <i>cash=Cash</i></li>'
        . '<li>Staff see the name. Dana keeps the short code with each donation.</li>'
        . '</ul> '
        . '<p class="note">Do not change or remove a line that donations already use. '
        . 'Old donations keep their old category and code. '
        . 'Keep the codes cash and kind as they are, because Form 10BD uses them.</p> ',
    ),
    array(
      'id' => 'templates',
      'title' => 'Receipt and email templates',
      'rights' => array(),
      'body' => '<ul>'
        . '<li><b class="ui">Donation Template</b>: the layout of the receipt you print or download.</li>'
        . '<li><b class="ui">Donation Email Template</b>: the layout of the receipt sent by email and WhatsApp.</li>'
        . '<li><b class="ui">Email Subject</b>: the subject of the receipt email.</li>'
        . '<li><b class="ui">Email Template</b>: the message of the receipt email. The receipt is attached to it.</li>'
        . '<li><b class="ui">Email From Name</b> and <b class="ui">From Email</b>: the sender name and email address the donor sees.</li>'
        . '<li><b class="ui">Email to send a bcc copy of the receipt</b>: Dana also sends every receipt email to this address. '
        . 'For example, use your centre email.</li>'
        . '</ul> '
        . '<p>Words in square brackets, like [donor-name] or [amount], are filled in by Dana for each donation. '
        . 'Do not change or remove them.</p> '
        . '<p class="note">Change templates with care. A mistake shows on every new receipt. '
        . 'Before a change, keep the old text in a file on your computer, so you can put it back. '
        . 'Receipts already made do not change.</p> ',
    ),
    array(
      'id' => 'trust',
      'title' => 'Trust details',
      'rights' => array(),
      'body' => '<ul>'
        . '<li><b class="ui">Center Display Name</b>: your centre name as donors should see it. '
        . 'It is used in the emails that send Form 10BE certificates to donors.</li>'
        . '<li><b class="ui">Unique Registration Number (URN)</b>: the Income Tax registration number of your trust.</li>'
        . '<li><b class="ui">Registration Date of Issuance</b>: the date this number was given.</li>'
        . '<li><b class="ui">Permanent Account Number (PAN)</b>: the PAN of your trust.</li>'
        . '</ul> '
        . '<p class="note">Dana puts the URN and its date in Form 10BD. Check them before you make Form 10BD.</p> ',
    ),
    array(
      'id' => 'tally',
      'title' => 'Tally settings',
      'rights' => array(),
      'condition' => 'tally',
      'body' => '<ul>'
        . '<li><b class="ui">Tally Server with port</b>: the address of your Tally computer, with its port number.</li>'
        . '<li><b class="ui">Tally Company</b>: the company in Tally that gets the donations.</li>'
        . '<li><b class="ui">Tally voucher type name</b>: the voucher type in Tally.</li>'
        . '</ul> '
        . '<p>Change these only with the person who set up Tally for your centre.</p> '
        . '<p class="note">If you empty <b class="ui">Tally Server with port</b>, the <b class="ui">Sync to Tally</b> button '
        . 'goes away from {tab:report}.</p> ',
    ),
    array(
      'id' => 'registration',
      'title' => 'Registration System',
      'rights' => array('access registration settings'),
      'body' => '<p>You may also see <b class="ui">Registration System</b> and <b class="ui">Registration System Settings</b>. '
        . 'Leave them as they are. They do not change how Dana works today.</p> ',
    ),
    array(
      'id' => 'save',
      'title' => 'Save your changes',
      'rights' => array(),
      'body' => '<ol>'
        . '<li>Check your changes.</li>'
        . '<li>Click <b class="ui">Save configuration</b> at the bottom of the page.</li>'
        . '<li>Dana shows a message that the options are saved.</li>'
        . '</ol> '
        . '<p>The new settings work at once for everyone at your centre.</p> '
        . '<p class="note">Some fields must not be empty. They are marked with a star. '
        . 'If one is empty, Dana shows an error and saves nothing. Fill it in and save again.</p> ',
    ),
  ),
);
