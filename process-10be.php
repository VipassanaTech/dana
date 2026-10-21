<?php

if (isset($_SERVER) && ( isset($_SERVER['REMOTE_ADDR']) ))
{
   echo "I wont run from the web\n";
   exit(1);
}

define('DRUPAL_ROOT', getcwd());

require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
if ( count($argv) < 3)
{
   echo "Usage: php ".$argv[0]." <subdomain> <csv-file> [<fy>] [--dry-run]\n";
   echo "  <fy>: the financial year the certificates are for: 2025, 2025-26 or 2025-2026 (default: the year that ended on the last 31 March)\n";
   echo "  --dry-run: only write lookup.csv (who each certificate would go to); nothing is sent or moved\n\n";
   exit(1);
}
// donor versioning (R12): optional FY argument and --dry-run (anywhere after the CSV path)
$dry_run = in_array('--dry-run', $argv, TRUE);
$cli = array_values(array_diff(array_slice($argv, 1), array('--dry-run')));
$fy_arg = isset($cli[2]) ? $cli[2] : NULL;

$csv_file = $argv[2];
if (! file_exists($csv_file))
{
   echo "Csv File ".$csv_file." not found\n\n";
   exit(1);
}

$variables['url'] = 'https://'.$argv[1].'.dana.vridhamma.org/';
drupal_override_server_variables($variables);

drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

if ( conf_path() == 'sites/default')
{
   echo "Invalid subdomain ".$argv[1]. "\n\n";
   exit(1);
}

// L32: the certified FY comes from the argument (default CUR-1 by the DB clock), not from date('Y')-1
$fy = tenbe_parse_fy($fy_arg, donation_cur_fy());
if ($fy <= 0)
{
   echo "Invalid financial year '".$fy_arg."' (use e.g. 2025, 2025-26 or 2025-2026; it must be a past year)\n\n";
   exit(1);
}
$fin = $fy."-".($fy + 1);

echo "financial year: $fin".($dry_run ? " (dry run: nothing is sent or moved)" : "")."\n";

$site = variable_get("site_name", "");
echo $site."\n";

$site_email = variable_get("donation_email", "");
$site_display = variable_get("center_display_name", "");

if(!$site_display)
	$site_display = $site;

$subject = "Form 10BE Certificate for Your Donation to $site_display";

// R12 (donor versioning): the certified FY's donors (that year's details) are loaded once; the current
// records are loaded only if a certificate's UIN is not in that year (fallback with a name match).
$versioned = db_table_exists('dh_donor_fy') && db_table_exists('dh_donor_version');
$fy_map = tenbe_load_fy($fy, $versioned);
$live = NULL;
$loader = function () use (&$live) { if ($live === NULL) $live = tenbe_load_live(); return $live; };

if (($handle = fopen($csv_file, "r")) !== FALSE)
{
    $dir = dirname($csv_file);
    $f = basename($csv_file);
    if (!$dry_run)
    {
      $csv_u = fopen($dir."/unprocessed.csv", "w");
      $csv_p = fopen($dir."/processed.csv", "w");
    }
    $csv_l = fopen($dir."/lookup.csv", "w");
    fputcsv($csv_l, array('file', 'uin', 'fin', 'step', 'donor_id', 'reason'));
    while (($data = fgetcsv($handle, 1000, ",")) !== FALSE)
    {
    	$id_no = trim(str_replace( array(" ", ",", "-"), '', $data[1]));
      $tmp = explode("_", basename($data[0]));
      list($found, $step, $reason) = tenbe_pick($fy_map, $loader, $id_no, isset($tmp[1]) ? $tmp[1] : '');
      $row = $found ? array('d_email' => (string) $found['email'], 'd_contact' => (string) $found['contact']) : FALSE;
      fputcsv($csv_l, array(basename($data[0]), $id_no, $fin, $step, $found ? $found['donor_id'] : '', $reason));
      if ($dry_run)
        continue;
      $email_sent = false;
      $whatsapp_sent = false;
      if ($row && $id_no && ( trim($row['d_email']) || trim($row['d_contact']) ))
    	{
        $row['d_email'] = trim($row['d_email']);
        $row['d_contact'] = trim($row['d_contact']);
    	   //echo "$id_no => ".$row['d_email']." -> ".$row['d_contact']."\n";

    	   if ($row['d_email'])
    	   {
		   $body1 = "Dear {$tmp[1]},\n
We hope this message finds you well.\n
Thank you for your generous donation to $site_display during the financial year $fin. We deeply appreciate your support.\n
As per the Income Tax Department guidelines, all charitable organizations are required to report donations received and issue Form 10BE certificates to donors. Please find attached your Form 10BE certificate for your records. This certificate can be used to claim income tax benefits under applicable provisions.\n
If you made multiple donations, you may receive more than one email or WhatsApp message.\n
Should you have any questions or need assistance, please feel free to reach out to us at <strong>$site_email</strong>.\n
With warm regards,\n\n
<strong>Accounts Team</strong>
<strong>$site_display</strong>";
      		$body = "Dear ".$tmp[1]."\n\nPlease find attached Form 10 BE for financial year ".$fin."\n\nRegards,\n-Accounts Team\n($site)\n";
      		//$ret_email = send_email_generic($row['d_email'], $site." - Form 10 BE", nl2br($body), array($data[0]));
      		$ret_email = send_email_generic($row['d_email'], $subject, nl2br($body1), array($data[0]));
          $email_sent = $ret_email['success'];
    	   }
         if ($row['d_contact'])
         {
          $ret_whatsapp = send_whatsapp_generic($row['d_contact'], "form_10be_quick", array($tmp[1], $fin, $site), $data[0]);
          $whatsapp_sent = $ret_whatsapp['success'];
         }
    	}

      if($email_sent || $whatsapp_sent)
      {
        $fields = array(  basename($data[0]), $id_no, $row['d_email'], $row['d_contact']);
        fputcsv($csv_p, $fields);
        $new_path = str_replace("unprocessed", "processed", $data[0]);
        $b_folder = dirname($new_path);

        if (! file_exists($b_folder))
          mkdir($b_folder, 0755, true);

        rename($data[0], $new_path);
      }
      else
      {
        $fields = array(  basename($data[0]), $id_no);
        fputcsv($csv_u, $fields);
      }

    }
    if (!$dry_run)
    {
      fclose($csv_p);
      fclose($csv_u);
    }
    fclose($csv_l);
    fclose($handle);
}
