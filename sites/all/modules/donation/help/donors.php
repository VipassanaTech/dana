<?php
// Help page: Donors. Simple English, no technical details. Checked against the live code on 2026-10-30.
return array(
  'title' => 'Donors',
  'summary' => 'Find a donor, see what changed, correct donor details, and delete a donor.',
  'rights' => array('view donors'),
  'sections' => array(
    array(
      'id' => 'search',
      'title' => 'Find a donor',
      'rights' => array(),
      'body' => '<p>Click {tab:donors}. Dana lists all donors by name, 50 on each page.</p> '
        . '<ol>'
        . '<li>Type in one or more of the boxes <b class="ui">Name</b>, <b class="ui">Email</b>, <b class="ui">Mobile</b> and <b class="ui">ID No</b>.'
        . ' A part is enough, for example only the first name.</li>'
        . '<li>To see only one kind of ID, choose it in <b class="ui">ID Type</b>.</li>'
        . '<li>If you need, tick <b class="ui">Has donations</b> or <b class="ui">No donations</b>.</li>'
        . '<li>If you need, tick <b class="ui">Has change logs</b> or <b class="ui">No change logs</b>.'
        . ' <b class="ui">Has change logs</b> shows only donors with something in their change history.</li>'
        . '<li>Click <b class="ui">Search</b>.</li>'
        . '</ol>'
        . '<p>Click <b class="ui">Clear</b> to see all donors again.</p> '
        . '<p>For each donor the list shows <b class="ui">Name</b>, <b class="ui">Gender</b>, <b class="ui">Contact</b>,'
        . ' <b class="ui">Email</b>, <b class="ui">ID Type</b>, <b class="ui">ID No</b> and <b class="ui">Location</b>.'
        . ' <b class="ui">Donations</b> is the number of donations of the donor.</p> '
        . '<p>In the <b class="ui">Actions</b> column, click <b class="ui">History</b> to see what changed.'
        . ' You may also see <b class="ui">Edit</b> and <b class="ui">Delete</b>.</p> ',
    ),
    array(
      'id' => 'history',
      'title' => 'See what changed for a donor',
      'rights' => array(),
      'body' => '<p>In the donors list, click <b class="ui">History</b> next to the donor. The page shows two things.</p> '
        . '<p>First, the table <b class="ui">Donor details by financial year</b>. It has one line for each financial year in which the donor gave.</p> '
        . '<ul>'
        . '<li><b class="ui">Donations (active / cancelled)</b>: how many donations of that year are active, and how many are cancelled.</li>'
        . '<li><b class="ui">Details used</b>: <b class="ui">Current details</b> means that year uses the donor details of today.'
        . ' <b class="ui">Own details</b> means that year keeps its own details. They are shown in the same line.</li>'
        . '<li><b class="ui">Since</b>: from when, and by whom, that year has its own details.</li>'
        . '</ul>'
        . '<p>Second, the list of changes, newest first, 20 on each page.'
        . ' <b class="ui">What changed</b> shows the old and the new value.'
        . ' <b class="ui">When</b> is the date and time. <b class="ui">Who</b> is the user who made the change.</p> '
        . '<p>If <b class="ui">Who</b> says <b class="ui">System / Sync</b>, the change came in automatically, not from a person.</p> ',
    ),
    array(
      'id' => 'edit',
      'title' => 'Correct donor details',
      'rights' => array('edit donors'),
      'body' => '<ol>'
        . '<li>Find the donor in the donors list.</li>'
        . '<li>Click <b class="ui">Edit</b> in the <b class="ui">Actions</b> column.</li>'
        . '<li>Correct the details, for example <b class="ui">Donor Name</b>, <b class="ui">ID No</b>, <b class="ui">Contact No</b>,'
        . ' <b class="ui">Email</b> or <b class="ui">Address</b>.</li>'
        . '<li>If the donor gave in past financial years, decide about those years. See the next part of this help.</li>'
        . '<li>Click <b class="ui">Update Donor</b>.</li>'
        . '</ol>'
        . '<p>Dana goes back to the donors list and says the donor was updated.'
        . ' If you changed nothing, it says <i>No donor details were changed.</i>'
        . ' To leave without saving, click <b class="ui">Cancel</b>.</p> '
        . '<ul>'
        . '<li>These must be filled: <b class="ui">Donor Name</b>, <b class="ui">Contact No</b>, <b class="ui">Post/Pin/Zip Code</b>,'
        . ' <b class="ui">Country</b> and <b class="ui">State</b>.</li>'
        . '<li>Type the full name of the country and the state, for example India. Dana does not accept a name it does not know.</li>'
        . '<li>When you leave the <b class="ui">ID No</b> box, Dana removes spaces, dots and dashes. It also makes letters capital.</li>'
        . '<li>When you click <b class="ui">Update Donor</b>, Dana checks the <b class="ui">ID No</b>.'
        . ' It must be filled, unless the <b class="ui">ID Type</b> is No ID.</li>'
        . '<li>If Dana says the number is wrong, see {help:problems#invalid-pan} or {help:problems#invalid-aadhaar}.</li>'
        . '</ul>'
        . '<p>The <b class="ui">View change history</b> link on this page shows what changed for the donor.</p> '
        . '<p class="note">Receipts already issued are not changed when you change donor details.</p> ',
    ),
    array(
      'id' => 'by-year',
      'title' => 'Donor details of past financial years',
      'rights' => array('edit donors'),
      'body' => '<p>Dana keeps the donor details of each financial year (1 April to 31 March).'
        . ' The receipts and the 10BD of a year use the details of that year.</p> '
        . '<p>When you change a donor, past years keep their old details. Only this year gets the new details.</p> '
        . '<p>If the donor gave in past years, the form shows <b class="ui">Also use these details for past financial years</b>.'
        . ' It has a box for each of those years. For each year, decide:</p> '
        . '<ul>'
        . '<li>Were the old details wrong, for example a typing mistake in the PAN or the name? Then tick the year.'
        . ' That year will use the new details too.</li>'
        . '<li>Did the details really change later, for example the donor moved house this year? Then do not tick the year.'
        . ' That year keeps its old details.</li>'
        . '</ul>'
        . '<p>A year may say <i>(10BD due date passed)</i>. The 10BD of that year was due on 31 May.'
        . ' If it was already filed, a change to that year may need a revised 10BD.</p> '
        . '<p>If the donor gave in past years, Dana asks you to confirm when you click <b class="ui">Update Donor</b>.'
        . ' It lists the years that keep their details and the years that will use the new details.'
        . ' Click OK to save, or Cancel to go back.</p> '
        . '<p>The table <b class="ui">Donor details by financial year</b> below the form shows which details each year uses now.'
        . ' If a value differs from the current details, point the mouse at it to see the current value.</p> '
        . '<p class="note">In April and May, the form may show a warning. It says that the year that just ended keeps the details it had on 31 March.'
        . ' Its 10BD uses those details. If a change must also be in that 10BD, tick that year.</p> ',
    ),
    array(
      'id' => 'reprint-year',
      'title' => 'Reprint the receipts of one year',
      'rights' => array('administer donation', 'edit own donation'),
      'body' => '<p>Receipts already issued are not changed when donor details change. Nothing is reprinted by itself.</p> '
        . '<p>After you save new donor details, Dana may show a message: receipts already issued still show the old details.'
        . ' The message has a link for each financial year, for example <i>Reprint 2 receipts of FY 2025-26</i>.</p> '
        . '<ol>'
        . '<li>Click the link of the year.</li>'
        . '<li>Dana shows the donor details that will be printed, and the receipt numbers.</li>'
        . '<li>Click <b class="ui">Reprint</b>. To go back without reprinting, click <b class="ui">Cancel</b>.</li>'
        . '<li>Wait until Dana says how many receipts were reprinted.</li>'
        . '</ol>'
        . '<ul>'
        . '<li>The receipt number does not change.</li>'
        . '<li>Reprinting does not send the receipt to the donor again.</li>'
        . '<li>Cancelled receipts and receipts of online donations are not reprinted.</li>'
        . '<li>If you can see only your own donations, only the receipts you entered are reprinted.</li>'
        . '</ul>'
        . '<p>If Dana says some receipts were not reprinted, try again later.</p> ',
    ),
    array(
      'id' => 'donations',
      'title' => 'See the donations of a donor',
      'rights' => array('edit donors'),
      'body' => '<p>There are two ways:</p> '
        . '<ul>'
        . '<li>In the donors list, click the number in the <b class="ui">Donations</b> column.</li>'
        . '<li>On the <b class="ui">Edit Donor</b> page, look under <b class="ui">Donations mapped to this donor (read-only)</b>.</li>'
        . '</ul>'
        . '<p>Dana shows 10 donations on each page, newest first.'
        . ' You see the <b class="ui">Receipt No</b>, <b class="ui">Date</b>, <b class="ui">Amount</b> and <b class="ui">Category</b>, and more.'
        . ' <b class="ui">Status</b> is <b class="ui">Active</b> or <b class="ui">Cancelled</b>.</p> '
        . '<p><b class="ui">Donor details</b> says which donor details that receipt uses:</p> '
        . '<ul>'
        . '<li><b class="ui">Own (that year)</b>: the details kept for that financial year.</li>'
        . '<li><b class="ui">Current</b>: the donor details of today.</li>'
        . '</ul>'
        . '<p>If a <b class="ui">Receipt No</b> is a link, click it to open the receipt.'
        . ' You cannot change a donation here.</p> ',
    ),
    array(
      'id' => 'delete',
      'title' => 'Delete a donor',
      'rights' => array('delete donors'),
      'body' => '<p>You can delete a donor only if the donor has no donations at all. Cancelled donations count too.</p> '
        . '<ol>'
        . '<li>Find the donor in the donors list.</li>'
        . '<li>Click <b class="ui">Delete</b> in the <b class="ui">Actions</b> column.</li>'
        . '<li>A page asks <i>Delete donor ...?</i> with the donor\'s name. Check the name, then click <b class="ui">Delete</b>. To go back without deleting, click <b class="ui">Cancel</b>.</li>'
        . '</ol>'
        . '<p>Dana says the donor was deleted. The donor is no longer in the list.</p> '
        . '<p>If the donor has donations, <b class="ui">Delete</b> is plain text, not a link.'
        . ' Point the mouse at it to see <b class="ui">Has mapped donations</b>.</p> '
        . '<p class="note">Check the name before you click Delete. There is no undo on the screen.</p> ',
    ),
  ),
);
