<?php

if (isset($_REQUEST) && ( isset($_REQUEST['REMOTE_IP']) ))
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

if ( date('m') > 3 )
  $fin = date('Y')."-".(date('Y')+1);
else
  $fin = (date('Y')-1)."-".date('Y');

echo "financial year: $fin\n";

$site = variable_get("site_name", "");
echo $site."\n";
if (($handle = fopen($csv_file, "r")) !== FALSE)
{
    $dir = dirname($csv_file);
    $f = basename($csv_file);
    $csv_u = fopen($dir."/unprocessed.csv", "w");
    $csv_p = fopen($dir."/processed.csv", "w");
    while (($data = fgetcsv($handle, 1000, ",")) !== FALSE)
    {
	$id_no = trim(str_replace( array(" ", ",", "-"), '', $data[1]));
	$q = "select d_email, d_contact from dh_donor where d_id_no='".$id_no."' order by d_id desc limit 1";
	//echo "$q\n";
	$row = db_query($q)->fetchAssoc();
        if ($row)
	{
	   //echo "$id_no => ".$row['d_email']." -> ".$row['d_contact']."\n";
	   $fields = array(  basename($data[0]), $id_no, $row['d_email'], $row['d_contact']);
	   fputcsv($csv_p, $fields);
	   $new_path = str_replace("unprocessed", "processed", $data[0]);
	   $b_folder = dirname($new_path);
     $tmp = explode("_", basename($data[0]));

	   if ($row['d_email'])
	   {
  		$body = "Dear ".$tmp[1]."\n\nPlease find attached Form 10 BE for financial year ".$fin."\n\nRegards,\n-Accounts Team\n($site)\n";
  		send_email_generic($row['d_email'], $site." - Form 10 BE", nl2br($body), array($data[0]));
	   }
     if ($row['d_contact'])
     {
      send_whatsapp_generic($row['d_contact'], "form_10be_2", array($tmp[1], $fin, $site), $data[0]);
     }

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
