<?php
/**
 * Help section (spec "Dana Help Section: Design", 2026-10-30): simple-English help for centre staff, at /help.
 *
 * Each page is help/<key>.php and returns array('title', 'summary', 'rights', 'sections'); each section is
 * array('id', 'title', 'body') with optional 'rights', 'unless' and 'condition'. A page or section shows only to
 * people who have ANY of its 'rights' (empty = everyone who is logged in), NONE of its 'unless' rights, and whose
 * centre meets its 'condition' ('tally' = a Tally server is set). Same pages for every centre; nothing is stored.
 *
 * When a screen, button or right changes, update its help part in the same change.
 */

/** The help pages, in menu order. A page file help/<key>.php must exist for a key to show. */
function dana_help_order() {
  return array('donation', 'change-donation', 'report', 'tags', 'donors', 'donor-master', '10bd',
    'tally', 'settings', 'users', 'problems');
}

/** All help pages (key => page array with 'key' added). */
function dana_help_pages() {
  $pages = &drupal_static(__FUNCTION__);
  if ($pages === NULL) {
    $pages = array();
    $dir = dirname(__FILE__) . '/help';
    foreach (dana_help_order() as $key) {
      if (!is_file("$dir/$key.php")) {
        continue;
      }
      $page = include "$dir/$key.php";
      $page['key'] = $key;
      $pages[$key] = $page;
    }
  }
  return $pages;
}

/** Centre conditions used by help pages and sections. An unknown condition hides the part. */
function dana_help_condition($name) {
  if ($name === 'tally') {
    return trim((string) variable_get('tally_server', '')) !== '';
  }
  return FALSE;
}

/** TRUE when $account may see this page or section (see the file comment). */
function dana_help_allowed(array $item, $account = NULL) {
  if (!empty($item['rights'])) {
    $ok = FALSE;
    foreach ($item['rights'] as $right) {
      if (user_access($right, $account)) {
        $ok = TRUE;
        break;
      }
    }
    if (!$ok) {
      return FALSE;
    }
  }
  if (!empty($item['unless'])) {
    foreach ($item['unless'] as $right) {
      if (user_access($right, $account)) {
        return FALSE;
      }
    }
  }
  if (!empty($item['condition']) && !dana_help_condition($item['condition'])) {
    return FALSE;
  }
  return TRUE;
}

/** The sections of $page that $account may see (none when the page itself is not allowed). */
function dana_help_visible_sections(array $page, $account = NULL) {
  if (!dana_help_allowed($page, $account)) {
    return array();
  }
  $out = array();
  foreach ($page['sections'] as $section) {
    if (dana_help_allowed($section, $account)) {
      $out[] = $section;
    }
  }
  return $out;
}

function dana_help_page_visible(array $page, $account = NULL) {
  return (bool) dana_help_visible_sections($page, $account);
}

/**
 * Turns the tokens in a section body into HTML:
 * {help:<page>} or {help:<page>#<section>} = link to that help page; {img:<name>} = help/images/<name>.png;
 * {tab:<name>} = the name of a top-menu tab as this centre shows it (see dana_help_tab_title()); <p class="note"> = a "Note" box.
 */
function dana_help_render_body($html) {
  $html = preg_replace_callback('/\{tab:([a-z0-9]+)\}/', function ($m) {
    return '<b class="ui">' . check_plain(dana_help_tab_title($m[1])) . '</b>';
  }, $html);
  $html = preg_replace_callback('/\{help:([a-z0-9-]+)(?:#([a-z0-9-]+))?\}/', function ($m) {
    $pages = dana_help_pages();
    $title = isset($pages[$m[1]]) ? $pages[$m[1]]['title'] : $m[1];
    return l($title, 'help/' . $m[1], isset($m[2]) ? array('fragment' => $m[2]) : array());
  }, $html);
  $html = preg_replace_callback('/\{img:([a-z0-9-]+)\}/', function ($m) {
    $src = base_path() . drupal_get_path('module', 'donation') . '/help/images/' . $m[1] . '.png';
    return '<img class="dana-help-shot" src="' . check_plain($src) . '" alt="' . t('Picture of the screen') . '">';
  }, $html);
  return str_replace('<p class="note">', '<p class="dana-help-note"><b>' . t('Note:') . '</b> ', $html);
}

/**
 * The name of a top-menu tab as this centre's menu shows it (a few centres renamed some tabs), else the usual name.
 * $name: home, report, settings, donors, 10bd, users or help.
 */
function dana_help_tab_title($name) {
  $tabs = array('home' => array('<front>', 'Home'), 'report' => array('donation-report', 'Report'),
    'settings' => array('admin/donation', 'Settings'), 'donors' => array('donors', 'Donors'),
    '10bd' => array('income-tax-report', 'Generate 10BD'), 'users' => array('centre-users', 'Users'),
    'help' => array('help', 'Help'));
  if (!isset($tabs[$name])) {
    return $name;
  }
  $titles = &drupal_static(__FUNCTION__);
  if ($titles === NULL) {
    $titles = db_query("SELECT link_path, link_title FROM {menu_links} WHERE menu_name = 'main-menu' AND plid = 0 AND hidden = 0")->fetchAllKeyed();
  }
  $path = $tabs[$name][0];
  return (isset($titles[$path]) && trim($titles[$path]) !== '') ? $titles[$path] : $tabs[$name][1];
}

/** Inline CSS (no new .css file, so no cache clear is needed on the sites). */
function dana_help_css() {
  drupal_add_css('.dana-help h2{margin-top:1.4em} .dana-help .ui{font-weight:bold}'
    . ' .dana-help-note{background:#fff8e1;border-left:4px solid #f0b400;padding:8px 12px}'
    . ' .dana-help-shot{max-width:100%;border:1px solid #ccc;margin:8px 0}'
    . ' .dana-help-list li{margin:6px 0}'
    . ' .dana-help table{width:auto} .dana-help td,.dana-help th{padding:4px 10px}'
    . ' .dana-help-link{float:right;margin-left:12px;font-size:0.95em}', 'inline');
}

/** /help: the topics this person can use. */
function dana_help_index_page() {
  dana_help_css();
  $items = array();
  foreach (dana_help_pages() as $key => $page) {
    if (dana_help_page_visible($page)) {
      $items[] = l($page['title'], 'help/' . $key) . ' &ndash; ' . check_plain($page['summary']);
    }
  }
  return '<div class="dana-help"><p>' . t('Choose a topic. You see help only for the parts of Dana you can use.') . '</p>'
    . theme('item_list', array('items' => $items, 'attributes' => array('class' => array('dana-help-list'))))
    . '</div>';
}

/** /help/<key>: one help page, with only the sections this person can use. */
function dana_help_page($key) {
  dana_help_css();
  $pages = dana_help_pages();
  $sections = isset($pages[$key]) ? dana_help_visible_sections($pages[$key]) : array();
  if (!$sections) {
    return '<div class="dana-help"><p>' . t('This help is not available for your account.') . ' '
      . l(t('All help topics'), 'help') . '</p></div>';
  }
  drupal_set_title($pages[$key]['title']);
  $out = '<div class="dana-help"><p>' . l(t('All help topics'), 'help') . '</p>';
  if (count($sections) > 2) {
    $toc = array();
    foreach ($sections as $section) {
      $toc[] = l($section['title'], 'help/' . $key, array('fragment' => $section['id']));
    }
    $out .= theme('item_list', array('items' => $toc, 'title' => t('On this page')));
  }
  foreach ($sections as $section) {
    $out .= '<h2 id="' . check_plain($section['id']) . '">' . check_plain($section['title']) . '</h2>'
      . dana_help_render_body($section['body']);
  }
  return $out . '</div>';
}

/** The "Help" link shown on a Dana screen; opens the help page in a new tab so nothing typed is lost. */
function dana_help_link($key, $section = '') {
  dana_help_css();
  $options = array('attributes' => array('target' => '_blank', 'rel' => 'noopener',
    'title' => t('Opens the help in a new tab')));
  if ($section !== '') {
    $options['fragment'] = $section;
  }
  return '<div class="dana-help-link">' . l(t('Help for this page'), 'help/' . $key, $options) . '</div>';
}
