<?php
// Help page: Correcting donor IDs in bulk (Donor Master). Simple English, no technical details. Checked against the live code on 2026-11-02.
return array(
  'title' => 'Correcting donor IDs in bulk',
  'summary' => 'Download the Donor Master, correct the ID Type and ID No of many donors year by year in a spreadsheet, and upload it again.',
  'rights' => array('download donor master', 'upload donor master'),
  'sections' => array(
    array(
      'id' => 'about',
      'title' => 'What it is for',
      'body' => '<p>Use the Donor Master to correct the ID of many donors at once, for example before you make Form 10BD.</p>'
        . '<ol>'
        . '<li>Download the Donor Master: one line for each donor and financial year, with the ID Type and ID No that year uses.</li>'
        . '<li>Correct the ID Type and ID No in a spreadsheet.</li>'
        . '<li>Upload the file again. Dana checks it, then saves the corrections.</li>'
        . '</ol>'
        . '<p>Each line changes the ID of its own financial year only. Only the ID Type and ID No are changed. Names, contact numbers and emails are not changed by the upload.</p>',
    ),
    array(
      'id' => 'download',
      'title' => 'Download the Donor Master',
      'rights' => array('download donor master'),
      'body' => '<ol>'
        . '<li>Click {tab:report}.</li>'
        . '<li>Choose the <b class="ui">From Date</b> and <b class="ui">To Date</b>. For Form 10BD, choose the whole financial year (1 April to 31 March).</li>'
        . '<li>You can also use most of the other boxes, for example <b class="ui">Category</b> or <b class="ui">ID Type</b>.</li>'
        . '<li>Click <b class="ui">Download Donor Master</b>. A file named Donor-Master.csv is saved.</li>'
        . '</ol>'
        . '<p>The file has one line for each donor and each financial year in which the donor gave in those dates.'
        . ' A donor who gave in two financial years has two lines. The <i>FY</i> column shows the year, for example FY 2025-26.'
        . ' Each line shows the name, contact, email and ID that year uses. Cancelled donations are not counted.'
        . ' If no donor matches, Dana shows <b class="ui">No data found</b>.</p>',
    ),
    array(
      'id' => 'own-note',
      'title' => 'Only your own donors',
      'rights' => array('access own donation'),
      'unless' => array('administer donation'),
      'body' => '<p>The file has only the donors of the donations that you entered yourself.</p>',
    ),
    array(
      'id' => 'edit',
      'title' => 'Correct the file',
      'body' => '<ul>'
        . '<li>Change only the <i>ID Type</i> and <i>ID No</i> columns.</li>'
        . '<li>Do not change line 1, and do not change the <i>Full Name</i>, <i>FY</i>, <i>Donor ID</i> and <i>Check</i> columns. Dana uses them to find the right donor and year.</li>'
        . '<li>ID Type must be one of: PAN, Aadhaar, Passport, Voter ID, Driving License, Ration Card, No ID.</li>'
        . '<li>A PAN has 10 characters: 5 letters, 4 digits, then 1 letter. Its 4th letter must be one of P, C, H, F, A, T, B, L, J or G.</li>'
        . '<li>An Aadhaar number has 12 digits. Dana checks that the digits fit together.</li>'
        . '<li>You may leave lines unchanged, or delete lines you do not need. Unchanged lines are left as they are.</li>'
        . '<li>Save the file as CSV. In Excel: <i>File, Save As</i>, then choose <i>CSV UTF-8 (Comma delimited)</i>.</li>'
        . '</ul>'
        . '<p class="note">Excel shows long numbers like Aadhaar in a short form (2.34568E+11) and can save them that way. Dana then refuses those lines. Before you type or paste Aadhaar numbers, select the ID No column and set its format to Text.</p>',
    ),
    array(
      'id' => 'ask',
      'title' => 'Uploading the corrections',
      'rights' => array('download donor master'),
      'unless' => array('upload donor master'),
      'body' => '<p>You can download the Donor Master, but you cannot upload it. Give the corrected file to a person who can upload it at your centre.</p>',
    ),
    array(
      'id' => 'upload',
      'title' => 'Upload the corrected file',
      'rights' => array('upload donor master'),
      'body' => '<ol>'
        . '<li>Click the {tab:donormaster} tab at the top of the page.</li>'
        . '<li>Under <b class="ui">CSV File</b>, choose the corrected file.</li>'
        . '<li>Click <b class="ui">Check file</b>. Dana checks the whole file. Nothing is changed yet.</li>'
        . '<li>If Dana lists problems: correct them in your file, click <b class="ui">Upload a corrected file</b>, and start again.</li>'
        . '<li>If there are no problems: Dana lists each line whose ID will change, with its financial year and the old and the new ID. Check the list.</li>'
        . '<li>Click <b class="ui">Use this file</b>. Or click <b class="ui">Cancel</b> to stop.</li>'
        . '</ol>'
        . '<p>Only a file downloaded from your own centre can be uploaded.</p>',
    ),
    array(
      'id' => 'problems',
      'title' => 'What the problems mean',
      'rights' => array('upload donor master'),
      'body' => '<table>'
        . '<tr><th>Dana says</th><th>What to do</th></tr>'
        . '<tr><td><i>This file was downloaded from ..., not from this centre</i></td><td>Download the Donor Master from your own centre.</td></tr>'
        . '<tr><td><i>The first line of the file was changed.</i></td><td>Download the Donor Master again. Do not change line 1.</td></tr>'
        . '<tr><td><i>This file does not start with the first line Dana writes.</i></td><td>Download the Donor Master again from your centre. Do not change line 1.</td></tr>'
        . '<tr><td><i>This file was downloaded before Dana showed the financial year (FY) of each line.</i></td><td>Download the Donor Master again and make your corrections in the new file.</td></tr>'
        . '<tr><td><i>FY ... is not a financial year of this donor\'s donations</i> or <i>this donor has no donations in FY ...</i></td><td>The FY column was changed. Put it back as it was.</td></tr>'
        . '<tr><td><i>Donor ID ... belongs to ..., not to this name.</i></td><td>The name or the Donor ID in this line was changed. Put them back as they were.</td></tr>'
        . '<tr><td><i>someone changed this donor\'s ID for FY ... in Dana after the file was downloaded</i></td><td>Download the Donor Master again and make your correction in the new file.</td></tr>'
        . '<tr><td><i>an upload cannot remove an ID</i></td><td>To remove an ID, use <b class="ui">Edit</b> on the {tab:donors} page.</td></tr>'
        . '<tr><td><i>An Aadhaar number has only digits.</i></td><td>Excel changed the number (2.34568E+11). See the note above.</td></tr>'
        . '<tr><td>Other ID problems, for example <i>one of the digits may be wrong</i></td><td>Check the number with the donor and correct it.</td></tr>'
        . '</table>',
    ),
    array(
      'id' => 'years',
      'title' => 'Earlier financial years',
      'rights' => array('upload donor master'),
      'body' => '<p>Each line changes the ID of its own financial year only:</p>'
        . '<ul>'
        . '<li>A line for this financial year changes the donor\'s ID from now on. Earlier years keep the ID they had.</li>'
        . '<li>A line for an earlier year changes only that year, for its Form 10BD. The donor\'s ID of today stays as it is.</li>'
        . '</ul>'
        . '<p>If the Form 10BD due date of that year has passed, Dana tells you. If that 10BD was already filed, a revised 10BD may be needed.</p>',
    ),
  ),
);
