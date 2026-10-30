<?php
// Help page: Donation tags. Simple English, no technical details. Checked against the live code on 2026-10-30.
return array(
  'title' => 'Donation tags',
  'summary' => 'Make tags, put them on donations, and find donations by tag.',
  'rights' => array('view donation tags', 'create donation tags', 'delete donation tags', 'assign donation tags', 'unassign donation tags'),
  'sections' => array(
    array(
      'id' => 'about',
      'title' => 'What tags are',
      'rights' => array(),
      'body' => '<p>A tag is a short name that you put on donations. Tags help you find those donations later.</p> '
        . '<p>All the tag work is done on {tab:report}.</p> ',
    ),
    array(
      'id' => 'view',
      'title' => 'See tags and find donations by tag',
      'rights' => array('view donation tags'),
      'body' => '<p>To see the tags of one donation:</p> '
        . '<ol>'
        . '<li>Get a report on {tab:report}.</li>'
        . '<li>In the <b class="ui">Tags</b> column of the donation, click <b class="ui">ViewTags</b>.</li>'
        . '<li>A box shows the tags of that donation. If it has no tags, the box says <b class="ui">No Tag Assigned to Donation.</b></li>'
        . '</ol>'
        . '<p>To find donations by tag:</p> '
        . '<ol>'
        . '<li>In the <b class="ui">Tags</b> box, with the other report boxes, choose one or more tags.</li>'
        . '<li>Click <b class="ui">Get Report</b>.</li>'
        . '</ol>'
        . '<p>The list shows the donations that have any of the chosen tags.</p> ',
    ),
    array(
      'id' => 'create',
      'title' => 'Make new tags',
      'rights' => array('create donation tags'),
      'body' => '<ol>'
        . '<li>On {tab:report}, find the box <b class="ui">Create/Delete Tags</b>. It is just above the list.</li>'
        . '<li>Type the new tag in the box and press Enter.</li>'
        . '<li>To make more tags at once, type each one and press Enter.</li>'
        . '<li>Click <b class="ui">Create Selected Tags</b>.</li>'
        . '<li>Dana shows the tags and asks you to confirm. Click <b class="ui">Ok</b>.</li>'
        . '</ol>'
        . '<p>Dana shows a message and opens the page again. The new tags are now in the lists.</p> ',
    ),
    array(
      'id' => 'delete',
      'title' => 'Delete tags',
      'rights' => array('delete donation tags'),
      'body' => '<ol>'
        . '<li>On {tab:report}, in the box <b class="ui">Create/Delete Tags</b>, choose the tags to delete.</li>'
        . '<li>Click <b class="ui">Delete Selected Tags</b>.</li>'
        . '<li>Dana shows the tags and asks you to confirm. Click <b class="ui">Ok</b>.</li>'
        . '</ol>'
        . '<p class="note">A tag that is still on a donation cannot be deleted. First it must be removed from all its donations. If even one chosen tag is still in use, no tag is deleted.</p> ',
    ),
    array(
      'id' => 'assign',
      'title' => 'Put tags on donations',
      'rights' => array('assign donation tags'),
      'body' => '<ol>'
        . '<li>On {tab:report}, get a report with the donations you want.</li>'
        . '<li>In the <b class="ui">Tags</b> column, tick the box of each donation.</li>'
        . '<li>Below the list, in <b class="ui">Assign/Unassign Tags to Selected Donations</b>, choose one or more tags.</li>'
        . '<li>Click <b class="ui">Assign Selected</b>.</li>'
        . '<li>Dana shows the receipt numbers and the tags. Click <b class="ui">Ok</b>.</li>'
        . '</ol>'
        . '<p>A message shows which tags were put on which receipts.</p> '
        . '<p>If you did not tick a donation or choose a tag, Dana says so and nothing changes.</p> ',
    ),
    array(
      'id' => 'unassign',
      'title' => 'Remove tags from donations',
      'rights' => array('unassign donation tags'),
      'body' => '<ol>'
        . '<li>On {tab:report}, get a report with the donations you want.</li>'
        . '<li>In the <b class="ui">Tags</b> column, tick the box of each donation.</li>'
        . '<li>Below the list, in <b class="ui">Assign/Unassign Tags to Selected Donations</b>, choose the tags to remove.</li>'
        . '<li>Click <b class="ui">Unassign Selected</b>.</li>'
        . '<li>Dana shows the receipt numbers and the tags. Click <b class="ui">Ok</b>.</li>'
        . '</ol>'
        . '<p>A message shows which tags were removed from which receipts.</p> '
        . '<p>If you did not tick a donation or choose a tag, Dana says so and nothing changes.</p> ',
    ),
  ),
);
