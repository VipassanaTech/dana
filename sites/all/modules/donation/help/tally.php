<?php
// Help page: Sending donations to Tally. Simple English, no technical details. Checked against the live code on 2026-10-30.
return array(
  'title' => 'Sending donations to Tally',
  'summary' => 'Send donations to Tally and see which ones were sent.',
  'rights' => array('sync to tally'),
  'condition' => 'tally',
  'sections' => array(
    array(
      'id' => 'sync',
      'title' => 'Send donations to Tally',
      'rights' => array(),
      'body' => '<p>Your centre keeps its accounts in Tally. Dana can send the donations to Tally.</p> '
        . '<ol>'
        . '<li>Click {tab:report}.</li>'
        . '<li>Click <b class="ui">Sync to Tally</b>.</li>'
        . '<li>Dana shows a message that it will take a few minutes.</li>'
        . '</ol>'
        . '<ul>'
        . '<li>Dana sends every donation that was not sent before, of any date.</li>'
        . '<li>The dates and the other boxes on the screen do not matter.</li>'
        . '<li>Cancelled donations are not sent.</li>'
        . '<li>The message only means that the sending has started. You can go on working.</li>'
        . '</ul>'
        . '<p>To check after a few minutes:</p> '
        . '<ol>'
        . '<li>Choose a wide date range in <b class="ui">From Date</b> and <b class="ui">To Date</b>.</li>'
        . '<li>Tick <b class="ui">Not Synced</b> and click <b class="ui">Get Report</b>.</li>'
        . '<li>The list shows the donations that are not in Tally yet.</li>'
        . '</ol>'
        . '<p>If Tally could not take a donation, it stays in this list. Click <b class="ui">Sync to Tally</b> again later.</p> '
        . '<p>Cancelled donations are never sent. So they can also be in this list.</p> '
        . '<p class="note">Click <b class="ui">Sync to Tally</b> only once. Wait a few minutes before you click it again.</p> ',
    ),
    array(
      'id' => 'filters',
      'title' => 'See which donations were sent',
      'rights' => array(),
      'body' => '<p>On {tab:report}:</p> '
        . '<ul>'
        . '<li><b class="ui">Synced</b>: tick it to see only donations already sent to Tally.</li>'
        . '<li><b class="ui">Not Synced</b>: tick it to see only donations not yet sent to Tally.</li>'
        . '</ul>'
        . '<p>Tick one of them, choose the dates, and click <b class="ui">Get Report</b>. Do not tick both. Then no donation is shown.</p> ',
    ),
    array(
      'id' => 'after-sync',
      'title' => 'After a donation is sent',
      'rights' => array('edit own donation', 'delete transaction', 'administer donation'),
      'unless' => array('edit delete after tally sync'),
      'body' => '<p>After a donation is sent to Tally, you cannot change or cancel it in Dana.</p> '
        . '<p>In the report, its row shows <b class="ui">Synced</b> where <b class="ui">Edit</b> or <b class="ui">Delete</b> would be.</p> ',
    ),
    array(
      'id' => 'after-sync-change',
      'title' => 'Changing a donation after it is sent',
      'rights' => array('edit delete after tally sync'),
      'body' => '<p>You can still change or cancel a donation after it was sent to Tally.</p> '
        . '<p>In the report, its row shows <b class="ui">Edit</b> and <b class="ui">Delete</b> with the word Synced and the date it was sent.</p> '
        . '<ul>'
        . '<li>Online donations can never be changed or cancelled in Dana.</li>'
        . '<li>Dana does not send the change or the cancelling to Tally. Correct Tally yourself.</li>'
        . '</ul>',
    ),
  ),
);
