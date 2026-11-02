<?php
// Help page: Students from DIPI (the nightly DIPI student sync, domain-dana students-dipi.inc + course-dipi.inc).
// Simple English, no technical details. Checked against the live sync code (8b922e0) on 2026-11-02.
return array(
  'title' => 'Students from DIPI',
  'summary' => 'How the students of your courses come into Dana from DIPI, and what to do when a student is missing.',
  'rights' => array('access donation'),
  'sections' => array(
    array(
      'id' => 'about',
      'title' => 'What it does',
      'rights' => array(),
      'body' => '<p>Dana takes your courses and students from DIPI by itself. You do not need to type them.</p>'
        . '<ul>'
        . '<li>Each night Dana adds the next 3 courses of your centre from DIPI. They come up in <b class="ui">Select Course</b> on the donation form.</li>'
        . '<li>Dana adds the students who are marked <b>attended</b> in DIPI. Students not marked attended do not come in.</li>'
        . '<li>On the donation form, choose the course in <b class="ui">Select Course</b>. Its students come up in <b class="ui">Donor Name</b>.</li>'
        . '</ul>'
        . '<p class="note">Mark attendance in DIPI as early as you can. Then the students are in Dana before the donation desk opens.</p>',
    ),
    array(
      'id' => 'when',
      'title' => 'When the students come in',
      'rights' => array(),
      'body' => '<ul>'
        . '<li>Dana checks DIPI every night at 11:30 pm.</li>'
        . '<li>Each course is checked every night, from 3 days before it ends until 4 days after it ends.</li>'
        . '<li>Each night adds the students marked attended since the night before.</li>'
        . '<li>A 1-day course is also checked on the day of the course, about twice an hour from 1 am to 5 pm.</li>'
        . '<li>After that, Dana does not check the course again.</li>'
        . '</ul>'
        . '<p><i>Example: a 10-day course ends on 12 November. Dana checks it every night from 9 November to 16 November.</i></p>',
    ),
    array(
      'id' => 'details',
      'title' => 'What Dana takes from DIPI',
      'rights' => array(),
      'body' => '<p>For each student Dana takes:</p>'
        . '<ul>'
        . '<li>name, address, pin code, mobile, email, gender and occupation</li>'
        . '<li>country, state and city</li>'
        . '<li>confirmation number</li>'
        . '<li>ID number (PAN, Aadhaar or passport)</li>'
        . '</ul>'
        . '<p>It also tidies some details:</p>'
        . '<ul>'
        . '<li>Dana removes spaces, dots and dashes from the ID number, and makes letters capital.</li>'
        . '<li>Students often type a number in the wrong box. For a student from India, Dana takes the ID type from the number itself. A PAN is saved as PAN, and an Aadhaar as Aadhaar.</li>'
        . '<li>Dana finds the city by its name, or else by the pin code. If neither works, the city stays empty. Fill it in when the student donates.</li>'
        . '</ul>'
        . '<p>In {tab:donors}, <b class="ui">History</b> shows these changes with <b class="ui">System / Sync</b> in <b class="ui">Who</b>.</p>',
    ),
    array(
      'id' => 'already-in-dana',
      'title' => 'Students already in Dana',
      'rights' => array(),
      'body' => '<ul>'
        . '<li>A student comes in only once for a course. If you correct the details later, for example the PAN, your correction stays.</li>'
        . '<li>Dana may find a donor with exactly the same name, mobile, email, pin code, country, state and city. '
        . 'If so, it uses that donor and puts the donor on the new course. Then:'
        . '<ul>'
        . '<li><b>The donor already gave this financial year (from 1 April):</b> Dana only puts the donor on the course. '
        . 'The details stay as they are, because this year\'s receipts and 10BD use them. '
        . 'Even an empty ID is not filled in from DIPI. To add it, use <b class="ui">Edit</b> in {tab:donors}.</li>'
        . '<li><b>The donor did not give this financial year:</b> Dana updates the details from DIPI. '
        . 'It keeps the ID already in Dana when DIPI has no ID. '
        . 'It also keeps a PAN already in Dana when DIPI has a different number.</li>'
        . '</ul></li>'
        . '<li>If any of the seven details is different, for example a new mobile number, Dana adds a new donor. '
        . 'The same person may then show twice in {tab:donors}. On the donation form, choose the course first, then the student.</li>'
        . '<li>If a student sits two courses close together, for example two 1-day courses a week apart, Dana keeps the student on the later course.</li>'
        . '</ul>',
    ),
    array(
      'id' => 'typed-by-hand',
      'title' => 'Students you typed by hand',
      'rights' => array(),
      'body' => '<p>Sometimes a student donates before Dana has them, so you type them in by hand.</p>'
        . '<ul>'
        . '<li><b>You added the student as a new donor after the course started:</b> Dana does not add them again if the mobile number is the same. It leaves your donor as you typed it.</li>'
        . '<li><b>You changed an existing donor after the course started:</b> Dana keeps your changes and only puts the donor on the course.</li>'
        . '<li><b>You typed a different mobile number:</b> Dana may add a second donor.</li>'
        . '</ul>',
    ),
    array(
      'id' => 'missing',
      'title' => 'A student does not come up',
      'rights' => array(),
      'body' => '<ol>'
        . '<li>Check that the student is marked attended in DIPI.</li>'
        . '<li>Check that you chose the right course in <b class="ui">Select Course</b>.</li>'
        . '<li>If attendance was marked today, wait until tomorrow morning. For a 1-day course on its day, wait half an hour.</li>'
        . '<li>Choose <b class="ui">Non Course</b> and search the name. The student may already be in Dana, for example from an older course or typed by hand.</li>'
        . '</ol>'
        . '<p>If the course ended more than 4 days ago, Dana no longer checks it. Add the student by hand as a new donor. '
        . 'If many students are missing, write to {sitemail}.</p>',
    ),
    array(
      'id' => 'many-students',
      'title' => 'Adding many students at once',
      'rights' => array('administer donation'),
      'body' => '<p>You can add the students of a course from a file with {tab:uploadcsv}. See {help:upload-csv}. '
        . 'A student who already gave this financial year is only put on the course, the same as with DIPI.</p>',
    ),
  ),
);
