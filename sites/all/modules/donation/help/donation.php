<?php
// Help page: Entering a donation. Simple English, no technical details. Checked against the live code on 2026-10-30.
return array(
  'title' => 'Entering a donation',
  'summary' => 'Enter a donation, check the donor and the ID, choose how the receipt is sent, and read the messages after saving.',
  'rights' => array('access donation'),
  'sections' => array(
    array(
      'id' => 'find-donor',
      'title' => 'Find the donor',
      'rights' => array(),
      'body' => '<p>Click the {tab:home} tab. The donation form opens.</p> '
        . '{img:donation-form}'
        . '<ol>'
        . '<li>Check <b class="ui">Select Course</b>. Choose <b class="ui">Non Course</b> if the donation is not for a course.</li>'
        . '<li>Type at least 2 letters of the name in <b class="ui">Donor Name</b>.</li>'
        . '<li>A list of matching donors opens. With <b class="ui">Non Course</b>, each name also shows the contact number.</li>'
        . '<li>Click the right donor. Dana fills in all the donor details.</li>'
        . '<li>Check the details with the donor. Correct them if needed.</li>'
        . '</ol>'
        . '<p class="note">When a course is chosen, the list may show only the students of that course. '
        . 'To search all donors, choose <b class="ui">Non Course</b>.</p> '
        . '<p>To start again, clear the <b class="ui">Donor Name</b> box. '
        . 'Dana then also empties the contact, email, ID No and address boxes.</p> ',
    ),
    array(
      'id' => 'new-donor',
      'title' => 'Enter a new donor',
      'rights' => array(),
      'body' => '<p>Always search for the donor first. Enter a new donor only when the name is not in the list.</p> '
        . '<ol>'
        . '<li>Type the full name in <b class="ui">Donor Name</b>. Do not pick a name from the list.</li>'
        . '<li>Choose the <b class="ui">Gender</b>: <b class="ui">Male</b>, <b class="ui">Female</b> or <b class="ui">Organization</b>.</li>'
        . '<li>Fill in <b class="ui">ID Type</b> and <b class="ui">ID No</b>. See the next part of this page.</li>'
        . '<li>Type the <b class="ui">Contact No</b>. WhatsApp receipts go to this number.</li>'
        . '<li>Type the <b class="ui">Post/Pin/Zip Code</b>. When you leave the box, Dana fills in Country, State and City if it knows the code.</li>'
        . '<li>Check <b class="ui">Country</b> and <b class="ui">State</b>. To change them, type the first letters and pick from the list.</li>'
        . '<li><b class="ui">Email</b>, <b class="ui">City</b> and <b class="ui">Address</b> may stay empty.</li>'
        . '</ol>'
        . '<p>Boxes marked with * must be filled. ID No is also needed, unless ID Type is No ID. '
        . 'If you type an email address, it must be a correct address.</p> '
        . '<p class="note">If you type a name but do not pick it from the list, Dana adds a new donor. '
        . 'A donor who is already in Dana is then added a second time.</p> '
        . '<p>Pressing Enter moves you to the next box. It does not save the form.</p> ',
    ),
    array(
      'id' => 'id',
      'title' => 'ID Type and ID No',
      'rights' => array(),
      'body' => '<ul>'
        . '<li>In <b class="ui">ID Type</b>, choose the kind of ID, for example PAN or Aadhaar.</li>'
        . '<li>In <b class="ui">ID No</b>, type the number.</li>'
        . '</ul>'
        . '<p>Letters become capital letters as you type. '
        . 'When you leave the ID No box, Dana removes spaces, dots and dashes.</p> '
        . '<p>When you click <b class="ui">Submit</b>, Dana checks the ID No:</p> '
        . '<ul>'
        . '<li>PAN: 10 characters. First 5 letters, then 4 digits, then 1 letter. For example ABCPE1234F. The 4th letter must be one of P, C, H, F, A, T, B, L, J or G.</li>'
        . '<li>Aadhaar: 12 digits, nothing else. It never starts with 0 or 1. Dana also checks that the digits fit together.</li>'
        . '<li>Other ID types: the ID No must not be empty.</li>'
        . '<li>No ID: the ID No may stay empty.</li>'
        . '</ul>'
        . '<p>If the number is wrong, a message box says what is wrong. '
        . 'For example, it may say that an Aadhaar number "must have 12 digits". Correct the number and click Submit again.</p> '
        . '<p>Sometimes a number is typed under the wrong ID Type, for example an Aadhaar number under Passport. Then the message asks you to change the ID Type.</p> '
        . '<p>More help: {help:problems#invalid-aadhaar} and {help:problems#invalid-pan}.</p> '
        . '<p class="note">If you tick <b class="ui">Foreign Student?</b>, type the passport number in ID No. '
        . 'It must not be empty. PAN and Aadhaar checks are not done then.</p> '
        . '<p class="note">Donations with No ID are not part of Form 10BD.</p> ',
    ),
    array(
      'id' => 'payment',
      'title' => 'Amount, payment and course',
      'rights' => array(),
      'body' => '<ol>'
        . '<li>Choose the <b class="ui">Type of Transaction</b>: how the donor paid.</li>'
        . '<li>For cash or kind, nothing more is needed. For other types, the <b class="ui">Cheque/Card/Bank Transfer</b> boxes appear. '
        . 'Fill in <b class="ui">Reference No</b>, <b class="ui">Bank</b>, <b class="ui">Branch</b> and <b class="ui">Chq Date</b> as needed.</li>'
        . '<li>Type the <b class="ui">Donation Amount</b>. Use digits only: no commas, dots or spaces.</li>'
        . '<li>Tick <b class="ui">Foreign Student?</b> for a foreign national. Then <b class="ui">Currency</b> appears. Choose the currency of the donation.</li>'
        . '<li><b class="ui">Transaction Date</b> shows today. If the money came on another day, pick that day in the calendar. '
        . 'The calendar does not allow future dates.</li>'
        . '<li>Check <b class="ui">Select Course</b>. Dana may choose a course that is ending now.</li>'
        . '<li>Check the <b class="ui">Category</b>.</li>'
        . '<li>Add a <b class="ui">Comment</b> if needed.</li>'
        . '</ol>',
    ),
    array(
      'id' => 'receipt-by',
      'title' => 'How the receipt is sent',
      'rights' => array(),
      'body' => '<p>Tick how the donor gets the receipt. You can tick more than one box.</p> '
        . '<ul>'
        . '<li><b class="ui">Paper</b>: for a printed receipt.</li>'
        . '<li><b class="ui">Email</b>: Dana emails the receipt to the donor\'s <b class="ui">Email</b> address.</li>'
        . '<li><b class="ui">Whatsapp</b>: Dana sends the receipt on WhatsApp to the <b class="ui">Contact No</b>.</li>'
        . '</ul>'
        . '<p>Which boxes are ticked at first is set in the settings of your centre.</p> '
        . '<p class="note">The receipt opens for printing after every save, even when <b class="ui">Paper</b> is not ticked.</p> ',
    ),
    array(
      'id' => 'confirm',
      'title' => 'Check and confirm',
      'rights' => array(),
      'body' => '<ol>'
        . '<li>Click <b class="ui">Submit</b>.</li>'
        . '<li>Dana checks the form. If something is missing or wrong, a message says what. Correct it and click Submit again.</li>'
        . '<li>The <b class="ui">Confirm Details</b> popup shows the donation. Check it with the donor.</li>'
        . '<li>Click <b class="ui">Confirm</b> to save. Or click <b class="ui">Cancel</b> to go back to the form. Then nothing is saved.</li>'
        . '</ol>'
        . '<p>Did you pick a donor from the list and then change some details? '
        . 'Then the popup also shows <b class="ui">Donor details changed</b>, with the old and the new values. '
        . 'The new details are saved for the donor too. Receipts given earlier do not change.</p> '
        . '<p>If the <b class="ui">Transaction Date</b> is in an earlier financial year (1 April to 31 March), '
        . 'the popup also asks how to apply these donor changes. There are three choices.</p> '
        . '<ul>'
        . '<li><b class="ui">From now on</b>: the current details and that year\'s details both change.</li>'
        . '<li>Only FY and the year, for example Only FY 2024-25: only that year\'s details change.</li>'
        . '<li><b class="ui">Current details only</b>: only the current details change. That year, and this receipt, keep the old details.</li>'
        . '</ul>'
        . '<p>Dana ticks one choice for you. Check it, then click <b class="ui">Confirm</b>.</p> '
        . '<p>If a message says the donor details are still loading, wait a moment and click Submit again.</p> ',
    ),
    array(
      'id' => 'after-saving',
      'title' => 'After saving',
      'rights' => array(),
      'body' => '<ul>'
        . '<li>A message says that the receipt was created, with its receipt number.</li>'
        . '<li>The receipt opens in a new small window. Print it from there if needed.</li>'
        . '<li>If you ticked Email, a message says "Receipt emailed to" and the address.</li>'
        . '<li>If you ticked Whatsapp, a message says "Receipt sent on WhatsApp to" and the number.</li>'
        . '<li>The form is empty again, ready for the next donation.</li>'
        . '</ul>'
        . '<p>Sometimes the receipt cannot be sent. Then a message starts with '
        . '"The donation is saved, but the receipt could NOT be emailed" (or "sent on WhatsApp"). It also gives the reason.</p> '
        . '<ul>'
        . '<li>The donation is saved. Do not enter it again.</li>'
        . '<li>Fix the cause if you can, for example a wrong email address or mobile number.</li>'
        . '<li>Send the receipt again from the report. See {help:report#send-again}.</li>'
        . '</ul>'
        . '<p>If Email was ticked but the donor has no email address, the message says "No email address to send e-Receipt".</p> '
        . '<p>More help: {help:problems#not-sent}.</p> '
        . '<p class="note">If no receipt window opens, your browser may be blocking pop-up windows. Allow pop-ups for the Dana site.</p> ',
    ),
    array(
      'id' => 'own-note',
      'title' => 'Only your own donations',
      'rights' => array('access own donation'),
      'unless' => array('administer donation'),
      'body' => '<p>Your account works only with the donations you entered yourself.</p> '
        . '<ul>'
        . '<li>The {tab:report} tab lists only your donations.</li>'
        . '<li>You can open and send only the receipts of your donations.</li>'
        . '<li>You can still find and pick any donor in <b class="ui">Donor Name</b>.</li>'
        . '</ul>',
    ),
  ),
);
