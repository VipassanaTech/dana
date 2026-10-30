<?php
// Help page: Donation report. Simple English, no technical details. Checked against the live code on 2026-10-30.
return array(
  'title' => 'Donation report',
  'summary' => 'Find donations, see the totals, open receipts and send them again.',
  'rights' => array('access donation report'),
  'sections' => array(
    array(
      'id' => 'filters',
      'title' => 'Choose which donations to see',
      'rights' => array(),
      'body' => '<p>{img:report}</p> '
        . '<p><i>The picture shows the screen of a Donation Admin. Your screen may have fewer buttons and links.</i></p> '
        . '<p>Click {tab:report} in the top menu. At first it shows the donations of today.</p> '
        . '<p>Use these boxes to choose the donations you want. Leave a box empty, or on <b class="ui">All</b>, <b class="ui">Both</b> or <b class="ui">Choose</b>, to see all.</p> '
        . '<ul>'
        . '<li><b class="ui">From Date</b> and <b class="ui">To Date</b>: the first and the last date of the donations.</li>'
        . '<li><b class="ui">Type</b>: <b class="ui">Both</b>, <b class="ui">Local</b> or <b class="ui">Foreign</b> donations.</li>'
        . '<li><b class="ui">Category</b>: the category of the donation.</li>'
        . '<li><b class="ui">ID Type</b>: the ID type of the donor, for example PAN or Aadhaar.</li>'
        . '<li><b class="ui">Type of Transaction</b>: how the money was given, for example cash or cheque. You can choose more than one.</li>'
        . '<li><b class="ui">Course</b>: donations of one course. The list has the courses of the last 12 months. <b class="ui">Non Course</b> means donations not linked to a course.</li>'
        . '<li><b class="ui">Receipt No</b>: type only the number, without the letters in front.</li>'
        . '<li><b class="ui">Donor Name</b>: type the full name, exactly as it is saved. If a list of names appears, choose the name from it.</li>'
        . '<li><b class="ui">Created By</b>: donations entered by one user.</li>'
        . '<li><b class="ui">Min Amount</b> and <b class="ui">Max Amount</b>: the smallest and the largest amount.</li>'
        . '<li><b class="ui">Photo ID</b>: the ID number of the donor, for example a PAN or Aadhaar number. Type the full number without spaces.</li>'
        . '<li><b class="ui">Deleted</b>: tick it to see only cancelled donations.</li>'
        . '<li><b class="ui">Having Edit Logs</b>: tick it to see only donations that were changed after they were saved.</li>'
        . '</ul>',
    ),
    array(
      'id' => 'filter-tags',
      'title' => 'Find donations by tag',
      'rights' => array('view donation tags'),
      'body' => '<p><b class="ui">Tags</b>: choose one or more tags to see the donations that have them. See {help:tags#view}.</p> ',
    ),
    array(
      'id' => 'filters-tally',
      'title' => 'Synced and Not Synced',
      'rights' => array(),
      'condition' => 'tally',
      'body' => '<ul>'
        . '<li><b class="ui">Synced</b>: tick it to see only donations already sent to Tally.</li>'
        . '<li><b class="ui">Not Synced</b>: tick it to see only donations not yet sent to Tally.</li>'
        . '</ul>'
        . '<p>Do not tick both. Then no donation is shown.</p> ',
    ),
    array(
      'id' => 'get-report',
      'title' => 'Get the report',
      'rights' => array(),
      'body' => '<ol>'
        . '<li>Choose the dates and the other boxes.</li>'
        . '<li>Click <b class="ui">Get Report</b>.</li>'
        . '</ol>'
        . '<p>Dana shows how many donations it found, and then the list. If nothing matches, you see <b class="ui">No transactions found</b>.</p> '
        . '<p>Each row is one donation. The columns show the date (<b class="ui">Txn Date</b>), course, donor details, <b class="ui">Receipt No</b> and more.</p> '
        . '<ul>'
        . '<li>The amount is in the column of its type of transaction, for example cash.</li>'
        . '<li>The last row of the list has the total of each amount column.</li>'
        . '<li><b class="ui">Grand Total</b> below the list is the total of all the donations in the list. Each currency has its own total.</li>'
        . '<li>Cancelled donations are not added to the totals.</li>'
        . '<li>At the bottom, a second table shows the total amount for each ID type. It uses only the two dates. The other boxes do not change it.</li>'
        . '<li>A donor name with a dotted line uses the donor details saved for that financial year. Point at the name to see the year.</li>'
        . '</ul>',
    ),
    array(
      'id' => 'own-note',
      'title' => 'You see only your own donations',
      'rights' => array('access own donation'),
      'unless' => array('administer donation'),
      'body' => '<p>The list, the totals and the Excel file have only the donations that you entered yourself.</p> ',
    ),
    array(
      'id' => 'excel',
      'title' => 'Download the report as an Excel file',
      'rights' => array(),
      'body' => '<ol>'
        . '<li>Get the report.</li>'
        . '<li>Click <b class="ui">Download as Excel</b> just above the list.</li>'
        . '<li>Open or save the file.</li>'
        . '</ol>'
        . '<p>The link shows only when the report found donations.</p> '
        . '<p>The file is made from the dates and the other boxes of your report. It also has the city, state and PIN code of each donor. The totals are at the end.</p> '
        . '<p class="note">Cancelled donations are never in the Excel file.</p> ',
    ),
    array(
      'id' => 'receipt',
      'title' => 'Open a receipt',
      'rights' => array('download receipt'),
      'body' => '<ol>'
        . '<li>Find the donation in the report.</li>'
        . '<li>Click its number in the <b class="ui">Receipt No</b> column.</li>'
        . '<li>The receipt opens in a small new window. You can print or save it from there.</li>'
        . '</ol>'
        . '<p>If the number is not a link, this donation has no receipt.</p> '
        . '<p>If nothing opens, your browser may have blocked the new window. Allow pop-up windows for Dana.</p> '
        . '<p>The receipt of a cancelled donation opens with the word CANCELLED across it. If a reason was given, it is printed at the top.</p> '
        . '<p>If you see <b class="ui">The cancelled receipt could not be prepared. Please try again later.</b>, wait a little and click again.</p> ',
    ),
    array(
      'id' => 'send-again',
      'title' => 'Send a receipt again',
      'rights' => array(),
      'body' => '<p>Use this when the donor did not get the receipt.</p> '
        . '<ol>'
        . '<li>Find the donation in the report.</li>'
        . '<li>In the <b class="ui">Action 3</b> column, click <b class="ui">Email</b> or <b class="ui">Whatsapp</b>.</li>'
        . '<li>The link shows <b class="ui">Loading...</b> for a moment.</li>'
        . '<li>A message box opens. Read it, then click OK.</li>'
        . '</ol>'
        . '<ul>'
        . '<li><b class="ui">Message successfully sent</b>: the receipt was sent. The box shows the email address or the mobile number.</li>'
        . '<li><b class="ui">Send Failed</b>: the receipt was not sent. The box shows the reason.</li>'
        . '</ul>'
        . '<p>The email goes to the email address of the donor. WhatsApp goes to the mobile number of the donor.</p> '
        . '<p>A common reason for a failure: the donor has no email address or no mobile number. See {help:problems#not-sent}.</p> '
        . '<p class="note">Each click sends the receipt again. Click only once.</p> '
        . '<p>These links are not shown for cancelled donations or for donations without a receipt.</p> ',
    ),
    array(
      'id' => 'view-log',
      'title' => 'See the changes of a donation',
      'rights' => array('view log'),
      'body' => '<p>The <b class="ui">View Log</b> link shows in the <b class="ui">Action 3</b> column when a donation was changed after it was saved.</p> '
        . '<ol>'
        . '<li>Click <b class="ui">View Log</b>.</li>'
        . '<li>A new tab opens with the changes of this receipt.</li>'
        . '</ol>'
        . '<p>Each line shows what was changed (<b class="ui">Log</b>), the date and time (<b class="ui">DateTime</b>) and the user (<b class="ui">User</b>).</p> '
        . '<p>To list only the changed donations, tick <b class="ui">Having Edit Logs</b> and click <b class="ui">Get Report</b>.</p> ',
    ),
    array(
      'id' => 'row-links',
      'title' => 'The links in each row',
      'rights' => array(),
      'body' => '<p>The first columns of each row are for working on that donation:</p> '
        . '<ul>'
        . '<li><b class="ui">Action 1</b>: for changing the donation.</li>'
        . '<li><b class="ui">Action 2</b>: for cancelling the donation.</li>'
        . '<li><b class="ui">Action 3</b>: for sending the receipt again, and more.</li>'
        . '</ul>'
        . '<p>What you see there depends on your rights. The parts below explain what you can use.</p> ',
    ),
    array(
      'id' => 'row-edit',
      'title' => 'Edit',
      'rights' => array('edit own donation', 'administer donation'),
      'body' => '<ul>'
        . '<li><b class="ui">Edit</b> in <b class="ui">Action 1</b> opens the donation, so you can change it. See {help:change-donation#edit}.</li>'
        . '<li><b class="ui">Synced</b> in <b class="ui">Action 1</b>: you cannot change this donation. It shows for cancelled donations. It can also show for donations already sent to Tally.</li>'
        . '</ul>',
    ),
    array(
      'id' => 'row-cancel',
      'title' => 'Delete (cancel a donation)',
      'rights' => array('delete transaction', 'administer donation'),
      'body' => '<ul>'
        . '<li><b class="ui">Delete</b> in <b class="ui">Action 2</b> cancels the donation. Dana first asks for the reason. See {help:change-donation#cancel}.</li>'
        . '<li><b class="ui">Synced</b> in <b class="ui">Action 2</b>: you cannot cancel this donation. It shows for donations already cancelled. It can also show for donations already sent to Tally.</li>'
        . '</ul>',
    ),
    array(
      'id' => 'row-reprint',
      'title' => 'Reprint a receipt',
      'rights' => array('edit own donation', 'administer donation'),
      'body' => '<p><b class="ui">Reprint</b> in <b class="ui">Action 3</b> makes the receipt again, with the same number. See {help:change-donation#reprint}.</p> '
        . '<p>Reprint does not send the receipt. Use <b class="ui">Email</b> or <b class="ui">Whatsapp</b> to send it.</p> ',
    ),
    array(
      'id' => 'row-no-access',
      'title' => 'No Access',
      'rights' => array(),
      'unless' => array('administer donation'),
      'body' => '<ul>'
        . '<li><b class="ui">No Access</b> in <b class="ui">Action 1</b>: you do not have the right to change donations.</li>'
        . '<li><b class="ui">No Access</b> in <b class="ui">Action 2</b>: you do not have the right to cancel donations.</li>'
        . '</ul>',
    ),
    array(
      'id' => 'row-tags',
      'title' => 'The Tags column',
      'rights' => array('view donation tags', 'assign donation tags', 'unassign donation tags'),
      'body' => '<p>The <b class="ui">Tags</b> column is for donation tags. See {help:tags}.</p> ',
    ),
    array(
      'id' => 'cancelled',
      'title' => 'Cancelled donations',
      'rights' => array(),
      'body' => '<ul>'
        . '<li>Cancelled donations stay in the list. They are in bold, with a line through them.</li>'
        . '<li>The <b class="ui">Extra</b> column shows who cancelled the donation and when.</li>'
        . '<li>They are not added to the totals.</li>'
        . '<li>To see only cancelled donations, tick <b class="ui">Deleted</b> and click <b class="ui">Get Report</b>.</li>'
        . '<li>Cancelled donations are not in the Excel file.</li>'
        . '</ul>',
    ),
  ),
);
