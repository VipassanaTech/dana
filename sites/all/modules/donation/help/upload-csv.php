<?php
// Help page: Importing students (Upload CSV). Simple English, no technical details. Checked against the live code on 2026-10-30.
return array(
  'title' => 'Importing students (Upload CSV)',
  'summary' => 'Add the students of a course to Dana from a file, so their names come up on the donation form.',
  'rights' => array('administer donation'),
  'sections' => array(
    array(
      'id' => 'about',
      'title' => 'What it is for',
      'body' => '<p>When you choose a course on the donation form, Dana shows the students of that course in <b class="ui">Donor Name</b>.'
        . ' This page adds the students of a course to Dana from a file.</p>'
        . '<p>Open it with the {tab:uploadcsv} tab at the top of the page.</p>'
        . '<p class="note">Dana first checks the whole file and lists every problem. Nothing is saved until the file has no problems and you click <b class="ui">Use this file</b>.</p>',
    ),
    array(
      'id' => 'template',
      'title' => 'Get the template',
      'body' => '<ol>'
        . '<li>Download the template: {link:admin/upload-csv/template|Download the template}.</li>'
        . '<li>Open it in Excel or Google Sheets.</li>'
        . '<li>Keep line 1 as it is. It has the 14 column names.</li>'
        . '<li>Replace the two sample lines with your students: one line for each student.</li>'
        . '<li>Keep the columns in the same order. Do not add or remove columns.</li>'
        . '</ol>',
    ),
    array(
      'id' => 'columns',
      'title' => 'How to fill each column',
      'body' => '<table>'
        . '<tr><th>Column</th><th>What to write</th><th>Example</th></tr>'
        . '<tr><td>Course Type</td><td>The course name, written exactly as Dana shows it. The same name for all students of the course.</td><td>10-Day Course</td></tr>'
        . '<tr><td>Start</td><td>The first day of the course: year-month-day, or day-month-year as Excel saves it.</td><td>2026-11-05 or 05-11-2026</td></tr>'
        . '<tr><td>End</td><td>The last day of the course, written the same way. Not before Start.</td><td>2026-11-16 or 16-11-2026</td></tr>'
        . '<tr><td>Name</td><td>The full name of the student. Must be filled.</td><td>Sample Student</td></tr>'
        . '<tr><td>Gender</td><td>male or female. Must be filled.</td><td>female</td></tr>'
        . '<tr><td>Address</td><td>May be empty.</td><td>12 Sample Road</td></tr>'
        . '<tr><td>Pincode</td><td>May be empty.</td><td>400001</td></tr>'
        . '<tr><td>Contact</td><td>The mobile number. May be empty.</td><td>9000000001</td></tr>'
        . '<tr><td>Email</td><td>Empty, or a correct email address.</td><td>name@example.org</td></tr>'
        . '<tr><td>Country</td><td>The full country name. Must be filled.</td><td>India</td></tr>'
        . '<tr><td>State</td><td>The full state name. Needed for India.</td><td>Maharashtra</td></tr>'
        . '<tr><td>City</td><td>May be empty. A new city name is added to Dana.</td><td>Mumbai</td></tr>'
        . '<tr><td>ID No</td><td>The number of the ID. Empty when ID Type is No ID.</td><td>ABCPE1234F</td></tr>'
        . '<tr><td>ID Type</td><td>One of: PAN, Aadhaar, Passport, Voter ID, Driving License, Ration Card, No ID.</td><td>PAN</td></tr>'
        . '</table>',
    ),
    array(
      'id' => 'ids',
      'title' => 'ID numbers',
      'body' => '<ul>'
        . '<li>Spaces, dots and dashes in the ID No are removed. Small letters become capital letters.</li>'
        . '<li>A PAN has 10 characters: 5 letters, 4 digits, then 1 letter. Its 4th letter must be one of P, C, H, F, A, T, B, L, J or G.</li>'
        . '<li>An Aadhaar number has 12 digits. Dana also checks that the digits fit together, so a mistyped digit is found.</li>'
        . '<li>For a student from India, an Aadhaar number written under Passport (or a PAN under Aadhaar) is saved with the right ID Type. Dana tells you when it does this.</li>'
        . '<li>If the student has no ID, write No ID and leave ID No empty.</li>'
        . '<li>An ID that a student already has in Dana is never removed by this page.</li>'
        . '</ul>',
    ),
    array(
      'id' => 'upload',
      'title' => 'Upload the file',
      'body' => '<ol>'
        . '<li>Save the file as CSV. In Excel: <i>File, Save As</i>, then choose <i>CSV UTF-8 (Comma delimited)</i>.</li>'
        . '<li>Click the {tab:uploadcsv} tab. Under <b class="ui">CSV File</b>, choose your file.</li>'
        . '<li>Click <b class="ui">Check file</b>. Dana checks the whole file. Nothing is saved yet.</li>'
        . '<li>If Dana lists problems: correct them in your file, click <b class="ui">Upload a corrected file</b>, and start again.</li>'
        . '<li>If there are no problems: Dana shows how many students will be added or updated, and the courses. Check it.</li>'
        . '<li>Click <b class="ui">Use this file</b>. Or click <b class="ui">Cancel</b> to stop.</li>'
        . '</ol>'
        . '<p>Uploading the same file again does no harm. Students already in Dana are updated, not added twice.</p>',
    ),
    array(
      'id' => 'problems',
      'title' => 'Common problems in the file',
      'body' => '<table>'
        . '<tr><th>Dana says</th><th>What to do</th></tr>'
        . '<tr><td><i>This is not a valid Aadhaar number; one of the digits may be wrong.</i></td><td>Check the number with the student. If you cannot, write No ID in ID Type and leave ID No empty.</td></tr>'
        . '<tr><td><i>The ID Type is No ID, but an ID number is given.</i></td><td>Choose the right ID Type, or leave ID No empty.</td></tr>'
        . '<tr><td><i>Gender ... must be male or female.</i></td><td>Write male or female.</td></tr>'
        . '<tr><td><i>... is not in Dana, but ... starting the same day is.</i></td><td>The course already exists with a slightly different name. Write the name exactly as Dana shows it.</td></tr>'
        . '<tr><td><i>Country ... is not in Dana\'s list</i> or <i>State ... is not in Dana\'s list</i></td><td>Write the full name, for example India, Maharashtra.</td></tr>'
        . '<tr><td><i>Start ... is not a date.</i></td><td>Write the date like 2026-10-17 or 17-10-2026. The year must have 4 digits.</td></tr>'
        . '<tr><td><i>An Aadhaar number has only digits.</i></td><td>Excel may have changed a long number into something like 2.34568E+11. See the note below.</td></tr>'
        . '<tr><td><i>The same student ... is also on line ...</i></td><td>Remove one of the two lines.</td></tr>'
        . '</table>'
        . '<p class="note">Excel shows long numbers like Aadhaar in a short form (2.34568E+11) and saves them that way. Before you type or paste ID numbers, select the ID No column and set its format to Text.</p>',
    ),
  ),
);
