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
   echo "Usage: php ".$argv[0]." <subdomain> <csv-file>\n\n";
   exit(1);
}

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

$fin = (date('Y')-1)."-".date('Y');

echo "financial year: $fin\n";

$site = variable_get("site_name", "");
echo $site."\n";

$site_email = variable_get("donation_email", "");
$site_display = variable_get("center_display_name", "");

if(!$site_display)
	$site_display = $site;

$subject = "Form 10BE Certificate for Your Donation to $site_display";

if (($handle = fopen($csv_file, "r")) !== FALSE)
{
    $dir = dirname($csv_file);
    $f = basename($csv_file);
    $csv_u = fopen($dir."/unprocessed.csv", "w");
    $csv_p = fopen($dir."/processed.csv", "w");
    while (($data = fgetcsv($handle, 1000, ",")) !== FALSE)
    {
    	$id_no = trim(str_replace( array(" ", ",", "-"), '', $data[1]));
    	$q = "select d_email, d_contact from dh_donor where REPLACE(REPLACE(REPLACE(d_id_no, ' ', ''), '\\t', ''), '\\n', '')='".$id_no."' order by d_id desc limit 1";
    	//echo "$q\n";
    	$row = db_query($q)->fetchAssoc();
      $email_sent = false;
      $whatsapp_sent = false;
      if ($row && $id_no && ( trim($row['d_email']) || trim($row['d_contact']) ))
    	{
        $row['d_email'] = trim($row['d_email']);
        $row['d_contact'] = trim($row['d_contact']);
    	   //echo "$id_no => ".$row['d_email']." -> ".$row['d_contact']."\n";

         $tmp = explode("_", basename($data[0]));

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
    fclose($csv_p);
    fclose($csv_u);
    fclose($handle);
}
