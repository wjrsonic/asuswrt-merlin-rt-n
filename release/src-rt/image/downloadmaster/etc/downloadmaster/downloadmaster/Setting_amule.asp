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
<title>aMule Setting</title>
<link rel="stylesheet" type="text/css" href="index_style.css"> 
<link rel="stylesheet" type="text/css" href="form_style.css">
<link rel="stylesheet" type="text/css" href="routercss.css">
<link rel="stylesheet" type="text/css" href="ext/css/ext-all.css">
<!--<link href="multiLanguageCss/english.css" rel="stylesheet" type="text/css" id="languageCss" />
<link href="multiLanguageCss/english_1.css" rel="stylesheet" type="text/css" id="languageCss_1" />-->
<script type="text/javascript" src="jquery.js"></script>
<script>
var $j = jQuery.noConflict();
var multi_INT = 0;
var generalamule_data_tmp;
var AUTOLOGOUT_MAX_MINUTE_TMP=0;
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
	generalamule_data_tmp=data;
	var lang = array[14];
	AUTOLOGOUT_MAX_MINUTE_TMP=array[30];
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
<script type="text/javascript" src="multiLanguage_setting.js"></script>
<script type="text/javascript" src="multiLanguage_task.js"></script>
<script type="text/javascript" src="multiLanguage_all.js"></script>
<script language="JavaScript" type="text/javascript" src="state.js"></script>
<script language="JavaScript" type="text/javascript" src="general.js"></script>
<script language="JavaScript" type="text/javascript" src="popup.js"></script>
<script type="text/javascript" src="ext/ext-base.js"></script>
<script type="text/javascript" src="ext/ext-all.js"></script>
<!--<script type="text/javascript" language="JavaScript" src="/help.js"></script>-->
<script type="text/javascript" language="JavaScript" src="detect.js"></script>
<script type="text/javascript" src="plugin/jquery.iphone-switch.js"></script>
<script type="text/javascript" src="Setting.js"></script>
<style type="text/css">
a:focus{outline:none;}
</style>
<script>

function initial(){
	show_menu();
	initial_amule();
	//document.getElementById("select_lang_"+multi_INT).selected = true;
	get_Refresh_time(generalamule_data_tmp);
 	get_amule_server_status();
	setInterval("get_amule_server_status();",5000);
	AUTOLOGOUT_MAX_MINUTE=AUTOLOGOUT_MAX_MINUTE_TMP;
	updateStatus_AJAX();
}
function get_Refresh_time(data){
	/*var url = "dm_apply.cgi";
	var action_mode = "initial";
	var type = "General";
	url += "?action_mode=" + action_mode + "&download_type=" +type+ "&t=" +Math.random();
	$j.ajax({
			url:url,
			success:function(data){initial_Refresh_time(data)},
			error:function(XMLHttpRequest, textStatus, errorThrown){error_Refresh_time()}
			});*/
	var initial_array = new Array();
	eval("initial_array="+data);
	document.getElementById("helpAddress").href = "http://"+ location.host +"/downloadmaster/help.asp";
		if(location.host.split(":")[0] != initial_array[10]){
			if(document.getElementById("handToPhone"))
			document.getElementById("handToPhone").href = "http://"+ location.host.split(":")[0] +":" + initial_array[13]+"/downloadmaster/task_hand.asp";
			}
		else{
			if(document.getElementById("handToPhone"))
				document.getElementById("handToPhone").href = "http://"+ location.host +"/downloadmaster/task_hand.asp";
			}
	}

function initial_Refresh_time(data){
	var initial_array = new Array();
	eval("initial_array="+data);
	document.getElementById("helpAddress").href = "http://"+initial_array[10]+":8081/downloadmaster/help.asp";
	}

function checkIPCon(){
	updateStatusCounter=0;
	var x = true;
	//if(document.getElementById('Password').readOnly == false){
		var a = document.getElementById('aMuleServerIP').value;
		var b = document.getElementById('aMuleServerPort').value;
		var digit = /^\d*$/
		var IP = /^(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])(\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])){3}$/
		if(a=="" || b==""){
			x = false;
			Ext.MessageBox.alert(multiLanguage_setting_array[multi_INT][39],multiLanguage_setting_array[multi_INT][93]);
		}
		else if(!digit.test(b) || b > 65535 || b < 0){
			x = false;
			Ext.MessageBox.alert(multiLanguage_setting_array[multi_INT][39],multiLanguage_setting_array[multi_INT][94]);
			}
		else if(!IP.test(a)){
			x = false;
			Ext.MessageBox.alert(multiLanguage_setting_array[multi_INT][39],a+multiLanguage_setting_array[multi_INT][95]);
			}
		//}
		if(amule_status_tmp=="disconnected2"){
			x = false;
			Ext.MessageBox.alert(multiLanguage_setting_array[multi_INT][39],multiLanguage_setting_array[multi_INT][114]);
		}
		if(x)
		aMuleapplyRuleCon(a,b);
	}
function checkIPDiscon(){
	updateStatusCounter=0;
	var x = true;
	//if(document.getElementById('Password').readOnly == false){
		var a = document.getElementById('aMuleServerIP').value;
		var b = document.getElementById('aMuleServerPort').value;
		var digit = /^\d*$/
		var IP = /^(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])(\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])){3}$/
		if(a=="" || b==""){
			x = false;
			Ext.MessageBox.alert(multiLanguage_setting_array[multi_INT][39],multiLanguage_setting_array[multi_INT][93]);
		}
		else if(!digit.test(b) || b > 65535 || b < 0){
			x = false;
			Ext.MessageBox.alert(multiLanguage_setting_array[multi_INT][39],multiLanguage_setting_array[multi_INT][94]);
			}
		else if(!IP.test(a)){
			x = false;
			Ext.MessageBox.alert(multiLanguage_setting_array[multi_INT][39],a+multiLanguage_setting_array[multi_INT][95]);
			}
		//}
		if(x)
		aMuleapplyRuleDiscon(a,b);
	}
	
function judgeDigit(){
	var digit = /^\d*$/
	var port = document.getElementById("aMuleServerPort").value;
	if(!digit.test(port)){
		Ext.MessageBox.alert(multiLanguage_setting_array[multi_INT][39],multiLanguage_setting_array[multi_INT][96]);
		document.getElementById("aMuleServerPort").value = "";
	}
	}
</script>
</head>

<body onload="initial();" onunLoad="return unload_body();">
<div id="TopBanner"></div>

<div id="Loading" class="popup_bg"></div>

<iframe name="hidden_frame" id="hidden_frame" src="" width="0" height="0" frameborder="0"></iframe>
<form method="get" name="form" id="Setting_NZB_form" action="dm_apply.cgi" target="hidden_frame" />
<input type="hidden" name="action_mode" value="DM_APPLY" />
<input type="hidden" name="download_type" value="amule" />


<table class="content" align="center" cellpadding="0" cellspacing="0">
	<tr>
		<td width="17">&nbsp;</td>
		
		<td valign="top" width="202">				
		<div  id="mainMenu"></div>	
		<div  id="subMenu"></div>		
		</td>				
		
    <td valign="top">
		<!-- delete by alan <div id="tabMenu" class="submenuBlock"></div> -->
		<!--===================================Beginning of Main Content===========================================-->
        
        <div style="margin-top:-150px; padding-left:0px;">
<table width="98%" border="0" align="left" cellpadding="0" cellspacing="0">
	<tr>
		<td align="left" valign="top" >
          
		<table width="760px" border="0" cellpadding="5" cellspacing="0" class="FormTitle" id="FormTitle">
		<tbody>
			<tr>
		  		<td bgcolor="#4D595D">
		  		<div>&nbsp;</div>
		  		<div class="formfonttitle" id="multiSetting_30"></div>
		  		<div style="margin-left:5px;margin-top:10px;margin-bottom:10px"><img src="images/New_ui/export/line_export.png"></div>
		 		  <!-- delete by alan  <div class="formfontdesc"><#LANHostConfig_x_DDNSEnable_sectiondesc#></div>-->

			<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3"  class="FormTable">
			<tr><td colspan="2" id="multiSetting_31"></td></tr>
            <tr>
            	<th id="multiSetting_32"></th>
            	<td><input type="text" class="input_15_table" maxlength="15" name="aMuleServerIP" id="aMuleServerIP" />&nbsp;&nbsp;&nbsp;<span name="aMule_server_status_string" id="aMule_server_status_string" style="FONT-SIZE:10pt;color: #ff0000"></span>
					<img id="aMule_server_status_pic" src="images/statusIcon/Initialing.gif" alt="" /></td>
            </tr>
             <tr>
            	<th id="multiSetting_33"></th>
            	<td><input type="text" maxlength="5" class="input_15_table" value="" name="aMuleServerPort" id="aMuleServerPort" onkeyup="judgeDigit();" /></td>
            </tr>
		</table>
		
				<div class="apply_gen">
					<input id="aMule_Connection" class="button_gen" onclick="checkIPCon();" type="button" value="" />
                    <input id="aMule_Disconnection" class="button_gen" onclick="checkIPDiscon();" type="button" value="" />
				</div>		
		
			  </td>
              </tr>
            </tbody>

            </table>
		  </td>
</form>


        </tr>
      </table>	
      </div>			
		<!--===================================Ending of Main Content===========================================-->		
	</td>
		
    <td width="10" align="center" valign="top">&nbsp;</td>
	</tr>
</table>

<div id="footer"></div>
<script>
$j("#multiSetting_30").html(multiLanguage_setting_array[multi_INT][65]);
$j("#multiSetting_31").html(multiLanguage_setting_array[multi_INT][66]);
$j("#multiSetting_32").html(multiLanguage_setting_array[multi_INT][67]);
$j("#multiSetting_33").html(multiLanguage_setting_array[multi_INT][68]);
document.getElementById("aMule_Connection").value=multiLanguage_setting_array[multi_INT][69];
document.getElementById("aMule_Disconnection").value=multiLanguage_setting_array[multi_INT][70];
</script>
</body>
</html>
