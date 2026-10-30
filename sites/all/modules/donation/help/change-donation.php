<?php
// Help page: Changing or cancelling a donation. Simple English, no technical details. Checked against the live code on 2026-10-30.
return array(
  'title' => 'Changing or cancelling a donation',
  'summary' => 'Correct a saved donation, reprint its receipt, or cancel it with a reason.',
  'rights' => array('edit own donation', 'administer donation', 'delete transaction'),
  'sections' => array(
    array(
      'id' => 'edit',
      'title' => 'Change a donation',
      'rights' => array('edit own donation', 'administer donation'),
      'body' => '<ol>'
        . '<li>Open the {tab:report} tab and find the donation. See {help:report#filters}.</li>'
        . '<li>Click <b class="ui">Edit</b> in its row. The donation form opens with the saved details.</li>'
        . '<li>Correct what is wrong. You can change every box, also the donor.</li>'
        . '<li>Untick <b class="ui">Email</b> and <b class="ui">Whatsapp</b> if the donor should not get the receipt again.</li>'
        . '<li>Click <b class="ui">Update Receipt</b>.</li>'
        . '<li>Check the <b class="ui">Confirm Details</b> popup. Click <b class="ui">Confirm</b>.</li>'
        . '</ol>'
        . '<p>A message says that the receipt was updated. The receipt keeps its number. '
        . 'It opens in a new small window, and the empty donation form comes back.</p> '
        . '<ul>'
        . '<li>If you pick another donor, the popup says that the donation is being reassigned.</li>'
        . '<li>If you change the donor details, they change for the donor too. The popup shows these changes first.</li>'
        . '<li>If you change only things that are not printed on the receipt, such as the Comment, Dana keeps the old receipt. '
        . 'To make it again anyway, tick <b class="ui">Reprint the receipt even if nothing printed changed</b>.</li>'
        . '</ul>'
        . '<p class="note">The course of an older donation may no longer be in the <b class="ui">Select Course</b> list. '
        . 'Check that box before you click Update Receipt.</p> '
        . '<p>Cancelled donations and online donations cannot be changed.</p> ',
    ),
    array(
      'id' => 'own-only',
      'title' => 'Only donations you entered',
      'rights' => array('access own donation'),
      'unless' => array('administer donation'),
      'body' => '<p>Your account works only with the donations you entered yourself. '
        . 'The {tab:report} tab lists only those donations. Donations entered by other people do not show there.</p> ',
    ),
    array(
      'id' => 'cancel',
      'title' => 'Cancel a donation',
      'rights' => array('delete transaction', 'administer donation'),
      'body' => '<p>A donation is never removed. You cancel it, and it stays in the report as cancelled.</p> '
        . '<ol>'
        . '<li>Open the {tab:report} tab and find the donation.</li>'
        . '<li>Click <b class="ui">Delete</b> in its row.</li>'
        . '<li>A popup opens. Its title shows the receipt number. Check that it is the right receipt.</li>'
        . '<li>Type the reason for cancelling. It is required, up to 150 characters.</li>'
        . '<li>Click <b class="ui">Cancel Transaction</b>. To stop without cancelling, click <b class="ui">Close</b>.</li>'
        . '</ol>'
        . '<p>A message says "Transaction Cancelled successfully". After that:</p> '
        . '<ul>'
        . '<li>The donation stays in the report, crossed out. It shows who cancelled it and when.</li>'
        . '<li>Its amount is not counted in the report totals or in Form 10BD.</li>'
        . '<li>The reason is added to the <b class="ui">Comment</b>.</li>'
        . '<li>Its receipt shows CANCELLED in large letters, and the reason at the top.</li>'
        . '<li>The Email, Whatsapp and Reprint links of the donation are gone.</li>'
        . '</ul>'
        . '<p>To see only cancelled donations, tick <b class="ui">Deleted</b> when you search in the report.</p> '
        . '<p class="note">A cancel cannot be undone in Dana.</p> ',
    ),
    array(
      'id' => 'reprint',
      'title' => 'Reprint a receipt',
      'rights' => array('edit own donation', 'administer donation'),
      'body' => '<p>Reprint a receipt when the donor details on it are out of date, for example after a correction.</p> '
        . '<ol>'
        . '<li>Open the {tab:report} tab and find the donation.</li>'
        . '<li>Click <b class="ui">Reprint</b> in its row.</li>'
        . '<li>A page shows the donor details that will be printed. The receipt number does not change.</li>'
        . '<li>Click <b class="ui">Reprint receipt</b>. Or click <b class="ui">Cancel</b> to go back.</li>'
        . '</ol>'
        . '<p>A message says the receipt was reprinted. Open it from the report to print or send it.</p> '
        . '<p>When you change a donation, you can also tick <b class="ui">Reprint the receipt even if nothing printed changed</b>.</p> '
        . '<p>After you change the details of a donor, Dana may say that '
        . '"the donor details printed on receipts have changed". '
        . 'It then gives links such as "Reprint 2 receipts of FY 2025-26". '
        . 'Click a link. A page lists those receipts. Click <b class="ui">Reprint</b>.</p> '
        . '<p class="note">Cancelled donations and online donations cannot be reprinted.</p> ',
    ),
    array(
      'id' => 'older-donation',
      'title' => 'Donor details in an earlier year',
      'rights' => array('edit own donation', 'administer donation'),
      'body' => '<p>A donor\'s details can be different in each financial year (1 April to 31 March). '
        . 'A receipt shows the donor details of its own year.</p> '
        . '<p>When you change a donation of an earlier year, the form shows the donor details of that year. '
        . 'A line above <b class="ui">Donor Name</b> says which details that year uses. '
        . 'Boxes that differ from the current details are coloured. '
        . 'Point the mouse at such a box to see the current value.</p> '
        . '<p>If you change donor details, the <b class="ui">Confirm Details</b> popup shows the changes. '
        . 'It asks how the donor changes should be applied. There are three choices.</p> '
        . '<ul>'
        . '<li><b class="ui">From now on</b>: the current details and that year\'s details both change.</li>'
        . '<li>Only FY and the year, for example Only FY 2024-25: only that year\'s details change. '
        . 'Use this when the donor really had other details in that year. The current details stay the same.</li>'
        . '<li><b class="ui">Current details only</b>: only the current details change. '
        . 'That year keeps its old details, so this receipt also keeps them.</li>'
        . '</ul>'
        . '<p>Dana ticks one choice for you. Check it, then click <b class="ui">Confirm</b>.</p> '
        . '<p class="note">Form 10BD of a year is due on 31 May after that year ends. '
        . 'After that date, the popup warns that the 10BD has probably been filed. '
        . 'Changing that year\'s details may then need a revised 10BD.</p> '
        . '<p>After saving, Dana may offer links to reprint other receipts of this donor. See {help:change-donation#reprint}.</p> ',
    ),
    array(
      'id' => 'synced',
      'title' => 'Donations sent to Tally',
      'rights' => array('edit own donation', 'administer donation', 'delete transaction'),
      'condition' => 'tally',
      'body' => '<p>Your centre sends donations to Tally. '
        . 'After a donation is sent, the report shows <b class="ui">Synced</b> in place of <b class="ui">Edit</b> and <b class="ui">Delete</b>. '
        . 'You cannot change or cancel it then.</p> '
        . '<p>Some accounts have the right to change donations that were sent. '
        . 'They still see Edit and Delete, with the word Synced and the date it was sent.</p> ',
    ),
    array(
      'id' => 'online',
      'title' => 'Online donations',
      'rights' => array('edit own donation', 'administer donation', 'delete transaction'),
      'body' => '<p>Online donations cannot be changed, cancelled or reprinted in Dana. '
        . 'The report shows no Reprint link for them.</p> ',
    ),
  ),
);
