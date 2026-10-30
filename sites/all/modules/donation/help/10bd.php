<?php
// Help page: Form 10BD. Simple English, no technical details. Checked against the live code on 2026-10-30.
return array(
  'title' => 'Form 10BD',
  'summary' => 'Download the Form 10BD file, and check donor details of a past year before you file it.',
  'rights' => array('access income tax report'),
  'sections' => array(
    array(
      'id' => 'download',
      'title' => 'Download the 10BD file',
      'rights' => array(),
      'body' => '<ol>'
        . '<li>Click {tab:10bd}.</li>'
        . '<li>In <b class="ui">Select Financial Year</b>, choose the year.'
        . ' For example, 2025 - 2026 means 1 April 2025 to 31 March 2026.</li>'
        . '<li>For the whole year, click <b class="ui">Download Annual 10BD</b>.</li>'
        . '</ol>'
        . '<p>For a part of the year, first choose the year as above. Then:</p> '
        . '<ul>'
        . '<li>One quarter: choose it in <b class="ui">Quarter</b>, for example <b class="ui">Q1 (Apr - Jun)</b>.'
        . ' Click <b class="ui">Download Quarterly 10BD</b>.</li>'
        . '<li>One month: choose it in <b class="ui">Month</b>. Click <b class="ui">Download Monthly 10BD</b>.</li>'
        . '</ul>'
        . '<p>Dana downloads a CSV file. It opens in Excel. It has the columns of Form 10BD.'
        . ' Check it before you use it for Form 10BD on the income tax website.</p> '
        . '<p>The file for 2025 - 2026 is named ITR-10BD-2026.csv. Quarter and month files get the same name.'
        . ' So rename each file after you download it.</p> '
        . '<p>What the file shows:</p> '
        . '<ul>'
        . '<li>One line for each donor, donation type and mode of payment, with the total amount.</li>'
        . '<li>The donor name, address and ID of that financial year. See the review list below.</li>'
        . '<li>Mode of receipt: Cash, Kind, or Electronic modes for all other payments.</li>'
        . '<li>Donation Type: a category with "corpus" in its name shows as Corpus.'
        . ' A category with "general" or "others" in its name shows as Others. All other categories show as Specific grant.</li>'
        . '<li>The registration number (URN) of the centre and its date.'
        . ' They come from {tab:settings}: <b class="ui">Unique Registration Number (URN)</b> and <b class="ui">Registration Date of Issuance</b>.'
        . ' If they are empty in the file, ask the person who manages Settings.</li>'
        . '</ul>',
    ),
    array(
      'id' => 'before-download',
      'title' => 'Which donations are in the file',
      'rights' => array(),
      'body' => '<p>The file has the donations dated in the year, quarter or month you chose. Online donations are in it too.</p> '
        . '<p>These donations are left out:</p> '
        . '<ul>'
        . '<li>cancelled donations</li>'
        . '<li>donations marked <b class="ui">Foreign Student?</b> on the donation form</li>'
        . '<li>donations of donors with no ID (ID Type No ID)</li>'
        . '</ul>'
        . '<p>Before you file the 10BD:</p> '
        . '<ul>'
        . '<li>Check that each donor has the right ID Type and ID No, for example a PAN or an Aadhaar number.</li>'
        . '<li>Check the review list for that year. See the next part of this help.</li>'
        . '</ul>',
    ),
    array(
      'id' => 'review',
      'title' => 'Check donor details of a past year',
      'rights' => array(),
      'body' => '<p>Dana keeps the donor details of each financial year. The 10BD of a year uses those details.'
        . ' If a donor was changed later, the year may still have the old details.</p> '
        . '<p>The box <b class="ui">Donor details review for the 10BD</b> at the top of the page lists these donors.'
        . ' It shows donors whose ID, name, address, city, state or pin code differ between that year and today.</p> '
        . '<ol>'
        . '<li>In <b class="ui">Financial year</b>, choose a past year. At first Dana shows the year that ended last.</li>'
        . '<li>Click <b class="ui">Show review list</b>.</li>'
        . '</ol>'
        . '<p>The first line says how many donors differ. For each donor the list shows the details of that year and the details of today.'
        . ' <b class="ui">Year details since / by</b> and <b class="ui">Current record last changed</b> show when, and by whom.</p> '
        . '<ul>'
        . '<li>If a line says <i>looks the same</i>, the two values look alike. The difference is very small, for example spaces or capital letters.</li>'
        . '<li>A second list may show donors whose details of today have no ID for the 10BD.'
        . ' Using today\'s details would take them out of that year\'s 10BD.</li>'
        . '<li><b class="ui">UINs shared by more than one donor in this 10BD</b>: the same ID number is used by more than one donor.'
        . ' Check these donors. One person may be in Dana twice, or an ID number may be wrong.</li>'
        . '<li><b class="ui">Nothing to review.</b> in the list means that no donor of that year differs.</li>'
        . '</ul>'
        . '<p class="note">If 31 May after that year has passed, Dana shows a warning. If that 10BD was already filed, a change may need a revised 10BD.</p> ',
    ),
    array(
      'id' => 'review-apply',
      'title' => 'Use the details of today for a past year',
      'rights' => array('edit donors'),
      'body' => '<p>For each donor in the review list, decide which details are right for that year.</p> '
        . '<ul>'
        . '<li>The details of that year were wrong, for example a mistyped PAN? Then tick the donor.</li>'
        . '<li>The details really changed after that year, for example a new address? Then do not tick.'
        . ' The 10BD keeps the details of that year.</li>'
        . '</ul>'
        . '<ol>'
        . '<li>Tick the donors.</li>'
        . '<li>Click the button <i>Apply current details to FY</i> with the year.</li>'
        . '<li>Dana asks you to confirm. Click OK.</li>'
        . '</ol>'
        . '<p>Dana says how many donors were changed.'
        . ' A donor that someone changed after the list was shown is skipped. Show the list again and check.</p> '
        . '<p>After this, download the 10BD file again.</p> '
        . '<p>Click a donor name to open the <b class="ui">Edit Donor</b> page.</p> '
        . '<p>Receipts already issued still show the old details. If you are allowed to reprint receipts, Dana shows links to reprint them.</p> '
        . '<p class="note">Be careful with the second list. Ticking a donor there takes the donor out of that year\'s 10BD.</p> ',
    ),
    array(
      'id' => 'review-ask',
      'title' => 'If a donor needs a correction',
      'rights' => array(),
      'unless' => array('edit donors'),
      'body' => '<p>You can look at the review list, but you cannot change it.'
        . ' If the details of a donor are wrong for a year, tell a person who can change donor details.</p> ',
    ),
  ),
);
