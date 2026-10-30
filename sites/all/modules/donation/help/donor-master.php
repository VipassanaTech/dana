<?php
// Help page: Download the donor list. Simple English, no technical details. Checked against the live code on 2026-10-30.
return array(
  'title' => 'Download the donor list',
  'summary' => 'Download a list of the donors who gave in the dates you choose.',
  'rights' => array('download donor master'),
  'sections' => array(
    array(
      'id' => 'download',
      'title' => 'Download the donor list',
      'rights' => array(),
      'body' => '<p>The <b class="ui">Download Donor Master</b> button is on {tab:report}, next to <b class="ui">Get Report</b>.</p> '
        . '<ol>'
        . '<li>Click {tab:report}.</li>'
        . '<li>Choose the <b class="ui">From Date</b> and the <b class="ui">To Date</b>. You can also use most of the other boxes.</li>'
        . '<li>Click <b class="ui">Download Donor Master</b>.</li>'
        . '<li>Your browser saves a file named Donor-Master.csv. You can open it in Excel.</li>'
        . '</ol>'
        . '<p>The file has the donors who gave a donation in the chosen dates. Each donor is listed once.</p> '
        . '<p>For each donor it has the name, mobile number, email, ID type, ID number and Donor ID.</p> '
        . '<ul>'
        . '<li>You do not need to click <b class="ui">Get Report</b> first. The button uses what is in the boxes now.</li>'
        . '<li>These boxes are not used: <b class="ui">Course</b>, <b class="ui">Receipt No</b>, <b class="ui">Donor Name</b> and <b class="ui">Created By</b>.</li>'
        . '<li>Cancelled donations are not counted.</li>'
        . '<li>The file has the current details of each donor.</li>'
        . '</ul>'
        . '<p>If no donor matches, Dana shows <b class="ui">No data found</b>.</p> ',
    ),
    array(
      'id' => 'own-note',
      'title' => 'Only your own donors',
      'rights' => array('access own donation'),
      'unless' => array('administer donation'),
      'body' => '<p>The file has only the donors of the donations that you entered yourself.</p> ',
    ),
  ),
);
