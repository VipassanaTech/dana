<?php
/*
	-- PHP Image Generator --
	By Ryan Maffit
	http://www.ryanmaffit.com/
	
	-- Original Author --
    Dynamic Heading Generator
    By Stewart Rosenberger
    http://www.stewartspeak.com/headings/    

   -- Included in donation module
   Created a function to be a part of donation module
   By Vinay Ramdas	
*/

function sanitize( $text )
{
    return str_replace(array(' ','.',"'" ), "-", $text);
}

function generate_sign( $text )
{
    $presets = array(
        'testimonials'  =>  array(
            'font'  =>  'journal.ttf',
            'size'  =>  '36',
            'font_color'    =>  '3e4b4b',
            'bg_color'  =>  'ffffff',
            'transparent_background'    =>  true,
        )
    );

    $font_path = dirname(__FILE__).'/fonts';

    $preset = $presets['testimonials'];
    
    $font_file = $font_path.'/'.$preset['font'];
    $font_size  = $preset['size']; // font size in pts
    $font_color = '#' . $preset['font_color'];
    $background_color = '#' . $preset['bg_color'];
    $transparent_background  = $preset['transparent_background'];

    $cache_images = true;
    $cache_folder = 'sign-cache';

    $mime_type = 'image/png';
    $extension = '.png';
    $send_buffer_size = 4096;

    // check for GD support
    if(!function_exists('ImageCreate'))
        fatal_error('Error: Server does not support PHP image generation');

    // clean up text
    if($text == "")
        return false; 

    // limit the maximum number of characters to 100 so that someone can't slow down the server
    $text = strlen($text) > 100 ? substr($text, 0, 100) : $text;
        
    // decode text
    $text = html_entity_decode($text);
    if(get_magic_quotes_gpc())
        $text = stripslashes($text);

    // convert javascript to html
    $text = javascript_to_html($text);

    // look for cached copy, send if it exists
    $hash = md5(basename($font_file) . $font_size . $font_color . $background_color . $transparent_background . $text);
    $hash = sanitize($text);
    $cache_filename = conf_path().'/files/'.$cache_folder . '/' . $hash . $extension;

    if ( file_exists($cache_filename) )
        return $cache_filename;

    // check font availability
    $font_found = is_readable($font_file);
    if(!$font_found)
    {
        fatal_error('Error: The server is missing the specified font.');
        return false;
    }

    // create image
    $background_rgb = hex_to_rgb($background_color);
    $font_rgb = hex_to_rgb($font_color);
    $dip = get_dip($font_file,$font_size);
    $box = @ImageTTFBBox($font_size,0,$font_file,$text);
    $image = @ImageCreate(abs($box[2]-$box[0]),abs($box[5]-$dip));

    if(!$image || !$box)
    {
        fatal_error('Error: The server could not create this heading image.');
        return false;
    }

    // allocate colors and draw text
    $background_color = @ImageColorAllocate($image,$background_rgb['red'],
        $background_rgb['green'],$background_rgb['blue']);

    $font_color = ImageColorAllocate($image,$font_rgb['red'],
        $font_rgb['green'],$font_rgb['blue']);

    ImageTTFText($image,$font_size,0,-$box[0],abs($box[5]-$box[3])-$box[1],
        $font_color,$font_file,$text);

    // set transparency
    if($transparent_background)
        ImageColorTransparent($image,$background_color);

//    header('Content-type: ' . $mime_type);
    //ImagePNG($image);
    // save copy of image for cache
    if($cache_images)
    {
        @ImagePNG($image,$cache_filename);
    }
    ImageDestroy($image);
    return $cache_filename;    
}


// boundary function
function get_dip($font,$size)
{
	$test_chars = 'abcdefghijklmnopqrstuvwxyz' .
			      'ABCDEFGHIJKLMNOPQRSTUVWXYZ' .
				  '1234567890' .
				  '!@#$%^&*()\'"\\/;.,`~<>[]{}-+_-=';
	$box = @ImageTTFBBox($size,0,$font,$test_chars);
	return $box[3];
}

// fatal error
function fatal_error($message)
{
    // send an image
    if(function_exists('ImageCreate'))
    {
        $width = ImageFontWidth(5) * strlen($message) + 10;
        $height = ImageFontHeight(5) + 10;
        if($image = ImageCreate($width,$height))
        {
            $background = ImageColorAllocate($image,255,255,255);
            $text_color = ImageColorAllocate($image,0,0,0);
            ImageString($image,5,5,5,$message,$text_color);    
            header('Content-type: image/png');
            ImagePNG($image);
            ImageDestroy($image);
            exit;
        }
    }
    // send 500 code
    header("HTTP/1.0 500 Internal Server Error");
    print($message);
    exit;
}

// hex to RGB
function hex_to_rgb($hex)
{
    // remove '#'
    if(substr($hex,0,1) == '#')
        $hex = substr($hex,1);
    // expand short form ('fff') color
    if(strlen($hex) == 3)
    {
        $hex = substr($hex,0,1) . substr($hex,0,1) .
               substr($hex,1,1) . substr($hex,1,1) .
               substr($hex,2,1) . substr($hex,2,1);
    }
    if(strlen($hex) != 6)
        fatal_error('Error: Invalid color "'.$hex.'"');
    // convert
    $rgb['red'] = hexdec(substr($hex,0,2));
    $rgb['green'] = hexdec(substr($hex,2,2));
    $rgb['blue'] = hexdec(substr($hex,4,2));
    return $rgb;
}

// convert embedded, javascript unicode characters into embedded HTML entities
function javascript_to_html($text)
{
    $matches = null;
    preg_match_all('/%u([0-9A-F]{4})/i',$text,$matches);
    if(!empty($matches)) for($i=0;$i<sizeof($matches[0]);$i++)
        $text = str_replace($matches[0][$i],
                            '&#'.hexdec($matches[1][$i]).';',$text);
    return $text;
}
?>