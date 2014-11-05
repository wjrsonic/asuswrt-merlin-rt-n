<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<html xmlns:v>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta HTTP-EQUIV="Pragma" CONTENT="no-cache">
<meta HTTP-EQUIV="Cache-Control" CONTENT="no-cache">
<meta HTTP-EQUIV="Expires" CONTENT="-1">
<link rel="shortcut icon" href="images/favicon.png">
<link rel="icon" href="images/favicon.png">
<title>Help</title>
<link href="sliderplugin/slider_css.css" rel="stylesheet" type="text/css" />
<link rel="stylesheet" type="text/css" href="index_style.css">
<link rel="stylesheet" type="text/css" href="NM_style.css">
<link rel="stylesheet" type="text/css" href="other.css">
<link rel="stylesheet" type="text/css" href="form_style.css">
<link rel="stylesheet" type="text/css" href="routercss.css">
<link rel="stylesheet" type="text/css" href="ext/css/ext-all.css">
<link rel="stylesheet" href="sliderplugin/jquery.tabs.css" type="text/css" media="print,projection,screen" />
<!--<link href="english.css" rel="stylesheet" type="text/css" id="languageCss" />
<link href="english_1.css" rel="stylesheet" type="text/css" id="languageCss_1" />-->
<script type="text/javascript" src="jquery.js"></script>
<script>
var utility_ver="";
var $j = jQuery.noConflict();
var multi_INT = 0;
var url = "dm_apply.cgi";
var action_mode = "initial";
var type = "General";
url += "?action_mode=" + action_mode + "&download_type=" +type+ "&t=" +Math.random();
$j.ajax({url: url,
		async: false,
		success: function(data){initial_multi_INT_status(data)}
		});
function initial_multi_INT_status(data){
	var array = new Array();
	eval("array="+data);
	var lang = array[14];
	utility_ver = array[28];
	if(lang == "EN")
	multi_INT = 0;
	else if(lang == "TW")
	multi_INT = 1;
	else if(lang == "CN")
	multi_INT = 2;
	else if(lang == "RU")
	multi_INT = 3;
	else if(lang == "FR")
	multi_INT = 4;
	else if(lang == "DE")
	multi_INT = 5;
	else if(lang == "BR")
	multi_INT = 6;
	else if(lang == "CZ")
	multi_INT = 7;
	else if(lang == "DA")
	multi_INT = 8;
	else if(lang == "FI")
	multi_INT = 9;
	else if(lang == "MS")
	multi_INT = 10;
	else if(lang == "NO")
	multi_INT = 11;
	else if(lang == "PL")
	multi_INT = 12;
	else if(lang == "SV")
	multi_INT = 13;
	else if(lang == "TH")
	multi_INT = 14;
	else if(lang == "TR")
	multi_INT = 15;
	else if(lang == "JP")
	multi_INT = 16;
	else if(lang == "IT")
	multi_INT = 17;
	else if(lang == "HU")
	multi_INT = 18;
	else if(lang == "RO")
	multi_INT = 19;
	else if(lang == "UK")
	multi_INT = 20;
	else if(lang == "ES")
	multi_INT = 21;
	else
	multi_INT = 0;
	}
</script>
<script type="text/javascript" src="multiLanguage_all.js"></script>
<script type="text/javascript" src="multiLanguage_help.js"></script>
<script type="text/javascript" src="state.js"></script>
<script type="text/javascript" src="general.js"></script>
<script type="text/javascript" src="popup.js"></script>
<!--<script type="text/javascript" src="/help.js"></script>-->
<script type="text/javascript" src="detect.js"></script>
<script type="text/javascript" src="ext/ext-base.js"></script>
<script type="text/javascript" src="ext/ext-all.js"></script>
<script type="text/javascript" src="sliderplugin/jquery-easing.1.2.pack.js"></script>
<script type="text/javascript" src="sliderplugin/jquery-easing-compatibility.1.2.pack.js"></script>
<script type="text/javascript" src="sliderplugin/coda-slider.1.1.1.pack.js"></script>
<style type="text/css">
.mask_bg{
	position:absolute;	
	margin-top:-160px;
	width:100%;
	height:100%;
	z-index:100;
	/*background-color: #FFF;*/
	background:url(images/popup_bg2.gif);
	background-repeat: repeat;
	filter:progid:DXImageTransform.Microsoft.Alpha(opacity=60);
	-moz-opacity: 0.6;
	display:none;
	/*visibility:hidden;*/
	overflow:hidden;
}
.icon{	
	width:40px;
	height:40px;
	margin-left:2px;
	float:left;
	text-decoration:none;
}
.icon a:link, .icon a:visited{
	cursor:pointer;
	display: block;
	width:40px;	
	height:40px;
}
#icon_back{background:url(images/icon/back.png)}
#icon_back a:active{background:url(images/icon/back_click.png);}
div.wrapper{margin:0 auto;width:730px;}
td.sidenav{width:200px;}
body{font-family:Verdana, Tohoma, Arial, Helvetica, sans-serif;
padding:0;
margin:0;}
.wrapperDesc{margin:0 auto; width:570px;}

</style>

<script>
jQuery(window).bind("load", function() {
jQuery("div#slider1").codaSlider()
jQuery("div#slider2").codaSlider()
});
	
function initial(){
	show_menu();
	//initial_general();
	setTimeout("showMethod('','none');", 100);
	//setTimeout('update_appstate();', 100);
	//document.getElementById("select_lang_"+multi_INT).selected = true;
	document.getElementById("DMUtiltyLink").href = "http://dlcdnet.asus.com/pub/ASUS/wireless/ASUSWRT/"+utility_ver;
	get_Refresh_time();
}
function get_Refresh_time(){
	/*var url = "dm_apply.cgi";
	var action_mode = "initial";
	var type = "General";
	url += "?action_mode=" + action_mode + "&download_type=" +type+ "&t=" +Math.random();
	$j.ajax({
			url:url,
			success:function(data){initial_Refresh_time(data)},
			error:function(XMLHttpRequest, textStatus, errorThrown){error_Refresh_time()}
			});*/
	document.getElementById("helpAddress").href = "http://"+ location.host +"/downloadmaster/help.asp";
	document.getElementById("backAddress").href = "http://"+ location.host +"/downloadmaster/index.asp";
	}

function initial_Refresh_time(data){
	var initial_array = new Array();
	eval("initial_array="+data);
	document.getElementById("backAddress").href = "http://"+initial_array[10]+":8081/downloadmaster/index.asp";
	document.getElementById("helpAddress").href = "http://"+initial_array[10]+":8081/downloadmaster/help.asp";
	}

function showMethod(flag1, flag2){
	document.getElementById("method1").style.display = flag1;
	document.getElementById("method1Title").style.display = flag1;
	document.getElementById("method2").style.display = flag2;
	document.getElementById("method2Title").style.display = flag2;
}
</script>
</head>

<body onload="initial();" onunLoad="return unload_body();">
<div id="TopBanner"></div>
<!--<div id="DM_mask" class="mask_bg"></div>-->
<div id="Loading" class="popup_bg"></div>
<iframe name="hidden_frame" id="hidden_frame" src="" width="0" height="0" frameborder="0"></iframe>

<!--<input type="hidden" class="input_12_table" maxlength="12" name="Base_path" id="Base_path" />-->

<table class="content" align="center" cellpadding="0" cellspacing="0">
  <tr>
	<td width="17">&nbsp;</td>
	<!--=====Beginning of Main Menu=====-->
	<td valign="top" width="202">
	  <div id="mainMenu"></div>
	  <div id="subMenu"></div>
	</td>
	
	<td height="480" valign="top">
	  <!--  delete by alan <div id="tabMenu" class="submenuBlock"></div>-->
	  
	  <!--===================================Beginning of Main Content===========================================-->
      <div class="app_table" style="margin-top:-150px; position:relative;">
<table bgcolor="#4D595D" style="width:760px;">
  <tr>
  	<td class="">
           
            <div class="formfonttitle"><table><tr><td valign="bottom" id="multiHelp_0"></td>
             <td><table id="BackTask" style="margin-left:550px;">
            	<tbody>
                	<tr>
                    	<td align="center">
                        <div class="icon" id="icon_back" title="Back"><a id="backAddress" href=""></a></div>
                        </td>
                        <td>&nbsp;</td>
                    </tr>
                    	<td align="center" valign="middle" id="multiHelp_1"></td>
                        <td>&nbsp;</td>
                    </tr>
                </tbody>
            </table>
            </td>
            </tr>
            </table>
            </div>
			<div style="margin-left:5px;margin-top:10px;margin-bottom:10px"><img width="745px" height="2px" src="images/New_ui/export/line_export.png"></div>
		</td>
  </tr>

  <tr>
   	<td valign="top" >
		<div id="partition_div"></div>
	<div id="DMDesc">
		<div style="margin-left:10px;display:none;" id="isInstallDesc">
			<h2>Download Master install success!</h2>
			<p class="MethodDesc">To use the Download master for continuous Downloading when PC is off.Please refer to the following few steps.</p>
		</div>
		<div id="mainbody">
			<div class="wrapper">
				<div class="shadow-l">
					<div class="shadow-r">
						<table class="" cellspacing="0" cellpadding="0">
							<tbody><tr valign="top">
								<td class="">
									<div class="padding">
										<div class="">
											<ul class="" style="margin-left:10px;">
												<li><p onclick="showMethod('','none');" style="cursor:pointer;text-decoration:underline;font-weight:bolder;" id="multiHelp_2">Use Download Master</p></li>    
												<li><p onclick="showMethod('none','');" style="cursor:pointer;text-decoration:underline;font-weight:bolder;" id="multiHelp_3">Install the Windows Utiity </p></li>         
											</ul>	
											<div class="moduletable">
												<center>		
													<div id="container" >
														<div class="top-heading" id="method1Title">
													  	<h2 id="multiHelp_4">Use Download Master</h2>
															<!--p class="MethodDesc">To use the Download master for continuous Downloading when PC is off.Please refer to the following few steps.</p-->
														</div>
														<div class="slider-wrap" id="method1">
															<div class="stripNavL" id="stripNavL0"><a href="#">Left</a></div><div class="stripNav" id="stripNav0" style="width: 10px; "><ul><li class="tab1" style=""><a href="#1">Step 1</a></li><li class="tab2" style=""><a href="#2">Step 2</a></li><li class="tab3" style=""><a href="#3">Step 3</a></li><li class="tab4" style=""><a href="#4" class="current">Step 4</a></li><li class="tab5" style=""><a href="#5">Step 5</a></li></ul></div><div id="slider1" class="stripViewer">
																<div class="panelContainer" style="width: 3000px; left: 0px; ">
																	<div class="panel">
																		<div class="wrapperDesc">
																			<div style="color: #FFF; text-align: left;">
																				<font size="3"><b><span id="multiHelp_5">Step</span> 1 <br/></b></font> 
																				<span id="multiHelp_6">Click the Download Master name or icon in the USB application page.</span>           
																			</div>
																			<p>
																				<img src="images/appinstall/DM_Step1.jpg" border="0" hspace="0" vspace="0" alt="" class="imdShade">
																			</p>
																		</div>
																	</div>
																	<div class="panel">	
																		<div class="wrapperDesc">
																			<div style="color: #FFF; text-align: left;">
																				<font size="3"><b><span id="multiHelp_7">Step</span> 2 <br/></b></font>
 																				<span id="multiHelp_8">The Download Master will open in new browser window. Click "Add" icon to add download task</span>           
																			</div>
																			<p>
																				<img src="images/appinstall/DM_Step2.jpg" border="0" hspace="0" vspace="0" alt="" class="imdShade">
																			</p>
																		</div>
																	</div>		
																	<div class="panel">
																		<div class="wrapperDesc">
																			<div style="color: #FFF; text-align: left;">
																				<font size="3"><b><span id="multiHelp_9">Step</span> 3 <br/></b></font>
																				<span id="multiHelp_10">To download the BitTorrent or NZB file, click the "choose file" button to upload  *.torrent or *.nzb file.</span>
																			</div>
																			<p>
																				<img src="images/appinstall/DM_Step3.jpg" border="0" hspace="0" vspace="0" alt="" class="imdShade">
																			</p>
																		</div>
																	</div>
																	<div class="panel">
																		<div class="wrapperDesc">
																			<div style="color: #FFF; text-align: left;">
																				<font size="3"><b><span id="multiHelp_11">Step</span> 4 <br/></b></font> 
																				 <span id="multiHelp_12">To download the FTP, HTTP or Magnet link, copy and paste the URL to the URL field</span>           
																			</div>
																			<p>
																				<img src="images/appinstall/DM_Step4.jpg" border="0" hspace="0" vspace="0" alt="" class="imdShade">
																			</p>
																		</div>
																	</div>
                                                                     <div class="panel">
																		<div class="wrapperDesc">
																			<div style="color: #FFF; text-align: left;">
																				<font size="3"><b><span id="multiHelp_13">Step</span> 5 <br/></b></font> 
																				<span id="multiHelp_14">If the FTP site needs username, password, there is an exclamation mark <img src="images/appinstall/warning_16X16.png"> in the task area.</span>
                                                                                <span id="multiHelp_15">Please click the exclamation mark to input the username, password.</span>              
																			</div>
																			<p>
																				<img src="images/appinstall/DM_Step5.jpg" border="0" hspace="0" vspace="0" alt="" class="imdShade">
																			</p>
																		</div>
																	</div>
																</div>
															</div><div class="stripNavR" id="stripNavR0"><a href="#">Right</a></div>
														</div>

														<div class="top-heading" id="method2Title">
															<a name="FTP_Upload"></a>
													  	<h2 id="multiHelp_16">Install the Windows Utiity</h2>
														</div>														
														<div class="slider-wrap" id="method2">
															<div class="stripNavL" id="stripNavL1"><a href="#">Left</a></div><div class="stripNav" id="stripNav1" style="width: 6px; "><ul><li class="tab1" style=""><a href="#1" class="current">Step 1</a></li><li class="tab2" style=""><a href="#2">Step 2</a></li></ul></div><div id="slider2" class="stripViewer">
																<div class="panelContainer" style="left: 0px; ">																		
																	<div class="panel">
																		<div class="wrapperDesc">
																			<div style="color: #FFF; text-align: left;">
																				<font size="3"><b><span id="multiHelp_17">Step</span> 1 <br/></b></font><span id="multiHelp_18">Download and install the Windows Utility.</span>&nbsp;&nbsp;
																				<a id="DMUtiltyLink" href="http://dlcdnet.asus.com/pub/ASUS/wireless/ASUSWRT/DM2_2033.zip" style="text-decoration:underline;font-size:14px;font-weight:bolder;"><span id="multiHelp_19">Download Now!</span></a>  
																			</div>
																			<p>
																				<img src="images/PrinterServer/DM_Utility1.jpg" border="0" hspace="0" vspace="0" alt="" class="imdShade">
																			</p>
																		</div>
																	</div>
																	<div class="panel">
																		<div class="wrapperDesc">																			
																			<div style="color: #FFF; text-align: left;">
																				<font size="3"><b><span id="multiHelp_20">Step</span> 2 <br/></b></font> 
																				<span id="multiHelp_21">After installed the utility, double click the torent file and it will be added to download master task automatically.</span>
																			</div>  
																			<p>
																				<img src="images/PrinterServer/DM_Utility2.jpg" border="0" hspace="0" vspace="0" alt="" class="imdShade">
																			</p>
																		</div>
																	</div>		
																</div>
															</div><div class="stripNavR" id="stripNavR1"><a href="#">Right</a></div>
														</div>		

													</div>														
												</center>			
											</div>
										</div>	
									<span class="article_seperator">&nbsp;</span>
								</div>
							</td>
						</tr>
					</tbody></table>													
				</div>
			</div>
		</div>
	</div>

		<div><input id="return_btn" style="margin-left:300px;display:none;" type="button" class="button_gen" value="Return" onclick="show_apps();"></div>
   	</td> 
  </tr>  
	   
  <tr>
   	<td valign="top" id="app_table_td" height="400px">
				<div id="app_table"></div>
   	</td> 
  </tr>  
</table>
</div>
      
		<!--===================================Ending of Main Content===========================================-->
	
    <td width="10" align="center" valign="top">&nbsp;</td>
	</tr>
</table>

<div id="footer"></div>
<script>
$j("#multiHelp_0").html(multiLanguage_help_array[multi_INT][0]);
$j("#multiHelp_1").html(multiLanguage_help_array[multi_INT][1]);
$j("#multiHelp_2").html(multiLanguage_help_array[multi_INT][2]);
$j("#multiHelp_3").html(multiLanguage_help_array[multi_INT][3]);
$j("#multiHelp_4").html(multiLanguage_help_array[multi_INT][2]);
$j("#multiHelp_5").html(multiLanguage_help_array[multi_INT][4]);			
$j("#multiHelp_6").html(multiLanguage_help_array[multi_INT][5]);
$j("#multiHelp_7").html(multiLanguage_help_array[multi_INT][4]);			
$j("#multiHelp_8").html(multiLanguage_help_array[multi_INT][6]);
$j("#multiHelp_9").html(multiLanguage_help_array[multi_INT][4]);			
$j("#multiHelp_10").html(multiLanguage_help_array[multi_INT][7]);
$j("#multiHelp_11").html(multiLanguage_help_array[multi_INT][4]);			
$j("#multiHelp_12").html(multiLanguage_help_array[multi_INT][8]);
$j("#multiHelp_13").html(multiLanguage_help_array[multi_INT][4]);			
$j("#multiHelp_14").html(multiLanguage_help_array[multi_INT][9]);
$j("#multiHelp_15").html(multiLanguage_help_array[multi_INT][10]);
$j("#multiHelp_16").html(multiLanguage_help_array[multi_INT][3]);
$j("#multiHelp_17").html(multiLanguage_help_array[multi_INT][4]);
$j("#multiHelp_18").html(multiLanguage_help_array[multi_INT][11]);
$j("#multiHelp_19").html(multiLanguage_help_array[multi_INT][12]);
$j("#multiHelp_20").html(multiLanguage_help_array[multi_INT][4]);
$j("#multiHelp_21").html(multiLanguage_help_array[multi_INT][13]);
</script>
</body>
</html>
