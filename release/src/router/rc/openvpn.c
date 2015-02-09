/*

	Copyright (C) 2008-2010 Keith Moyer, tomatovpn@keithmoyer.com

	No part of this file may be used without permission.

*/

#include "rc.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <time.h>

// Line number as text string
#define __LINE_T__ __LINE_T_(__LINE__)
#define __LINE_T_(x) __LINE_T(x)
#define __LINE_T(x) # x

#define VPN_LOG_ERROR -1
#define VPN_LOG_NOTE 0
#define VPN_LOG_INFO 1
#define VPN_LOG_EXTRA 2
#define vpnlog(level,x...) if(nvram_get_int("vpn_debug")>=level) syslog(LOG_INFO, #level ": " __LINE_T__ ": " x)

#define CLIENT_IF_START 10
#define SERVER_IF_START 20

#define BUF_SIZE 256
#define IF_SIZE 8

static int ovpn_waitfor(const char *name)
{
	int pid, n = 5;

	killall_tk(name);
	while ( (pid = pidof(name)) >= 0 && (n-- > 0) )
	{
		// Reap the zombie if it has terminated
		waitpid(pid, NULL, WNOHANG);
		sleep(1);
	}
	return (pid >= 0);
}

void start_vpnclient(int clientNum)
{
	FILE *fp;
	char iface[IF_SIZE];
	char buffer[BUF_SIZE];
	char buffer2[4000];
	char *argv[6];
	int argc = 0;
	enum { TLS, SECRET, CUSTOM } cryptMode = CUSTOM;
	enum { TAP, TUN } ifType = TUN;
	enum { BRIDGE, NAT, NONE } routeMode = NONE;
	int nvi, ip[4], nm[4];
	long int nvl;
	int pid;
	int userauth, useronly;
	int taskset_ret;

	sprintf(&buffer[0], "start_vpnclient", clientNum);
	if (getpid() != 1) {
		notify_rc(&buffer[0]);
		return;
	}

	vpnlog(VPN_LOG_INFO,"VPN GUI client backend starting...");

	if ( (pid = pidof(&buffer[6])) >= 0 )
	{
		vpnlog(VPN_LOG_NOTE, "VPN Client 1 already running...", clientNum);
		vpnlog(VPN_LOG_INFO,"PID: %d", pid);
		return;
	}

	sprintf(&buffer[0], "vpn_client_state", clientNum);
	nvram_set(&buffer[0], "1");	//initializing
	sprintf(&buffer[0], "vpn_client_errno", clientNum);
	nvram_set(&buffer[0], "0");

	// Determine interface
	sprintf(&buffer[0], "vpn_client_if", clientNum);
	if ( nvram_contains_word(&buffer[0], "tap") )
		ifType = TAP;
	else if ( nvram_contains_word(&buffer[0], "tun") )
		ifType = TUN;
	else
	{
		vpnlog(VPN_LOG_ERROR, "Invalid interface type, %.3s", nvram_safe_get(&buffer[0]));
		return;
	}

	// Build interface name
	snprintf(&iface[0], IF_SIZE, "%s11", nvram_safe_get(&buffer[0]), clientNum+CLIENT_IF_START);

	// Determine encryption mode
	sprintf(&buffer[0], "vpn_client_crypt", clientNum);
	if ( nvram_contains_word(&buffer[0], "tls") )
		cryptMode = TLS;
	else if ( nvram_contains_word(&buffer[0], "secret") )
		cryptMode = SECRET;
	else if ( nvram_contains_word(&buffer[0], "custom") )
		cryptMode = CUSTOM;
	else
	{
		vpnlog(VPN_LOG_ERROR,"Invalid encryption mode, %.6s", nvram_safe_get(&buffer[0]));
		return;
	}

	// Determine if we should bridge the tunnel
	sprintf(&buffer[0], "vpn_client_bridge", clientNum);
	if ( ifType == TAP && nvram_get_int(&buffer[0]) == 1 )
		routeMode = BRIDGE;

	// Determine if we should NAT the tunnel
	sprintf(&buffer[0], "vpn_client_nat", clientNum);
	if ( (ifType == TUN || routeMode != BRIDGE) && nvram_get_int(&buffer[0]) == 1 )
		routeMode = NAT;

	// Make sure openvpn directory exists
	mkdir("/etc/openvpn", 0700);
	sprintf(&buffer[0], "/etc/openvpn/client", clientNum);
	mkdir(&buffer[0], 0700);

	// Make sure symbolic link exists
	sprintf(&buffer[0], "/etc/openvpn/vpnclient", clientNum);
	unlink(&buffer[0]);
	if ( symlink("/usr/sbin/openvpn", &buffer[0]) )
	{
		vpnlog(VPN_LOG_ERROR,"Creating symlink failed...");
		stop_vpnclient(clientNum);
		return;
	}

	// Make sure module is loaded
	modprobe("tun");
	f_wait_exists("/dev/net/tun", 5);

	// Create tap/tun interface
	sprintf(&buffer[0], "openvpn --mktun --dev %s", &iface[0]);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	if ( _eval(argv, NULL, 0, NULL) )
	{
		vpnlog(VPN_LOG_ERROR,"Creating tunnel interface %s failed...",&iface[0]);
		stop_vpnclient(clientNum);
		return;
	}

	// Bring interface up (TAP only)
	if( ifType == TAP )
	{
		if ( routeMode == BRIDGE )
		{
			snprintf(&buffer[0], BUF_SIZE, "brctl addif %s %s", nvram_safe_get("lan_ifname"), &iface[0]);
			for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
			if ( _eval(argv, NULL, 0, NULL) )
			{
				vpnlog(VPN_LOG_ERROR,"Adding tunnel interface to bridge failed...");
				stop_vpnclient(clientNum);
				return;
			}
		}

		snprintf(&buffer[0], BUF_SIZE, "ifconfig %s promisc up", &iface[0]);
		for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
		if ( _eval(argv, NULL, 0, NULL) )
		{
			vpnlog(VPN_LOG_ERROR,"Bringing interface up failed...");
			stop_vpnclient(clientNum);
			return;
		}
	}

	sprintf(&buffer[0], "vpn_client_userauth", clientNum);
	userauth = nvram_get_int(&buffer[0]);
	sprintf(&buffer[0], "vpn_client_useronly", clientNum);
	useronly = userauth && nvram_get_int(&buffer[0]);

	// Build and write config file
	vpnlog(VPN_LOG_EXTRA,"Writing config file");
	sprintf(&buffer[0], "/etc/openvpn/client/config.ovpn", clientNum);
	fp = fopen(&buffer[0], "w");
	chmod(&buffer[0], S_IRUSR|S_IWUSR);
	fprintf(fp, "# Automatically generated configuration\n");
	fprintf(fp, "daemon\n");
	if ( cryptMode == TLS )
		fprintf(fp, "client\n");
	fprintf(fp, "dev %s\n", &iface[0]);
	sprintf(&buffer[0], "vpn_client_proto", clientNum);
	fprintf(fp, "proto %s\n", nvram_safe_get(&buffer[0]));
	sprintf(&buffer[0], "vpn_client_addr", clientNum);
	fprintf(fp, "remote %s ", nvram_safe_get(&buffer[0]));
	sprintf(&buffer[0], "vpn_client_port", clientNum);
	fprintf(fp, "%d\n", nvram_get_int(&buffer[0]));
	if ( cryptMode == SECRET )
	{
		if ( ifType == TUN )
		{
			sprintf(&buffer[0], "vpn_client_local", clientNum);
			fprintf(fp, "ifconfig %s ", nvram_safe_get(&buffer[0]));
			sprintf(&buffer[0], "vpn_client_remote", clientNum);
			fprintf(fp, "%s\n", nvram_safe_get(&buffer[0]));
		}
		else if ( ifType == TAP )
		{
			sprintf(&buffer[0], "vpn_client_local", clientNum);
			fprintf(fp, "ifconfig %s ", nvram_safe_get(&buffer[0]));
			sprintf(&buffer[0], "vpn_client_nm", clientNum);
			fprintf(fp, "%s\n", nvram_safe_get(&buffer[0]));
		}
	}
	sprintf(&buffer[0], "vpn_client_retry", clientNum);
	if ( (nvi = nvram_get_int(&buffer[0])) >= 0 )
		fprintf(fp, "resolv-retry %d\n", nvi);
	else
		fprintf(fp, "resolv-retry infinite\n");
	fprintf(fp, "nobind\n");
	fprintf(fp, "persist-key\n");
	fprintf(fp, "persist-tun\n");
	sprintf(&buffer[0], "vpn_client_comp", clientNum);
	if ( nvram_get_int(&buffer[0]) >= 0 )
		fprintf(fp, "comp-lzo %s\n", nvram_safe_get(&buffer[0]));
	sprintf(&buffer[0], "vpn_client_cipher", clientNum);
	if ( !nvram_contains_word(&buffer[0], "default") )
		fprintf(fp, "cipher %s\n", nvram_safe_get(&buffer[0]));
	sprintf(&buffer[0], "vpn_client_rgw", clientNum);
	if ( nvram_get_int(&buffer[0]) )
	{
		sprintf(&buffer[0], "vpn_client_gw", clientNum);
		if ( ifType == TAP && nvram_safe_get(&buffer[0])[0] != '\0' )
			fprintf(fp, "route-gateway %s\n", nvram_safe_get(&buffer[0]));
		fprintf(fp, "redirect-gateway def1\n");
	}
	if ( (nvi = nvram_get_int("vpn_loglevel")) >= 0 )
                fprintf(fp, "verb %d\n", nvi);
        else
                fprintf(fp, "verb 3\n");
	if ( cryptMode == TLS )
	{
		sprintf(&buffer[0], "vpn_client_reneg", clientNum);
		if ( (nvl = atol(nvram_safe_get(&buffer[0]))) >= 0 )
			fprintf(fp, "reneg-sec %ld\n", nvl);

		sprintf(&buffer[0], "vpn_client_adns", clientNum);
		if ( nvram_get_int(&buffer[0]) > 0 )
		{
			sprintf(&buffer[0], "/etc/openvpn/client/updown.sh", clientNum);
			symlink("/usr/sbin/updown.sh", &buffer[0]);
			fprintf(fp, "script-security 2\n");
			fprintf(fp, "up updown.sh\n");
			fprintf(fp, "down updown.sh\n");
		}

		sprintf(&buffer[0], "vpn_client_hmac", clientNum);
		nvi = nvram_get_int(&buffer[0]);
		sprintf(&buffer[0], "vpn_crt_client_static", clientNum);
		if ( !nvram_is_empty(&buffer[0]) && nvi >= 0 )
		{
			fprintf(fp, "tls-auth static.key");
			if ( nvi < 2 )
				fprintf(fp, " %d", nvi);
			fprintf(fp, "\n");
		}

		sprintf(&buffer[0], "vpn_crt_client_ca", clientNum);
		if ( !nvram_is_empty(&buffer[0]) || check_if_file_exist("/jffs/openvpn/ca_cl.crt") )
			fprintf(fp, "ca ca.crt\n");
		if (!useronly)
		{
			sprintf(&buffer[0], "vpn_crt_client_crt", clientNum);
			if ( !nvram_is_empty(&buffer[0]) || check_if_file_exist("/jffs/openvpn/client_cl.crt") )
				fprintf(fp, "cert client.crt\n");
			sprintf(&buffer[0], "vpn_crt_client_key", clientNum);
			if ( !nvram_is_empty(&buffer[0]) || check_if_file_exist("/jffs/openvpn/client_cl.key") )
				fprintf(fp, "key client.key\n");
		}
		sprintf(&buffer[0], "vpn_client_tlsremote", clientNum);
		if (nvram_get_int(&buffer[0]))
		{
			sprintf(&buffer[0], "vpn_client_cn", clientNum);
			fprintf(fp, "tls-remote %s\n", nvram_safe_get(&buffer[0]));
		}
		if (userauth)
			fprintf(fp, "auth-user-pass up\n");

		sprintf(&buffer[0], "vpn_crt_client_crl", clientNum);
		if ( !nvram_is_empty(&buffer[0]) || check_if_file_exist("/jffs/openvpn/crl_cl.pem") )
			fprintf(fp, "crl-verify crl.pem\n");
	}
	else if ( cryptMode == SECRET )
	{
		sprintf(&buffer[0], "vpn_crt_client_static", clientNum);
		if ( !nvram_is_empty(&buffer[0]) || check_if_file_exist("/jffs/openvpn/static_cl.key") )
			fprintf(fp, "secret static.key\n");

	}

	// All other cryptmodes need a default up/down script set
	if ( (cryptMode != TLS) && (check_if_file_exist("/jffs/scripts/openvpn-event")) )
	{
		sprintf(&buffer[0], "/etc/openvpn/client/updown.sh", clientNum);
		symlink("/jffs/scripts/openvpn-event", &buffer[0]);
		fprintf(fp, "script-security 2\n");
		fprintf(fp, "up updown.sh\n");
		fprintf(fp, "down updown.sh\n");
	}

	fprintf(fp, "status-version 2\n");
	fprintf(fp, "status status\n");
	fprintf(fp, "\n# Custom Configuration\n");
	sprintf(&buffer[0], "vpn_client_custom", clientNum);
	fprintf(fp, "%s", nvram_safe_get(&buffer[0]));
	fclose(fp);

	vpnlog(VPN_LOG_EXTRA,"Done writing config file");

	// Write certification and key files
	vpnlog(VPN_LOG_EXTRA,"Writing certs/keys");
	if ( cryptMode == TLS )
	{

			if ( !check_if_dir_exist("/jffs/openvpn/") )
								{ // jffs not exist
		sprintf(&buffer[0], "vpn_crt_client_ca", clientNum);
		if ( !nvram_is_empty(&buffer[0]) )
		 {
			sprintf(&buffer[0], "/etc/openvpn/client/ca.crt", clientNum);
			fp = fopen(&buffer[0], "w");
			chmod(&buffer[0], S_IRUSR|S_IWUSR);
			sprintf(&buffer[0], "vpn_crt_client_ca", clientNum);
			fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
			fclose(fp);
		 }
								} // jffs not exist

			else 
							{ // jffs exist 
		sprintf(&buffer[0], "vpn_crt_client_ca", clientNum);
			if ( !nvram_is_empty(&buffer[0]) && !check_if_file_exist("/jffs/openvpn/ca_cl.crt") ) 
				{ //create file from nvram
				sprintf(&buffer[0], "/jffs/openvpn/ca_cl.crt", clientNum);
				fp = fopen(&buffer[0], "w");
				chmod(&buffer[0], S_IRUSR|S_IWUSR);
				sprintf(&buffer[0], "vpn_crt_client_ca", clientNum);
				fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
				fclose(fp);
				symlink("/jffs/openvpn/ca_cl.crt", "/etc/openvpn/client/ca.crt");
				nvram_unset("vpn_crt_client_ca");
				} //create file from nvram
			else if ( !nvram_is_empty(&buffer[0]) && check_if_file_exist("/jffs/openvpn/ca_cl.crt") ) 
				{ //new config sets by webui
				sprintf(&buffer[0], "/jffs/openvpn/ca_cl.crt", clientNum);
				eval("rm","-f","/etc/openvpn/client/ca.crt");
				eval("rm","-f","/jffs/openvpn/ca_cl.crt");
				fp = fopen(&buffer[0], "w");
				chmod(&buffer[0], S_IRUSR|S_IWUSR);
				sprintf(&buffer[0], "vpn_crt_client_ca", clientNum);
				fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
				fclose(fp);
				symlink("/jffs/openvpn/ca_cl.crt", "/etc/openvpn/client/ca.crt");
				nvram_unset("vpn_crt_client_ca");
				} //new config sets by webui
			else 
				{
				if ( check_if_file_exist("/jffs/openvpn/ca_cl.crt") ) {
				symlink("/jffs/openvpn/ca_cl.crt", "/etc/openvpn/client/ca.crt");
				nvram_unset("vpn_crt_client_ca"); }
				}
							} // jffs exist

		if (!useronly)
		{
			
			if ( !check_if_dir_exist("/jffs/openvpn/") ) 
							{ // jffs not exist
		sprintf(&buffer[0], "vpn_crt_client_key", clientNum);
		if ( !nvram_is_empty(&buffer[0]) )
		 {
			sprintf(&buffer[0], "/etc/openvpn/client/client.key", clientNum);
			fp = fopen(&buffer[0], "w");
			chmod(&buffer[0], S_IRUSR|S_IWUSR);
			sprintf(&buffer[0], "vpn_crt_client_key", clientNum);
			fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
			fclose(fp);
		 }
							} // jffs not exist
			else 
							{ // jffs exist
		sprintf(&buffer[0], "vpn_crt_client_key", clientNum);
				if ( !nvram_is_empty(&buffer[0]) && !check_if_file_exist("/jffs/openvpn/client_cl.key") ) 
					{ //create file from nvram
					sprintf(&buffer[0], "/jffs/openvpn/client_cl.key", clientNum);
					fp = fopen(&buffer[0], "w");
					chmod(&buffer[0], S_IRUSR|S_IWUSR);
					sprintf(&buffer[0], "vpn_crt_client_key", clientNum);
					fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
					fclose(fp);
					symlink("/jffs/openvpn/client_cl.key", "/etc/openvpn/client/client.key");
					nvram_unset("vpn_crt_client_key");
					} //create file from nvram
				else if ( !nvram_is_empty(&buffer[0]) && check_if_file_exist("/jffs/openvpn/client.key") ) 
					{ //new config sets by webui
					sprintf(&buffer[0], "/jffs/openvpn/client_cl.key", clientNum);
					eval("rm","-f","/etc/openvpn/client/client_cl.key");
					eval("rm","-f","/jffs/openvpn/client_cl.key");
					fp = fopen(&buffer[0], "w");
					chmod(&buffer[0], S_IRUSR|S_IWUSR);
					sprintf(&buffer[0], "vpn_crt_client_key", clientNum);
					fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
					fclose(fp);
					symlink("/jffs/openvpn/client_cl.key", "/etc/openvpn/client/client.key");
					nvram_unset("vpn_crt_client_key");
					} //new config sets by webui
				else 
					{
					if ( check_if_file_exist("/jffs/openvpn/client.key") ) {
					symlink("/jffs/openvpn/client_cl.key", "/etc/openvpn/client/client.key");
					nvram_unset("vpn_crt_client_key"); }
					}
							} // jffs exist


			if ( !check_if_dir_exist("/jffs/openvpn/") ) 
							{ // jffs not exist
		sprintf(&buffer[0], "vpn_crt_client_crt", clientNum);
		if ( !nvram_is_empty(&buffer[0]) )
		 {
			sprintf(&buffer[0], "/etc/openvpn/client/client.crt", clientNum);
			fp = fopen(&buffer[0], "w");
			chmod(&buffer[0], S_IRUSR|S_IWUSR);
			sprintf(&buffer[0], "vpn_crt_client_crt", clientNum);
			fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
			fclose(fp);
		 }
							} // jffs not exist
			else 
						{ // jffs exist 
		sprintf(&buffer[0], "vpn_crt_client_crt", clientNum);
				if ( !nvram_is_empty(&buffer[0]) && !check_if_file_exist("/jffs/openvpn/client_cl.crt") ) 
					{ //create file from nvram
					sprintf(&buffer[0], "/jffs/openvpn/client_cl.crt", clientNum);
					fp = fopen(&buffer[0], "w");
					chmod(&buffer[0], S_IRUSR|S_IWUSR);
					sprintf(&buffer[0], "vpn_crt_client_crt", clientNum);
					fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
					fclose(fp);
					symlink("/jffs/openvpn/client_cl.crt", "/etc/openvpn/client/client.crt");
					nvram_unset("vpn_crt_client_crt");
					} //create file from nvram
				else if ( !nvram_is_empty(&buffer[0]) && check_if_file_exist("/jffs/openvpn/client_cl.crt") ) 
					{ //new config sets by webui
					sprintf(&buffer[0], "/jffs/openvpn/client_cl.crt", clientNum);
					eval("rm","-f","/etc/openvpn/client/client_cl.crt");
					eval("rm","-f","/jffs/openvpn/client_cl.crt");
					fp = fopen(&buffer[0], "w");
					chmod(&buffer[0], S_IRUSR|S_IWUSR);
					sprintf(&buffer[0], "vpn_crt_client_crt", clientNum);
					fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
					fclose(fp);
					symlink("/jffs/openvpn/client_cl.crt", "/etc/openvpn/client/client.crt");
					nvram_unset("vpn_crt_client_crt");
					} //new config sets by webui
				else 
					{
					if ( check_if_file_exist("/jffs/openvpn/client.crt") ) {
					symlink("/jffs/openvpn/client_cl.crt", "/etc/openvpn/client/client.crt");
					nvram_unset("vpn_crt_client_crt"); }
					}
						} // jffs exist

		}
		if (userauth)
		{
			sprintf(&buffer[0], "/etc/openvpn/client/up", clientNum);
			fp = fopen(&buffer[0], "w");
			chmod(&buffer[0], S_IRUSR|S_IWUSR);
			sprintf(&buffer[0], "vpn_client_username", clientNum);
			fprintf(fp, "%s\n", nvram_safe_get(&buffer[0]));
			sprintf(&buffer[0], "vpn_client_password", clientNum);
			fprintf(fp, "%s\n", nvram_safe_get(&buffer[0]));
			fclose(fp);
		}


			if ( !check_if_dir_exist("/jffs/openvpn/") ) 
							{ // jffs not exist 
		sprintf(&buffer[0], "vpn_crt_client_crl", clientNum);
		if ( !nvram_is_empty(&buffer[0]) )
		 {
			sprintf(&buffer[0], "/etc/openvpn/client/crl.pem", clientNum);
			fp = fopen(&buffer[0], "w");
			chmod(&buffer[0], S_IRUSR|S_IWUSR);
			sprintf(&buffer[0], "vpn_crt_client_crl", clientNum);
			fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
			fclose(fp);
		 }
							} // jffs not exist
			else 
						{ // jffs exist 
		sprintf(&buffer[0], "vpn_crt_client_crl", clientNum);
				if ( !nvram_is_empty(&buffer[0]) && !check_if_file_exist("/jffs/openvpn/crl_cl.pem") ) 
					{ //create file from nvram
					sprintf(&buffer[0], "/jffs/openvpn/crl_cl.pem", clientNum);
					fp = fopen(&buffer[0], "w");
					chmod(&buffer[0], S_IRUSR|S_IWUSR);
					sprintf(&buffer[0], "vpn_crt_client_crl", clientNum);
					fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
					fclose(fp);
					symlink("/jffs/openvpn/crl_cl.pem", "/etc/openvpn/client/crl.pem");
					nvram_unset("vpn_crt_client_crl");
					} //create file from nvram
				else if ( !nvram_is_empty(&buffer[0]) && check_if_file_exist("/jffs/openvpn/crl_cl.pem") ) 
					{ //new config sets by webui
					sprintf(&buffer[0], "/jffs/openvpn/crl_cl.pem", clientNum);
					eval("rm","-f","/etc/openvpn/client/crl_cl.pem");
					eval("rm","-f","/jffs/openvpn/crl_cl.pem");
					fp = fopen(&buffer[0], "w");
					chmod(&buffer[0], S_IRUSR|S_IWUSR);
					sprintf(&buffer[0], "vpn_crt_client_crl", clientNum);
					fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
					fclose(fp);
					symlink("/jffs/openvpn/crl_cl.pem", "/etc/openvpn/client/crl.pem");
					nvram_unset("vpn_crt_client_crl");
					} //new config sets by webui
				else 
					{
					if ( check_if_file_exist("/jffs/openvpn/crl_cl.pem") ) {
					symlink("/jffs/openvpn/crl_cl.pem", "/etc/openvpn/client/crl.pem");
					nvram_unset("vpn_crt_client_crl"); }
					}
							}  // jffs exist
	}
	sprintf(&buffer[0], "vpn_client_hmac", clientNum);
	if ( cryptMode == SECRET || (cryptMode == TLS && nvram_get_int(&buffer[0]) >= 0) )
	{

			if ( !check_if_dir_exist("/jffs/openvpn/") )
								{ // jffs not exist
		sprintf(&buffer[0], "vpn_crt_client_static", clientNum);
		if ( !nvram_is_empty(&buffer[0]) )
		{
			sprintf(&buffer[0], "/etc/openvpn/client/static.key", clientNum);
			fp = fopen(&buffer[0], "w");
			chmod(&buffer[0], S_IRUSR|S_IWUSR);
			sprintf(&buffer[0], "vpn_crt_client_static", clientNum);
			fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
			fclose(fp);
		}

								}  // jffs not exist
			else 
							{ // jffs exist 
			sprintf(&buffer[0], "/etc/openvpn/client/static.key", clientNum);
				if ( !nvram_is_empty(&buffer[0]) && !check_if_file_exist("/jffs/openvpn/static_cl.key") )
					{ //create file from nvram
					sprintf(&buffer[0], "/jffs/openvpn/static_cl.key", clientNum);
					fp = fopen(&buffer[0], "w");
					chmod(&buffer[0], S_IRUSR|S_IWUSR);
					sprintf(&buffer[0], "vpn_crt_client_static", clientNum);
					fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
					fclose(fp);
					symlink("/jffs/openvpn/static_cl.key", "/etc/openvpn/client/static_cl.key");
					nvram_unset("vpn_crt_client_static");
					} //create file from nvram
				else if ( !nvram_is_empty(&buffer[0]) && check_if_file_exist("/jffs/openvpn/static_cl.key") )
					{ //new config sets by webui
					sprintf(&buffer[0], "/jffs/openvpn/static_cl.key", clientNum);
					eval("rm","-f","/etc/openvpn/client/static_cl.key");
					eval("rm","-f","/jffs/openvpn/static_cl.key");
					fp = fopen(&buffer[0], "w");
					chmod(&buffer[0], S_IRUSR|S_IWUSR);
					sprintf(&buffer[0], "vpn_crt_client_static", clientNum);
					fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
					fclose(fp);
					symlink("/jffs/openvpn/static_cl.key", "/etc/openvpn/client/static.key");
					nvram_unset("vpn_crt_client_static");
					} //new config sets by webui
				else 
					{
					if ( check_if_file_exist("/jffs/openvpn/static_cl.key") ) {
					symlink("/jffs/openvpn/static_cl.key", "/etc/openvpn/client/static.key");
					nvram_unset("vpn_crt_client_static"); }
					}
							} // jffs exist

	}
	vpnlog(VPN_LOG_EXTRA,"Done writing certs/keys");

	// Run postconf custom script on it if it exists
	sprintf(&buffer[0], "openvpnclient.postconf", clientNum);
	sprintf(&buffer2[0], "/etc/openvpn/client/config.ovpn", clientNum);
	run_postconf(&buffer[0], &buffer2[0]);

	// Start the VPN client
	sprintf(&buffer[0], "/etc/openvpn/vpnclient", clientNum);
	sprintf(&buffer2[0], "/etc/openvpn/client", clientNum);
	taskset_ret = cpu_eval(NULL, (clientNum == 1 ? CPU1 : CPU0), &buffer[0], "--cd", &buffer2[0], "--config", "config.ovpn");

	vpnlog(VPN_LOG_INFO,"Starting OpenVPN client 1", clientNum);

	if (taskset_ret)
	{
		vpnlog(VPN_LOG_ERROR,"Starting OpenVPN failed...");
		stop_vpnclient(clientNum);
		return;
	}
	vpnlog(VPN_LOG_EXTRA,"Done starting openvpn");

	// Handle firewall rules if appropriate
	sprintf(&buffer[0], "vpn_client_firewall", clientNum);
	if ( !nvram_contains_word(&buffer[0], "custom") )
	{
		// Create firewall rules
		vpnlog(VPN_LOG_EXTRA,"Creating firewall rules");
		mkdir("/etc/openvpn/fw", 0700);
		sprintf(&buffer[0], "/etc/openvpn/fw/client-fw.sh", clientNum);
		fp = fopen(&buffer[0], "w");
		chmod(&buffer[0], S_IRUSR|S_IWUSR|S_IXUSR);
		fprintf(fp, "#!/bin/sh\n");
		fprintf(fp, "iptables -I INPUT -i %s -j ACCEPT\n", &iface[0]);
		fprintf(fp, "iptables -I FORWARD %d -i %s -j ACCEPT\n", (nvram_match("cstats_enable", "1") ? 4 : 2), &iface[0]);
		// Setup traffic accounting
		if (nvram_match("cstats_enable", "1")) {
			ipt_account(fp, &iface[0]);
		}
		if ( routeMode == NAT )
		{
			sscanf(nvram_safe_get("lan_ipaddr"), "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
			sscanf(nvram_safe_get("lan_netmask"), "%d.%d.%d.%d", &nm[0], &nm[1], &nm[2], &nm[3]);
			fprintf(fp, "iptables -t nat -I POSTROUTING -s %d.%d.%d.%d/%s -o %s -j MASQUERADE\n",
			        ip[0]&nm[0], ip[1]&nm[1], ip[2]&nm[2], ip[3]&nm[3], nvram_safe_get("lan_netmask"), &iface[0]);
		}
		fclose(fp);
		vpnlog(VPN_LOG_EXTRA,"Done creating firewall rules");

		// Run the firewall rules
		vpnlog(VPN_LOG_EXTRA,"Running firewall rules");
		sprintf(&buffer[0], "/etc/openvpn/fw/client-fw.sh", clientNum);
		argv[0] = &buffer[0];
		argv[1] = NULL;
		_eval(argv, NULL, 0, NULL);
		vpnlog(VPN_LOG_EXTRA,"Done running firewall rules");
	}

	// Set up cron job
	sprintf(&buffer[0], "vpn_client_poll", clientNum);
	if ( (nvi = nvram_get_int(&buffer[0])) > 0 )
	{
		vpnlog(VPN_LOG_EXTRA,"Adding cron job");
		argv[0] = "cru";
		argv[1] = "a";
		sprintf(&buffer[0], "CheckVPNclient", clientNum);
		argv[2] = &buffer[0];
		sprintf(&buffer[strlen(&buffer[0])+1], "*/1 * * * * service start_vpnclient", nvi, clientNum);
		argv[3] = &buffer[strlen(&buffer[0])+1];
		argv[4] = NULL;
		_eval(argv, NULL, 0, NULL);
		vpnlog(VPN_LOG_EXTRA,"Done adding cron job");
	}

#ifdef LINUX26
//	sprintf(&buffer[0], "vpn_client", clientNum);
//	allow_fastnat(buffer, 0);
//	try_enabling_fastnat();
#endif

	vpnlog(VPN_LOG_INFO,"VPN GUI client backend complete.");
}

void stop_vpnclient(int clientNum)
{
	int argc;
	char *argv[7];
	char buffer[BUF_SIZE];

	sprintf(&buffer[0], "stop_vpnclient", clientNum);
	if (getpid() != 1) {
                notify_rc(&buffer[0]);
		return;
	}

	vpnlog(VPN_LOG_INFO,"Stopping VPN GUI client backend.");

	// Remove cron job
	vpnlog(VPN_LOG_EXTRA,"Removing cron job");
	argv[0] = "cru";
	argv[1] = "d";
	sprintf(&buffer[0], "CheckVPNclient", clientNum);
	argv[2] = &buffer[0];
	argv[3] = NULL;
	_eval(argv, NULL, 0, NULL);
	vpnlog(VPN_LOG_EXTRA,"Done removing cron job");

	// Remove firewall rules
	vpnlog(VPN_LOG_EXTRA,"Removing firewall rules.");
	sprintf(&buffer[0], "/etc/openvpn/fw/client-fw.sh", clientNum);
	argv[0] = "sed";
	argv[1] = "-i";
	argv[2] = "s/-A/-D/g;s/-I/-D/g;s/FORWARD\\ [0-9]\\ /FORWARD\\ /g";
	argv[3] = &buffer[0];
	argv[4] = NULL;
	if (!_eval(argv, NULL, 0, NULL))
	{
		argv[0] = &buffer[0];
		argv[1] = NULL;
		_eval(argv, NULL, 0, NULL);
	}
	vpnlog(VPN_LOG_EXTRA,"Done removing firewall rules.");

	// Stop the VPN client
	vpnlog(VPN_LOG_EXTRA,"Stopping OpenVPN client.");
	sprintf(&buffer[0], "vpnclient", clientNum);
	if ( !ovpn_waitfor(&buffer[0]) )
		vpnlog(VPN_LOG_EXTRA,"OpenVPN client stopped.");

	// NVRAM setting for device type could have changed, just try to remove both
	vpnlog(VPN_LOG_EXTRA,"Removing VPN device.");
	sprintf(&buffer[0], "openvpn --rmtun --dev tap11", clientNum+CLIENT_IF_START);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	_eval(argv, NULL, 0, NULL);

	sprintf(&buffer[0], "openvpn --rmtun --dev tun11", clientNum+CLIENT_IF_START);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	_eval(argv, NULL, 0, NULL);
	vpnlog(VPN_LOG_EXTRA,"VPN device removed.");

	modprobe_r("tun");

	if ( nvram_get_int("vpn_debug") <= VPN_LOG_EXTRA )
	{
		vpnlog(VPN_LOG_EXTRA,"Removing generated files.");
		// Delete all files for this client
		sprintf(&buffer[0], "rm -rf /etc/openvpn/client /etc/openvpn/fw/client-fw.sh /etc/openvpn/vpnclient",clientNum,clientNum,clientNum);
		for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
		_eval(argv, NULL, 0, NULL);

		// Attempt to remove directories.  Will fail if not empty
		rmdir("/etc/openvpn/fw");
		rmdir("/etc/openvpn");
		vpnlog(VPN_LOG_EXTRA,"Done removing generated files.");
	}

#ifdef LINUX26
//	sprintf(&buffer[0], "vpn_client", clientNum);
//	allow_fastnat(buffer, 1);
//	try_enabling_fastnat();
#endif

	sprintf(&buffer[0], "vpn_client_state", clientNum);
	nvram_set(&buffer[0], "0");
	sprintf(&buffer[0], "vpn_client_errno", clientNum);
	nvram_set(&buffer[0], "0");

	vpnlog(VPN_LOG_INFO,"VPN GUI client backend stopped.");
}

void start_vpnserver(int serverNum)
{
	FILE *fp, *ccd, *fp_client;;
	char iface[IF_SIZE];
	char buffer[BUF_SIZE];
	char buffer2[4000];
	char *argv[6], *chp, *route;
	int argc = 0;
	int c2c = 0;
	enum { TAP, TUN } ifType = TUN;
	enum { TLS, SECRET, CUSTOM } cryptMode = CUSTOM;
	int nvi, ip[4], nm[4];
	long int nvl;
	int pid;
	int taskset_ret;
	char nv1[32], nv2[32], nv3[32], fpath[128];
	int valid = 0;
	int userauth = 0, useronly = 0;

	sprintf(&buffer[0], "start_vpnserver", serverNum);
	if (getpid() != 1) {
		notify_rc(&buffer[0]);
		return;
	}

	vpnlog(VPN_LOG_INFO,"VPN GUI server backend starting...");

	if ( (pid = pidof(&buffer[6])) >= 0 )
	{
		vpnlog(VPN_LOG_NOTE, "VPN Server 1 already running...", serverNum);
		vpnlog(VPN_LOG_INFO,"PID: %d", pid);
		return;
	}

	sprintf(&buffer[0], "vpn_server_state", serverNum);
	nvram_set(&buffer[0], "1");	//initializing
	sprintf(&buffer[0], "vpn_server_errno", serverNum);
	nvram_set(&buffer[0], "0");

	// Determine interface type
	sprintf(&buffer[0], "vpn_server_if", serverNum);
	if ( nvram_contains_word(&buffer[0], "tap") )
		ifType = TAP;
	else if ( nvram_contains_word(&buffer[0], "tun") )
		ifType = TUN;
	else
	{
		vpnlog(VPN_LOG_ERROR,"Invalid interface type, %.3s", nvram_safe_get(&buffer[0]));
		return;
	}

	// Build interface name
	snprintf(&iface[0], IF_SIZE, "%s21", nvram_safe_get(&buffer[0]), serverNum+SERVER_IF_START);

	//
	if(is_intf_up(&iface[0]) && ifType == TAP) {
		eval("brctl", "delif", nvram_safe_get("lan_ifname"), &iface[0]);
	}

	// Determine encryption mode
	sprintf(&buffer[0], "vpn_server_crypt", serverNum);
	if ( nvram_contains_word(&buffer[0], "tls") )
		cryptMode = TLS;
	else if ( nvram_contains_word(&buffer[0], "secret") )
		cryptMode = SECRET;
	else if ( nvram_contains_word(&buffer[0], "custom") )
		cryptMode = CUSTOM;
	else
	{
		vpnlog(VPN_LOG_ERROR,"Invalid encryption mode, %.6s", nvram_safe_get(&buffer[0]));
		return;
	}

	// Make sure openvpn directory exists
	mkdir("/etc/openvpn", 0700);
	sprintf(&buffer[0], "/etc/openvpn/server", serverNum);
	mkdir(&buffer[0], 0700);

	// Make sure symbolic link exists
	sprintf(&buffer[0], "/etc/openvpn/vpnserver", serverNum);
	unlink(&buffer[0]);
	if ( symlink("/usr/sbin/openvpn", &buffer[0]) )
	{
		vpnlog(VPN_LOG_ERROR,"Creating symlink failed...");
		stop_vpnserver(serverNum);
		return;
	}

	// Make sure module is loaded
	modprobe("tun");
	f_wait_exists("/dev/net/tun", 5);

	// Create tap/tun interface
	sprintf(&buffer[0], "openvpn --mktun --dev %s", &iface[0]);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	if ( _eval(argv, NULL, 0, NULL) )
	{
		vpnlog(VPN_LOG_ERROR,"Creating tunnel interface %s failed...",&iface[0]);
		stop_vpnserver(serverNum);
		return;
	}


	// Add interface to LAN bridge (TAP only)
	if( ifType == TAP )
	{
		snprintf(&buffer[0], BUF_SIZE, "brctl addif %s %s", nvram_safe_get("lan_ifname"), &iface[0]);
		for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
		if ( _eval(argv, NULL, 0, NULL) )
		{
			vpnlog(VPN_LOG_ERROR,"Adding tunnel interface to bridge failed...");
			stop_vpnserver(serverNum);
			return;
		}
	}

	// Bring interface up
	sprintf(&buffer[0], "ifconfig %s 0.0.0.0 promisc up", &iface[0]);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	if ( _eval(argv, NULL, 0, NULL) )
	{
		vpnlog(VPN_LOG_ERROR,"Bringing up tunnel interface failed...");
		stop_vpnserver(serverNum);
		return;
	}

	// Build and write config files
	vpnlog(VPN_LOG_EXTRA,"Writing config file");
	sprintf(&buffer[0], "/etc/openvpn/server/config.ovpn", serverNum);
	fp = fopen(&buffer[0], "w");
	chmod(&buffer[0], S_IRUSR|S_IWUSR);
	fprintf(fp, "# Automatically generated configuration\n");
	fprintf(fp, "daemon\n");

	sprintf(&buffer[0], "/etc/openvpn/server/client.ovpn", serverNum);
	fp_client = fopen(&buffer[0], "w");
	fprintf(fp_client, "client\n");

	if ( cryptMode == TLS )
	{
		if ( ifType == TUN )
		{
			sprintf(&buffer[0], "vpn_server_sn", serverNum);
			fprintf(fp, "server %s ", nvram_safe_get(&buffer[0]));
			sprintf(&buffer[0], "vpn_server_nm", serverNum);
			fprintf(fp, "%s\n", nvram_safe_get(&buffer[0]));
			fprintf(fp_client, "dev tun\n");
		}
		else if ( ifType == TAP )
		{
			fprintf(fp, "server-bridge");
			sprintf(&buffer[0], "vpn_server_dhcp", serverNum);
			if ( nvram_get_int(&buffer[0]) == 0 )
			{
				fprintf(fp, " %s ", nvram_safe_get("lan_ipaddr"));
				fprintf(fp, "%s ", nvram_safe_get("lan_netmask"));
				sprintf(&buffer[0], "vpn_server_r1", serverNum);
				fprintf(fp, "%s ", nvram_safe_get(&buffer[0]));
				sprintf(&buffer[0], "vpn_server_r2", serverNum);
				fprintf(fp, "%s", nvram_safe_get(&buffer[0]));
			}
			else
			{
				fprintf(fp, "\npush \"route 0.0.0.0 255.255.255.255 net_gateway\"");
			}

			fprintf(fp, "\n");
			fprintf(fp_client, "dev tap\n");
			fprintf(fp_client, "# Windows needs the TAP-Win32 adapter name\n");
			fprintf(fp_client, "# from the Network Connections panel\n");
			fprintf(fp_client, "# if you have more than one.  On XP SP2,\n");
			fprintf(fp_client, "# you may need to disable the firewall\n");
			fprintf(fp_client, "# for the TAP adapter.\n");
			fprintf(fp_client, ";dev-node MyTap\n");
		}
	}
	else if ( cryptMode == SECRET )
	{
		fprintf(fp_client, "mode p2p\n");

		if ( ifType == TUN )
		{
			sprintf(&buffer[0], "vpn_server_local", serverNum);
			fprintf(fp, "ifconfig %s ", nvram_safe_get(&buffer[0]));
			sprintf(&buffer[0], "vpn_server_remote", serverNum);
			fprintf(fp, "%s\n", nvram_safe_get(&buffer[0]));

			fprintf(fp_client, "dev tun\n");
			sprintf(&buffer[0], "vpn_server_remote", serverNum);
			fprintf(fp_client, "ifconfig %s ", nvram_safe_get(&buffer[0]));
			sprintf(&buffer[0], "vpn_server_local", serverNum);
			fprintf(fp_client, "%s\n", nvram_safe_get(&buffer[0]));
		}
		else if ( ifType == TAP )
		{
			fprintf(fp_client, "dev tap\n");
		}
	}
	//protocol
	sprintf(&buffer[0], "vpn_server_proto", serverNum);
	fprintf(fp, "proto %s\n", nvram_safe_get(&buffer[0]));
	if(!strcmp(nvram_safe_get(&buffer[0]), "udp"))
		fprintf(fp_client, "proto %s\n", nvram_safe_get(&buffer[0]));
	else
		fprintf(fp_client, "proto tcp-client\n");

	//port
	sprintf(&buffer[0], "vpn_server_port", serverNum);
	fprintf(fp, "port %d\n", nvram_get_int(&buffer[0]));

	if(nvram_get_int("ddns_enable_x"))
		fprintf(fp_client, "remote %s %s\n", nvram_safe_get("ddns_hostname_x"), nvram_safe_get(&buffer[0]));
	else
		fprintf(fp_client, "remote %s %s\n", nvram_safe_get("wan0_ipaddr"), nvram_safe_get(&buffer[0]));

	fprintf(fp_client, "float\n");


	//cipher
	fprintf(fp, "dev %s\n", &iface[0]);
	sprintf(&buffer[0], "vpn_server_cipher", serverNum);
	if ( !nvram_contains_word(&buffer[0], "default") ) {
		fprintf(fp, "cipher %s\n", nvram_safe_get(&buffer[0]));
		fprintf(fp_client, "cipher %s\n", nvram_safe_get(&buffer[0]));
	}

	//compression
	sprintf(&buffer[0], "vpn_server_comp", serverNum);
	if ( nvram_get_int(&buffer[0]) >= 0 ) {
		fprintf(fp, "comp-lzo %s\n", nvram_safe_get(&buffer[0]));
		fprintf(fp_client, "comp-lzo %s\n", nvram_safe_get(&buffer[0]));
	}

	fprintf(fp, "keepalive 15 60\n");
	fprintf(fp_client, "keepalive 15 60\n");

	if ( (nvi = nvram_get_int("vpn_loglevel")) >= 0 )
		fprintf(fp, "verb %d\n", nvi);
	else
		fprintf(fp, "verb 3\n");

	if ( cryptMode == TLS )
	{
		//TLS Renegotiation Time
		sprintf(&buffer[0], "vpn_server_reneg", serverNum);
		if ( (nvl = atol(nvram_safe_get(&buffer[0]))) >= 0 ) {
			fprintf(fp, "reneg-sec %ld\n", nvl);
			fprintf(fp_client, "reneg-sec %ld\n", nvl);
		}

		sprintf(&buffer[0], "vpn_server_plan", serverNum);
		if ( ifType == TUN && nvram_get_int(&buffer[0]) )
		{
			sscanf(nvram_safe_get("lan_ipaddr"), "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
			sscanf(nvram_safe_get("lan_netmask"), "%d.%d.%d.%d", &nm[0], &nm[1], &nm[2], &nm[3]);
			fprintf(fp, "push \"route %d.%d.%d.%d %s\"\n", ip[0]&nm[0], ip[1]&nm[1], ip[2]&nm[2], ip[3]&nm[3],
			        nvram_safe_get("lan_netmask"));
		}

		sprintf(&buffer[0], "vpn_server_ccd", serverNum);
		if ( nvram_get_int(&buffer[0]) )
		{
			fprintf(fp, "client-config-dir ccd\n");

			sprintf(&buffer[0], "vpn_server_c2c", serverNum);
			if ( (c2c = nvram_get_int(&buffer[0])) )
				fprintf(fp, "client-to-client\n");

			sprintf(&buffer[0], "vpn_server_ccd_excl", serverNum);
			if ( nvram_get_int(&buffer[0]) )
				fprintf(fp, "ccd-exclusive\n");
			else
				fprintf(fp, "duplicate-cn\n");

			sprintf(&buffer[0], "/etc/openvpn/server/ccd", serverNum);
			mkdir(&buffer[0], 0700);
			chdir(&buffer[0]);

			sprintf(&buffer[0], "vpn_server_ccd_val", serverNum);
			strcpy(&buffer[0], nvram_safe_get(&buffer[0]));
			chp = strtok(&buffer[0],"<");
			while ( chp != NULL )
			{
				nvi = strlen(chp);

				chp[strcspn(chp,">")] = '\0';
				vpnlog(VPN_LOG_EXTRA,"CCD: enabled: %d", atoi(chp));
				if ( atoi(chp) == 1 )
				{
					nvi -= strlen(chp)+1;
					chp += strlen(chp)+1;

					ccd = NULL;
					route = NULL;
					if ( nvi > 0 )
					{
						chp[strcspn(chp,">")] = '\0';
						vpnlog(VPN_LOG_EXTRA,"CCD: Common name: %s", chp);
						ccd = fopen(chp, "w");
						chmod(chp, S_IRUSR|S_IWUSR);

						nvi -= strlen(chp)+1;
						chp += strlen(chp)+1;
					}
					if ( nvi > 0 && ccd != NULL && strcspn(chp,">") != strlen(chp) )
					{
						chp[strcspn(chp,">")] = ' ';
						chp[strcspn(chp,">")] = '\0';
						route = chp;
						vpnlog(VPN_LOG_EXTRA,"CCD: Route: %s", chp);
						if ( strlen(route) > 1 )
						{
							fprintf(ccd, "iroute %s\n", route);
							fprintf(fp, "route %s\n", route);
						}

						nvi -= strlen(chp)+1;
						chp += strlen(chp)+1;
					}
					if ( ccd != NULL )
						fclose(ccd);
					if ( nvi > 0 && route != NULL )
					{
						chp[strcspn(chp,">")] = '\0';
						vpnlog(VPN_LOG_EXTRA,"CCD: Push: %d", atoi(chp));
						if ( c2c && atoi(chp) == 1 && strlen(route) > 1 )
							fprintf(fp, "push \"route %s\"\n", route);

						nvi -= strlen(chp)+1;
						chp += strlen(chp)+1;
					}

					vpnlog(VPN_LOG_EXTRA,"CCD leftover: %d", nvi+1);
				}
				// Advance to next entry
				chp = strtok(NULL, "<");
			}
			// Copy any user-configured client config files
			sprintf(&buffer[0], "/jffs/configs/openvpn/ccd", serverNum);

			if(check_if_dir_exist(&buffer[0]))
			{
				vpnlog(VPN_LOG_EXTRA,"CCD - copying user files");
				sprintf(buffer2, "cp %s/* .", &buffer[0]); /* */
				system(buffer2);
			}

			vpnlog(VPN_LOG_EXTRA,"CCD processing complete");
		}
		else
			fprintf(fp, "duplicate-cn\n");

		sprintf(&buffer[0], "vpn_server_pdns", serverNum);
		if ( nvram_get_int(&buffer[0]) )
		{
			if ( nvram_safe_get("wan_domain")[0] != '\0' )
				fprintf(fp, "push \"dhcp-option DOMAIN %s\"\n", nvram_safe_get("wan_domain"));
			fprintf(fp, "push \"dhcp-option DNS %s\"\n", nvram_safe_get("lan_ipaddr"));
		}

		sprintf(&buffer[0], "vpn_server_rgw", serverNum);
		if ( nvram_get_int(&buffer[0]) )
		{
			if ( ifType == TAP )
				fprintf(fp, "push \"route-gateway %s\"\n", nvram_safe_get("lan_ipaddr"));
			fprintf(fp, "push \"redirect-gateway def1\"\n");
		}

		sprintf(&buffer[0], "vpn_server_hmac", serverNum);
		nvi = nvram_get_int(&buffer[0]);
		//sprintf(&buffer[0], "vpn_crt_server_static", serverNum);
		//if ( !nvram_is_empty(&buffer[0]) && nvi >= 0 )
		if ( nvi >= 0 )
		{
			fprintf(fp, "tls-auth static.key");
			if ( nvi < 2 )
				fprintf(fp, " %d", nvi);
			fprintf(fp, "\n");
		}

		// Enable username/pass authentication (for Asus's PAM support)
		sprintf(&buffer[0], "vpn_server_userpass_auth", serverNum);
		userauth = nvram_get_int(&buffer[0]);
		sprintf(&buffer[0], "vpn_server_igncrt", serverNum);
		useronly = userauth && nvram_get_int(&buffer[0]);

		if ( userauth ) {
			//authentication
			fprintf(fp, "plugin /usr/lib/openvpn-plugin-auth-pam.so openvpn\n");
			fprintf(fp_client, "auth-user-pass\n");

			//ignore client certificate, but only if user/pass auth is enabled
			//That way, existing configuration (pre-Asus OVPN)
			//will remain unchanged.
			if ( useronly ) {
				fprintf(fp, "client-cert-not-required\n");
				fprintf(fp, "username-as-common-name\n");
			}
			fprintf(fp, "duplicate-cn\n");
		}

		fprintf(fp_client, "ns-cert-type server\n");
		//sprintf(&buffer[0], "vpn_crt_server_ca", serverNum);
		//if ( !nvram_is_empty(&buffer[0]) )
			fprintf(fp, "ca ca.crt\n");
		//sprintf(&buffer[0], "vpn_crt_server_dh", serverNum);
		//if ( !nvram_is_empty(&buffer[0]) )
			fprintf(fp, "dh dh.pem\n");
		//sprintf(&buffer[0], "vpn_crt_server_crt", serverNum);
		//if ( !nvram_is_empty(&buffer[0]) )
			fprintf(fp, "cert server.crt\n");
		//sprintf(&buffer[0], "vpn_crt_server_key", serverNum);
		//if ( !nvram_is_empty(&buffer[0]) )
			fprintf(fp, "key server.key\n");
		sprintf(&buffer[0], "vpn_crt_server_crl", serverNum);
		if ( !nvram_is_empty(&buffer[0]) || check_if_file_exist("/jffs/openvpn/crl.pem") )
			fprintf(fp, "crl-verify crl.pem\n");
	}
	else if ( cryptMode == SECRET )
	{
		//sprintf(&buffer[0], "vpn_crt_server_static", serverNum);
		//if ( !nvram_is_empty(&buffer[0]) )
			fprintf(fp, "secret static.key\n");
	}

	if (check_if_file_exist("/jffs/scripts/openvpn-event"))
	{
		sprintf(&buffer[0], "/etc/openvpn/server/updown.sh", serverNum);
		symlink("/jffs/scripts/openvpn-event", &buffer[0]);
		fprintf(fp, "script-security 2\n");
		fprintf(fp, "up updown.sh\n");
		fprintf(fp, "down updown.sh\n");
	}

	fprintf(fp, "status-version 2\n");
	fprintf(fp, "status status\n");
	fprintf(fp, "\n# Custom Configuration\n");
	sprintf(&buffer[0], "vpn_server_custom", serverNum);
	fprintf(fp, "%s", nvram_safe_get(&buffer[0]));
	fclose(fp);

	vpnlog(VPN_LOG_EXTRA,"Done writing server config file");

	// Write certification and key files
	vpnlog(VPN_LOG_EXTRA,"Writing certs/keys");
	if ( cryptMode == TLS )
	{
		//generate certification and key
		sprintf(nv1, "vpn_crt_server_ca", serverNum);
		sprintf(nv2, "vpn_crt_server_key", serverNum);
		sprintf(nv3, "vpn_crt_server_crt", serverNum);

// prepare for openvpn server for jffs

		if ( !check_if_dir_exist("/jffs/openvpn/") ) 
								{ // jffs not exist
		if( nvram_is_empty(nv1) || nvram_is_empty(nv2) || nvram_is_empty(nv3) ) {
			sprintf(fpath, "/tmp/genvpncert.sh");
			fp = fopen(fpath, "w");
			if(fp) {
				fprintf(fp, "#!/bin/sh\n");
				//fprintf(fp, ". /rom/easy-rsa/vars\n");
				fprintf(fp, "export OPENSSL=\"openssl\"\n");
				fprintf(fp, "export GREP=\"grep\"\n");
				fprintf(fp, "export KEY_CONFIG=\"/rom/easy-rsa/openssl-1.0.0.cnf\"\n");
				fprintf(fp, "export KEY_DIR=\"/etc/openvpn/server\"\n", serverNum);
				fprintf(fp, "export KEY_SIZE=1024\n");
				fprintf(fp, "export CA_EXPIRE=3650\n");
				fprintf(fp, "export KEY_EXPIRE=3650\n");
				fprintf(fp, "export KEY_COUNTRY=\"TW\"\n");
				fprintf(fp, "export KEY_PROVINCE=\"TW\"\n");
				fprintf(fp, "export KEY_CITY=\"Taipei\"\n");
				fprintf(fp, "export KEY_ORG=\"ASUS\"\n");
				fprintf(fp, "export KEY_EMAIL=\"me@myhost.mydomain\"\n");
				fprintf(fp, "export KEY_CN=\"%s\"\n", nvram_safe_get("productid"));
				fprintf(fp, "touch /etc/openvpn/server/index.txt\n", serverNum);
				fprintf(fp, "echo 01 >/etc/openvpn/server/serial\n", serverNum);
				fprintf(fp, "/rom/easy-rsa/pkitool --initca\n");
				fprintf(fp, "/rom/easy-rsa/pkitool --server server\n");

				//undefined common name, default use username-as-common-name
				fprintf(fp, "export KEY_CN=\"\"\n");
				fprintf(fp, "/rom/easy-rsa/pkitool client\n");

				fclose(fp);
				chmod(fpath, 0700);
				eval(fpath);
				unlink(fpath);
			}

			//set certification and key to nvram
			sprintf(fpath, "/etc/openvpn/server/ca.key", serverNum);
			fp = fopen(fpath, "r");
			if(fp) {
				sprintf(&buffer[0], "vpn_crt_server_ca_key", serverNum);
				set_crt_parsed(&buffer[0], fpath);
				fclose(fp);
			}
			sprintf(fpath, "/etc/openvpn/server/ca.crt", serverNum);
			fp = fopen(fpath, "r");
			if(fp) {
				set_crt_parsed(nv1, fpath);
				fclose(fp);
			}
			sprintf(fpath, "/etc/openvpn/server/server.key", serverNum);
			fp = fopen(fpath, "r");
			if(fp) {
				set_crt_parsed(nv2, fpath);
				fclose(fp);
			}
			sprintf(fpath, "/etc/openvpn/server/server.crt", serverNum);
			fp = fopen(fpath, "r");
			if(fp) {
				set_crt_parsed(nv3, fpath);
				fclose(fp);
			}
			sprintf(fpath, "/etc/openvpn/server/client.key", serverNum);
			fp = fopen(fpath, "r");
			if(fp) {
				sprintf(&buffer[0], "vpn_crt_server_client_key", serverNum);
				set_crt_parsed(&buffer[0], fpath);
				fclose(fp);
			}
			sprintf(fpath, "/etc/openvpn/server/client.crt", serverNum);
			fp = fopen(fpath, "r");
			if(fp) {
				sprintf(&buffer[0], "vpn_crt_server_client_crt", serverNum);
				set_crt_parsed(&buffer[0], fpath);
				fclose(fp);
			}
		}
		else {
				sprintf(&buffer[0], "/etc/openvpn/server/ca.key", serverNum);
				fp = fopen(&buffer[0], "w");
				chmod(&buffer[0], S_IRUSR|S_IWUSR);
				sprintf(&buffer[0], "vpn_crt_server_ca_key", serverNum);
				fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
				fclose(fp);
				sprintf(&buffer[0], "/etc/openvpn/server/ca.crt", serverNum);
				fp = fopen(&buffer[0], "w");
				chmod(&buffer[0], S_IRUSR|S_IWUSR);
				sprintf(&buffer[0], "vpn_crt_server_ca", serverNum);
				fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
				fclose(fp);
				sprintf(&buffer[0], "/etc/openvpn/server/server.key", serverNum);
				fp = fopen(&buffer[0], "w");
				chmod(&buffer[0], S_IRUSR|S_IWUSR);
				sprintf(&buffer[0], "vpn_crt_server_key", serverNum);
				fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
				fclose(fp);
				sprintf(&buffer[0], "/etc/openvpn/server/server.crt", serverNum);
				fp = fopen(&buffer[0], "w");
				chmod(&buffer[0], S_IRUSR|S_IWUSR);
				sprintf(&buffer[0], "vpn_crt_server_crt", serverNum);
				fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
				fclose(fp);

			/* sprintf(&buffer[0], "vpn_crt_server_crl", serverNum);
			if ( !nvram_is_empty(&buffer[0]) )
			{
				sprintf(&buffer[0], "/etc/openvpn/server/crl.pem", serverNum);
				fp = fopen(&buffer[0], "w");
				chmod(&buffer[0], S_IRUSR|S_IWUSR);
				sprintf(&buffer[0], "vpn_crt_server_crl", serverNum);
				fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
				fclose(fp);
			} */
		}
								} // end jffs not exist
		else 
								{ // jffs exist   
			if ( !check_if_file_exist("/jffs/openvpn/ca.crt") || !check_if_file_exist("/jffs/openvpn/server.key") || !check_if_file_exist("/jffs/openvpn/server.crt") ) { // if files not exist
				if( nvram_is_empty(nv1) || nvram_is_empty(nv2) || nvram_is_empty(nv3) )
							{ // and empty nvram
			sprintf(fpath, "/tmp/genvpncert.sh");
			fp = fopen(fpath, "w");
			if(fp) {
				fprintf(fp, "#!/bin/sh\n");
				//fprintf(fp, ". /rom/easy-rsa/vars\n");
				fprintf(fp, "export OPENSSL=\"openssl\"\n");
				fprintf(fp, "export GREP=\"grep\"\n");
				fprintf(fp, "export KEY_CONFIG=\"/rom/easy-rsa/openssl-1.0.0.cnf\"\n");
				fprintf(fp, "export KEY_DIR=\"/etc/openvpn/server\"\n", serverNum);
				fprintf(fp, "export KEY_SIZE=1024\n");
				fprintf(fp, "export CA_EXPIRE=3650\n");
				fprintf(fp, "export KEY_EXPIRE=3650\n");
				fprintf(fp, "export KEY_COUNTRY=\"TW\"\n");
				fprintf(fp, "export KEY_PROVINCE=\"TW\"\n");
				fprintf(fp, "export KEY_CITY=\"Taipei\"\n");
				fprintf(fp, "export KEY_ORG=\"ASUS\"\n");
				fprintf(fp, "export KEY_EMAIL=\"me@myhost.mydomain\"\n");
				fprintf(fp, "export KEY_CN=\"%s\"\n", nvram_safe_get("productid"));
				fprintf(fp, "touch /etc/openvpn/server/index.txt\n", serverNum);
				fprintf(fp, "echo 01 >/etc/openvpn/server/serial\n", serverNum);
				fprintf(fp, "/rom/easy-rsa/pkitool --initca\n");
				fprintf(fp, "/rom/easy-rsa/pkitool --server server\n");

				//undefined common name, default use username-as-common-name
				fprintf(fp, "export KEY_CN=\"\"\n");
				fprintf(fp, "/rom/easy-rsa/pkitool client\n");

				fclose(fp);
				chmod(fpath, 0700);
				eval(fpath);
				unlink(fpath);
			}

			//set certification and key to jffs 
			eval("mv", "-f", "/etc/openvpn/server/ca.key", "/jffs/openvpn/");
			nvram_unset("vpn_crt_server_ca_key");
			symlink("/jffs/openvpn/ca.key", "/etc/openvpn/server/ca.key");
			eval("mv", "-f", "/etc/openvpn/server/ca.crt", "/jffs/openvpn/");
			nvram_unset("vpn_crt_server_ca");
			symlink("/jffs/openvpn/ca.crt", "/etc/openvpn/server/ca.crt");
			eval("mv", "-f", "/etc/openvpn/server/server.key", "/jffs/openvpn/");
			nvram_unset("vpn_crt_server_key");
			symlink("/jffs/openvpn/server.key", "/etc/openvpn/server/server.key");
			eval("mv", "-f", "/etc/openvpn/server/server.crt", "/jffs/openvpn/");
			nvram_unset("vpn_crt_server_crt");
			symlink("/jffs/openvpn/server.crt", "/etc/openvpn/server/server.crt");
			eval("mv", "-f", "/etc/openvpn/server/client.key", "/jffs/openvpn/");
			nvram_unset("vpn_crt_server_client_key");
			symlink("/jffs/openvpn/client.key", "/etc/openvpn/server/client.key");
			eval("mv", "-f", "/etc/openvpn/server/client.crt", "/jffs/openvpn/");
			nvram_unset("vpn_crt_server_client_crt");
			symlink("/jffs/openvpn/client.crt", "/etc/openvpn/server/client.crt");
			/* sprintf(&buffer[0], "vpn_crt_server_crl", serverNum);
			if ( !nvram_is_empty(&buffer[0]) )
			{
				sprintf(&buffer[0], "/etc/openvpn/server/crl.pem", serverNum);
				fp = fopen(&buffer[0], "w");
				chmod(&buffer[0], S_IRUSR|S_IWUSR);
				sprintf(&buffer[0], "vpn_crt_server_crl", serverNum);
				fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
				fclose(fp);
				eval("mv", "-f", "/etc/openvpn/server/crl.pem", "/jffs/openvpn/");
				nvram_unset("vpn_crt_server_crl");
				symlink("/jffs/openvpn/crl.pem", "/etc/openvpn/server/crl.pem");
			} */
							} // and empty nvram
				else 				{ // nvram sets by webui
			sprintf(&buffer[0], "/etc/openvpn/server/ca.key", serverNum);
			if ( !nvram_is_empty(&buffer[0]) )
			{
				sprintf(&buffer[0], "/etc/openvpn/server/ca.key", serverNum);
				fp = fopen(&buffer[0], "w");
				chmod(&buffer[0], S_IRUSR|S_IWUSR);
				sprintf(&buffer[0], "vpn_crt_server_ca_key", serverNum);
				fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
				fclose(fp);
				eval("mv", "-f", "/etc/openvpn/server/ca.key", "/jffs/openvpn/");
				nvram_unset("vpn_crt_server_ca_key");
				symlink("/jffs/openvpn/ca.key", "/etc/openvpn/server/ca.key");
			}

			sprintf(&buffer[0], "vpn_crt_server_ca", serverNum);
			if ( !nvram_is_empty(&buffer[0]) )
			{
				sprintf(&buffer[0], "/etc/openvpn/server/ca.crt", serverNum);
				fp = fopen(&buffer[0], "w");
				chmod(&buffer[0], S_IRUSR|S_IWUSR);
				sprintf(&buffer[0], "vpn_crt_server_ca", serverNum);
				fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
				fclose(fp);
				eval("mv", "-f", "/etc/openvpn/server/ca.crt", "/jffs/openvpn/");
				nvram_unset("vpn_crt_server_ca");
				symlink("/jffs/openvpn/ca.crt", "/etc/openvpn/server/ca.crt");
			}

			sprintf(&buffer[0], "vpn_crt_server_key", serverNum);
			if ( !nvram_is_empty(&buffer[0]) )
			{
				sprintf(&buffer[0], "/etc/openvpn/server/server.key", serverNum);
				fp = fopen(&buffer[0], "w");
				chmod(&buffer[0], S_IRUSR|S_IWUSR);
				sprintf(&buffer[0], "vpn_crt_server_key", serverNum);
				fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
				fclose(fp);
				eval("mv", "-f", "/etc/openvpn/server/server.key", "/jffs/openvpn/");
				nvram_unset("vpn_crt_server_key");
				symlink("/jffs/openvpn/server.key", "/etc/openvpn/server/server.key");
			}

			sprintf(&buffer[0], "vpn_crt_server_crt", serverNum);
			if ( !nvram_is_empty(&buffer[0]) )
			{
				sprintf(&buffer[0], "/etc/openvpn/server/server.crt", serverNum);
				fp = fopen(&buffer[0], "w");
				chmod(&buffer[0], S_IRUSR|S_IWUSR);
				sprintf(&buffer[0], "vpn_crt_server_crt", serverNum);
				fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
				fclose(fp);
				eval("mv", "-f", "/etc/openvpn/server/server.crt", "/jffs/openvpn/");
				nvram_unset("vpn_crt_server_crt");
				symlink("/jffs/openvpn/server.crt", "/etc/openvpn/server/server.crt");
			}

			/* sprintf(&buffer[0], "vpn_crt_server_crl", serverNum);
			if ( !nvram_is_empty(&buffer[0]) )
			{
				sprintf(&buffer[0], "/etc/openvpn/server/crl.pem", serverNum);
				fp = fopen(&buffer[0], "w");
				chmod(&buffer[0], S_IRUSR|S_IWUSR);
				sprintf(&buffer[0], "vpn_crt_server_crl", serverNum);
				fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
				fclose(fp);
				eval("mv", "-f", "/etc/openvpn/server/crl.pem", "/jffs/openvpn/");
				nvram_unset("vpn_crt_server_crl");
				symlink("/jffs/openvpn/crl.pem", "/etc/openvpn/server/crl.pem");
			} */
								} // nvram sets by webui
		} // if files not exist
			else { // if files exist
				if( !nvram_is_empty(nv1) || !nvram_is_empty(nv2) || !nvram_is_empty(nv3) )
							{ // nvram sets by webui
			sprintf(&buffer[0], "/etc/openvpn/server/ca.key", serverNum);
			if ( !nvram_is_empty(&buffer[0]) )
			{
				sprintf(&buffer[0], "/etc/openvpn/server/ca.key", serverNum);
				if ( check_if_file_exist("/jffs/openvpn/ca.key") )
				{
				eval("rm", "-f", "/etc/openvpn/server/ca.key");
				eval("rm", "-f", "/jffs/openvpn/ca.key");
				}
				fp = fopen(&buffer[0], "w");
				chmod(&buffer[0], S_IRUSR|S_IWUSR);
				sprintf(&buffer[0], "vpn_crt_server_ca_key", serverNum);
				fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
				fclose(fp);
				eval("mv", "-f", "/etc/openvpn/server/ca.key", "/jffs/openvpn/");
				nvram_unset("vpn_crt_server_ca_key");
				symlink("/jffs/openvpn/ca.key", "/etc/openvpn/server/ca.key");
			}

			sprintf(&buffer[0], "vpn_crt_server_ca", serverNum);
			if ( !nvram_is_empty(&buffer[0]) )
			{
				sprintf(&buffer[0], "/etc/openvpn/server/ca.crt", serverNum);
				if ( check_if_file_exist("/jffs/openvpn/ca.crt") )
				{
				eval("rm", "-f", "/etc/openvpn/server/ca.crt");
				eval("rm", "-f", "/jffs/openvpn/ca.crt");
				}
				fp = fopen(&buffer[0], "w");
				chmod(&buffer[0], S_IRUSR|S_IWUSR);
				sprintf(&buffer[0], "vpn_crt_server_ca", serverNum);
				fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
				fclose(fp);
				eval("mv", "-f", "/etc/openvpn/server/ca.crt", "/jffs/openvpn/");
				nvram_unset("vpn_crt_server_ca");
				symlink("/jffs/openvpn/ca.crt", "/etc/openvpn/server/ca.crt");
			}

			sprintf(&buffer[0], "vpn_crt_server_key", serverNum);
			if ( !nvram_is_empty(&buffer[0]) )
			{
				sprintf(&buffer[0], "/etc/openvpn/server/server.key", serverNum);
				if ( check_if_file_exist("/jffs/openvpn/server.key") )
				{
				eval("rm", "-f", "/etc/openvpn/server/server.key");
				eval("rm", "-f", "/jffs/openvpn/server.key");
				}
				fp = fopen(&buffer[0], "w");
				chmod(&buffer[0], S_IRUSR|S_IWUSR);
				sprintf(&buffer[0], "vpn_crt_server_key", serverNum);
				fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
				fclose(fp);
				eval("mv", "-f", "/etc/openvpn/server/server.key", "/jffs/openvpn/");
				nvram_unset("vpn_crt_server_key");
				symlink("/jffs/openvpn/server.key", "/etc/openvpn/server/server.key");
			}

			sprintf(&buffer[0], "vpn_crt_server_crt", serverNum);
			if ( !nvram_is_empty(&buffer[0]) )
			{
				sprintf(&buffer[0], "/etc/openvpn/server/server.crt", serverNum);
				if ( check_if_file_exist("/jffs/openvpn/server.crt") )
				{
				eval("rm", "-f", "/etc/openvpn/server/server.crt");
				eval("rm", "-f", "/jffs/openvpn/server.crt");
				}
				fp = fopen(&buffer[0], "w");
				chmod(&buffer[0], S_IRUSR|S_IWUSR);
				sprintf(&buffer[0], "vpn_crt_server_crt", serverNum);
				fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
				fclose(fp);
				eval("mv", "-f", "/etc/openvpn/server/server.crt", "/jffs/openvpn/");
				nvram_unset("vpn_crt_server_crt");
				symlink("/jffs/openvpn/server.crt", "/etc/openvpn/server/server.crt");
			}

			/* sprintf(&buffer[0], "vpn_crt_server_crl", serverNum);
			if ( !nvram_is_empty(&buffer[0]) )
			{
				sprintf(&buffer[0], "/etc/openvpn/server/crl.pem", serverNum);
				if ( check_if_file_exist("/jffs/openvpn/crl.pem") )
				{
				eval("rm", "-f", "/etc/openvpn/server/crl.pem");
				eval("rm", "-f", "/jffs/openvpn/crl.pem");
				}
				fp = fopen(&buffer[0], "w");
				chmod(&buffer[0], S_IRUSR|S_IWUSR);
				sprintf(&buffer[0], "vpn_crt_server_crl", serverNum);
				fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
				fclose(fp);
				eval("mv", "-f", "/etc/openvpn/server/crl.pem", "/jffs/openvpn/");
				nvram_unset("vpn_crt_server_crl");
				symlink("/jffs/openvpn/crl.pem", "/etc/openvpn/server/crl.pem");
			} */
							} // nvram sets by webui
				else 				{ // nvram empty
				symlink("/jffs/openvpn/ca.key", "/etc/openvpn/server/ca.key");
				nvram_unset("vpn_crt_server_ca_key");
				symlink("/jffs/openvpn/ca.crt", "/etc/openvpn/server/ca.crt");
				nvram_unset("vpn_crt_server_ca");
				symlink("/jffs/openvpn/server.key", "/etc/openvpn/server/server.key");
				nvram_unset("vpn_crt_server_key");
				symlink("/jffs/openvpn/server.crt", "/etc/openvpn/server/server.crt");
				nvram_unset("vpn_crt_server_crt");
			if ( check_if_file_exist("/jffs/openvpn/client.key") ) {
				symlink("/jffs/openvpn/client.key", "/etc/openvpn/server/client.key");
				nvram_unset("vpn_crt_server_client_key"); }
			if ( check_if_file_exist("/jffs/openvpn/client.crt") ) {
				symlink("/jffs/openvpn/client.crt", "/etc/openvpn/server/client.crt");
				nvram_unset("vpn_crt_server_client_crt"); }
			/* if ( check_if_file_exist("/jffs/openvpn/crl.pem") ) {
				symlink("/jffs/openvpn/crl.pem", "/etc/openvpn/server/crl.pem");
				nvram_unset("vpn_crt_server_crl"); } */
								} // nvram empty
			} // end if jffs exist and not empty
								} // end else - jffs exist

			if ( !check_if_dir_exist("/jffs/openvpn/") ) 
							{ // jffs not exist 
		sprintf(&buffer[0], "vpn_crt_server_crl", serverNum);
		if ( !nvram_is_empty(&buffer[0]) )
		 {
			sprintf(&buffer[0], "/etc/openvpn/server/crl.pem", serverNum);
			fp = fopen(&buffer[0], "w");
			chmod(&buffer[0], S_IRUSR|S_IWUSR);
			sprintf(&buffer[0], "vpn_crt_server_crl", serverNum);
			fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
			fclose(fp);
		 }
							} // jffs not exist
			else 
						{ // jffs exist 
		sprintf(&buffer[0], "vpn_crt_server_crl", serverNum);
				if ( !nvram_is_empty(&buffer[0]) && !check_if_file_exist("/jffs/openvpn/crl.pem") ) 
					{ //create file from nvram
					sprintf(&buffer[0], "/jffs/openvpn/crl.pem", serverNum);
					fp = fopen(&buffer[0], "w");
					chmod(&buffer[0], S_IRUSR|S_IWUSR);
					sprintf(&buffer[0], "vpn_crt_server_crl", serverNum);
					fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
					fclose(fp);
					symlink("/jffs/openvpn/crl.pem", "/etc/openvpn/server/crl.pem");
					nvram_unset("vpn_crt_server_crl");
					} //create file from nvram
				else if ( !nvram_is_empty(&buffer[0]) && check_if_file_exist("/jffs/openvpn/crl.pem") ) 
					{ //new config sets by webui
					sprintf(&buffer[0], "/jffs/openvpn/crl.pem", serverNum);
					eval("rm","-f","/etc/openvpn/server/crl.pem");
					eval("rm","-f","/jffs/openvpn/crl.pem");
					fp = fopen(&buffer[0], "w");
					chmod(&buffer[0], S_IRUSR|S_IWUSR);
					sprintf(&buffer[0], "vpn_crt_server_crl", serverNum);
					fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
					fclose(fp);
					symlink("/jffs/openvpn/crl.pem", "/etc/openvpn/server/crl.pem");
					nvram_unset("vpn_crt_server_crl");
					} //new config sets by webui
				else 
					{
					if ( check_if_file_exist("/jffs/openvpn/crl.pem") ) {
					symlink("/jffs/openvpn/crl.pem", "/etc/openvpn/server/crl.pem");
					nvram_unset("vpn_crt_server_crl"); }
					}
							}  // jffs exist

// end check if store certifitcate in jffs

		fprintf(fp_client, "<ca>\n");
		if ( check_if_dir_exist("/jffs/openvpn/") ) {
			nvram_set("vpn_tmp","");
			sprintf(&buffer[0], "vpn_tmp");
			sprintf(fpath, "/etc/openvpn/server/ca.crt", serverNum);
			fp = fopen(fpath, "r");
			set_crt_parsed(&buffer[0], fpath);
			fprintf(fp_client, "%s", get_parsed_crt(&buffer[0], buffer2));
			fclose(fp);
			nvram_unset("vpn_tmp");
		}
		else {
		sprintf(&buffer[0], "vpn_crt_server_ca", serverNum);
		fprintf(fp_client, "%s", get_parsed_crt(&buffer[0], buffer2));
		if (buffer2[strlen(buffer2)-1] != '\n') fprintf(fp_client, "\n");	// Append newline if missing
		}
		fprintf(fp_client, "</ca>\n");

		// Only do this if we do not have both userauth and useronly enabled at the same time
		if ( !(userauth && useronly) ) {
			/*
			   See if stored client cert was signed with our stored CA.  If not, it means
			   the CA was changed by the user and the current client crt/key no longer match,
			   so we should not insert them in the exported client ovp file.
			*/
			if ( check_if_dir_exist("/jffs/openvpn/") ) {
			eval("cp", "-f", "/etc/openvpn/server/client.crt", "/tmp/test.crt");
			}
			else {
			fp = fopen("/tmp/test.crt", "w");
			sprintf(&buffer[0], "vpn_crt_server_client_crt", serverNum);
			fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
			fclose(fp);
			}

			sprintf(&buffer[0], "openssl verify -CAfile /etc/openvpn/server/ca.crt /tmp/test.crt > /tmp/output.txt", serverNum);
			system(&buffer[0]);
			f_read_string("/tmp/output.txt", &buffer[0], 64);
	                unlink("/tmp/test.crt");

			if (!strncmp(&buffer[0],"/tmp/test.crt: OK",17))
				valid = 1;

			fprintf(fp_client, "<cert>\n");
				if ( (check_if_dir_exist("/jffs/openvpn/")) && (valid == 1) ) {
				nvram_set("vpn_tmp","");
				sprintf(&buffer[0], "vpn_tmp");
				sprintf(fpath, "/etc/openvpn/server/client.crt", serverNum);
				fp = fopen(fpath, "r");
				set_crt_parsed(&buffer[0], fpath);
				fprintf(fp_client, "%s", get_parsed_crt(&buffer[0], buffer2));
				fclose(fp);
				nvram_unset("vpn_tmp");
				}
				else {
			sprintf(&buffer[0], "vpn_crt_server_client_crt", serverNum);
			if ((valid == 1) && ( !nvram_is_empty(&buffer[0]) ) ) {
				fprintf(fp_client, "%s", get_parsed_crt(&buffer[0], buffer2));
				if (buffer2[strlen(buffer2)-1] != '\n') fprintf(fp_client, "\n");  // Append newline if missing
			} else {
				fprintf(fp_client, "    paste client certificate data here\n");
			}
				}
			fprintf(fp_client, "</cert>\n");

			fprintf(fp_client, "<key>\n");
				if ( check_if_dir_exist("/jffs/openvpn/") && (valid == 1) ) {
				nvram_set("vpn_tmp","");
				sprintf(&buffer[0], "vpn_tmp");
				sprintf(fpath, "/etc/openvpn/server/client.key", serverNum);
				fp = fopen(fpath, "r");
				set_crt_parsed(&buffer[0], fpath);
				fprintf(fp_client, "%s", get_parsed_crt(&buffer[0], buffer2));
				fclose(fp);
				nvram_unset("vpn_tmp");
				}
				else {
			sprintf(&buffer[0], "vpn_crt_server_client_key", serverNum);
			if ((valid == 1) && ( !nvram_is_empty(&buffer[0]) ) ) {
				fprintf(fp_client, "%s", get_parsed_crt(&buffer[0], buffer2));
				if (buffer2[strlen(buffer2)-1] != '\n') fprintf(fp_client, "\n");  // Append newline if missing
			} else {
				fprintf(fp_client, "    paste client key data here\n");
			}
				}
			fprintf(fp_client, "</key>\n");
		}


			if ( !check_if_dir_exist("/jffs/openvpn/") ) 
			{ //jffs not exist
		sprintf(&buffer[0], "vpn_crt_server_dh", serverNum);
		if ( !nvram_is_empty(&buffer[0]) )
		{
			sprintf(&buffer[0], "/etc/openvpn/server/dh.pem", serverNum);
			fp = fopen(&buffer[0], "w");
			chmod(&buffer[0], S_IRUSR|S_IWUSR);
			sprintf(&buffer[0], "vpn_crt_server_dh", serverNum);
			fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
			fclose(fp);
		}
		else
		{	//generate dh param file
			sprintf(fpath, "/etc/openvpn/server/dh.pem", serverNum);
			eval("openssl", "dhparam", "-out", fpath, "512");
			fp = fopen(fpath, "r");
			if(fp) {
				sprintf(&buffer[0], "vpn_crt_server_dh", serverNum);
				set_crt_parsed(&buffer[0], fpath);
				fclose(fp);
			}
		}
			} //jffs not exist
			else 
				{ //jffs exist 
		sprintf(&buffer[0], "vpn_crt_server_dh", serverNum);
				if ( !check_if_file_exist("/jffs/openvpn/dh.pem") && nvram_is_empty(&buffer[0]) )
					{ //generate dh param file
				sprintf(fpath, "/etc/openvpn/server/dh.pem", serverNum);
				eval("openssl", "dhparam", "-out", fpath, "512");
				eval("mv", "-f", "/etc/openvpn/server/dh.pem", "/jffs/openvpn/");
				nvram_unset("vpn_crt_server_dh");
				symlink("/jffs/openvpn/dh.pem", "/etc/openvpn/server/dh.pem");
					} //generate dh param file
				else if ( !check_if_file_exist("/jffs/openvpn/dh.pem") && !nvram_is_empty(&buffer[0]) )
					{ //create file from nvram
					sprintf(&buffer[0], "/etc/openvpn/server/dh.pem", serverNum);
					fp = fopen(&buffer[0], "w");
					chmod(&buffer[0], S_IRUSR|S_IWUSR);
					sprintf(&buffer[0], "vpn_crt_server_dh", serverNum);
					fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
					fclose(fp);
					eval("mv", "-f", "/etc/openvpn/server/dh.pem", "/jffs/openvpn/");
					nvram_unset("vpn_crt_server_dh");
					symlink("/jffs/openvpn/dh.pem", "/etc/openvpn/server/dh.pem");
					} //create file from nvram
				else if ( check_if_file_exist("/jffs/openvpn/dh.pem") && !nvram_is_empty(&buffer[0]) )
						{ // new config sets by webui
					sprintf(&buffer[0], "/etc/openvpn/server/dh.pem", serverNum);
					eval("rm", "-f", "/etc/openvpn/server/dh.pem");
					eval("rm", "-f", "//jffs/openvpn/dh.pem", "");
					fp = fopen(&buffer[0], "w");
					chmod(&buffer[0], S_IRUSR|S_IWUSR);
					sprintf(&buffer[0], "vpn_crt_server_dh", serverNum);
					fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
					fclose(fp);
					eval("mv", "-f", "/etc/openvpn/server/dh.pem", "/jffs/openvpn/");
					nvram_unset("vpn_crt_server_dh");
					symlink("/jffs/openvpn/dh.pem", "/etc/openvpn/server/dh.pem");
						} // new config sets by webui
				else 
					{ // file exist and nvram empty
					symlink("/jffs/openvpn/dh.pem", "/etc/openvpn/server/dh.pem");
					nvram_unset("vpn_crt_server_dh");
					} // file exist and nvram empty
				} //jffs exist

	}
	sprintf(&buffer[0], "vpn_server_hmac", serverNum);
	if ( cryptMode == SECRET || (cryptMode == TLS && nvram_get_int(&buffer[0]) >= 0) )
	{
			if ( !check_if_dir_exist("/jffs/openvpn/") ) 
								{ // jffs not exist
		sprintf(&buffer[0], "vpn_crt_server_static", serverNum);	
		if ( !nvram_is_empty(&buffer[0]) )
		{
			sprintf(&buffer[0], "/etc/openvpn/server/static.key", serverNum);
			fp = fopen(&buffer[0], "w");
			chmod(&buffer[0], S_IRUSR|S_IWUSR);
			sprintf(&buffer[0], "vpn_crt_server_static", serverNum);
			fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
			fclose(fp);
		}
		else
		{	//generate openvpn static key
			sprintf(fpath, "/etc/openvpn/server/static.key", serverNum);
			eval("openvpn", "--genkey", "--secret", fpath);
			fp = fopen(fpath, "r");
			if(fp) {
				set_crt_parsed(&buffer[0], fpath);
				fclose(fp);
			}
		}
	
								} // end jffs not exist
			else 
							{ // jffs exist 
		sprintf(&buffer[0], "vpn_crt_server_static", serverNum);
			if ( !check_if_file_exist("/jffs/openvpn/static.key") && nvram_is_empty(&buffer[0]) )
					{ //generate openvpn static key
					sprintf(fpath, "/etc/openvpn/server/static.key", serverNum);
					eval("openvpn", "--genkey", "--secret", fpath);
					eval("mv", "-f", "/etc/openvpn/server/static.key", "/jffs/openvpn/");
					nvram_unset("vpn_crt_server_static");
					symlink("/jffs/openvpn/static.key", "/etc/openvpn/server/static.key");
					} //generate openvpn static key
			else if ( !check_if_file_exist("/jffs/openvpn/static.key") && !nvram_is_empty(&buffer[0]) )
					{ //create file from nvram
					sprintf(&buffer[0], "/etc/openvpn/server/static.key", serverNum);
					fp = fopen(&buffer[0], "w");
					chmod(&buffer[0], S_IRUSR|S_IWUSR);
					sprintf(&buffer[0], "vpn_crt_server_static", serverNum);
					fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
					fclose(fp);
					eval("mv", "-f", "/etc/openvpn/server/static.key", "/jffs/openvpn/");
					nvram_unset("vpn_crt_server_static");
					symlink("/jffs/openvpn/static.key", "/etc/openvpn/server/static.key");
					} //create file from nvram
			else if ( check_if_file_exist("/jffs/openvpn/static.key") && !nvram_is_empty(&buffer[0]) )
					{ //new config sets by webui
					sprintf(&buffer[0], "/etc/openvpn/server/static.key", serverNum);
					eval("rm", "-f", "/etc/openvpn/server/static.key");
					eval("rm", "-f", "/jffs/openvpn/static.key");
					fp = fopen(&buffer[0], "w");
					chmod(&buffer[0], S_IRUSR|S_IWUSR);
					sprintf(&buffer[0], "vpn_crt_server_static", serverNum);
					fprintf(fp, "%s", get_parsed_crt(&buffer[0], buffer2));
					fclose(fp);
					eval("mv", "-f", "/etc/openvpn/server/static.key", "/jffs/openvpn/");
					nvram_unset("vpn_crt_server_static");
					symlink("/jffs/openvpn/static.key", "/etc/openvpn/server/static.key");
					} //new config sets by webui
			else 
					{ // jffs file exist and nvram empty
					symlink("/jffs/openvpn/static.key", "/etc/openvpn/server/static.key");
					nvram_unset("vpn_crt_server_static");
					} // jffs file exist and nvram empty
							} // end jffs exist

		if(cryptMode == TLS)
			fprintf(fp_client, "<tls-auth>\n");
		else if(cryptMode == SECRET)
			fprintf(fp_client, "<secret>\n");

		if ( check_if_dir_exist("/jffs/openvpn/") ) {
			sprintf(fpath, "/etc/openvpn/server/static.key", serverNum);
			nvram_set("vpn_tmp","");
			sprintf(&buffer[0], "vpn_tmp");
			fp = fopen(fpath, "r");
			set_crt_parsed(&buffer[0], fpath);
			fprintf(fp_client, "%s", get_parsed_crt(&buffer[0], buffer2));
			fclose(fp);
			nvram_unset("vpn_tmp");
		}
		else {
		sprintf(&buffer[0], "vpn_crt_server_static", serverNum);
		fprintf(fp_client, "%s", get_parsed_crt(&buffer[0], buffer2));
		if (buffer2[strlen(buffer2)-1] != '\n') fprintf(fp_client, "\n");  // Append newline if missing
		}


		if(cryptMode == TLS) {
			fprintf(fp_client, "</tls-auth>\n");
			sprintf(&buffer[0], "vpn_server_hmac", serverNum);
			nvi = nvram_get_int(&buffer[0]);
			if(nvi == 1)
				fprintf(fp_client, "key-direction 0\n");
			else if(nvi == 0)
				fprintf(fp_client, "key-direction 1\n");
		}
		else if(cryptMode == SECRET)
			fprintf(fp_client, "</secret>\n");
	}

	vpnlog(VPN_LOG_EXTRA,"Done writing certs/keys");
	nvram_commit();

	fprintf(fp_client, "resolv-retry infinite\n");
	fprintf(fp_client, "nobind\n");
	fclose(fp_client);
	vpnlog(VPN_LOG_EXTRA,"Done writing client config file");

	// Run postconf custom script on it if it exists
	sprintf(&buffer[0], "openvpnserver.postconf", serverNum);
	sprintf(&buffer2[0], "/etc/openvpn/server/config.ovpn", serverNum);
	run_postconf(&buffer[0], &buffer2[0]);



        // Start the VPN client
	sprintf(&buffer[0], "/etc/openvpn/vpnserver", serverNum);
	sprintf(&buffer2[0], "/etc/openvpn/server", serverNum);

	taskset_ret = cpu_eval(NULL, (serverNum == 1 ? CPU1 : CPU0 ), &buffer[0], "--cd", &buffer2[0], "--config", "config.ovpn");

	vpnlog(VPN_LOG_INFO,"Starting OpenVPN Server 1", serverNum);
	if (taskset_ret)
	{
		vpnlog(VPN_LOG_ERROR,"Starting VPN instance failed...");
		stop_vpnserver(serverNum);
		return;
	}
	vpnlog(VPN_LOG_EXTRA,"Done starting openvpn");

	// Handle firewall rules if appropriate
	sprintf(&buffer[0], "vpn_server_firewall", serverNum);
	if ( !nvram_contains_word(&buffer[0], "custom") )
	{
		// Create firewall rules
		vpnlog(VPN_LOG_EXTRA,"Creating firewall rules");
		mkdir("/etc/openvpn/fw", 0700);
		sprintf(&buffer[0], "/etc/openvpn/fw/server-fw.sh", serverNum);
		fp = fopen(&buffer[0], "w");
		chmod(&buffer[0], S_IRUSR|S_IWUSR|S_IXUSR);
		fprintf(fp, "#!/bin/sh\n");
		sprintf(&buffer[0], "vpn_server_proto", serverNum);
		strncpy(&buffer[0], nvram_safe_get(&buffer[0]), BUF_SIZE);
		fprintf(fp, "iptables -t nat -I PREROUTING -p %s ", strtok(&buffer[0], "-"));
		sprintf(&buffer[0], "vpn_server_port", serverNum);
		fprintf(fp, "--dport %d -j ACCEPT\n", nvram_get_int(&buffer[0]));
		sprintf(&buffer[0], "vpn_server_proto", serverNum);
		strncpy(&buffer[0], nvram_safe_get(&buffer[0]), BUF_SIZE);
		fprintf(fp, "iptables -I INPUT -p %s ", strtok(&buffer[0], "-"));
		sprintf(&buffer[0], "vpn_server_port", serverNum);
		fprintf(fp, "--dport %d -j ACCEPT\n", nvram_get_int(&buffer[0]));
		sprintf(&buffer[0], "vpn_server_firewall", serverNum);
		if ( !nvram_contains_word(&buffer[0], "external") )
		{
			fprintf(fp, "iptables -I INPUT -i %s -j ACCEPT\n", &iface[0]);
			fprintf(fp, "iptables -I FORWARD %d -i %s -j ACCEPT\n", (nvram_match("cstats_enable", "1") ? 4 : 2), &iface[0]);
		}
		if (nvram_match("cstats_enable", "1")) {
			ipt_account(fp, &iface[0]);
		}
		fclose(fp);
		vpnlog(VPN_LOG_EXTRA,"Done creating firewall rules");

		// Run the firewall rules
		vpnlog(VPN_LOG_EXTRA,"Running firewall rules");
		sprintf(&buffer[0], "/etc/openvpn/fw/server-fw.sh", serverNum);
		argv[0] = &buffer[0];
		argv[1] = NULL;
		_eval(argv, NULL, 0, NULL);
		vpnlog(VPN_LOG_EXTRA,"Done running firewall rules");
	}

	// Set up cron job
	sprintf(&buffer[0], "vpn_server_poll", serverNum);
	if ( (nvi = nvram_get_int(&buffer[0])) > 0 )
	{
		vpnlog(VPN_LOG_EXTRA,"Adding cron job");
		argv[0] = "cru";
		argv[1] = "a";
		sprintf(&buffer[0], "CheckVPNserver", serverNum);
		argv[2] = &buffer[0];
		sprintf(&buffer[strlen(&buffer[0])+1], "*/1 * * * * service start_vpnserver", nvi, serverNum);
		argv[3] = &buffer[strlen(&buffer[0])+1];
		argv[4] = NULL;
		_eval(argv, NULL, 0, NULL);
		vpnlog(VPN_LOG_EXTRA,"Done adding cron job");
	}

	sprintf(&buffer[0], "vpn_server_state", serverNum);
	nvram_set(&buffer[0], "2");	//running
	sprintf(&buffer[0], "vpn_server_errno", serverNum);
	nvram_set(&buffer[0], "0");

#ifdef LINUX26
//	sprintf(&buffer[0], "vpn_server", serverNum);
//	allow_fastnat(buffer, 0);
//	try_enabling_fastnat();
#endif

	vpnlog(VPN_LOG_INFO,"VPN GUI server backend complete.");
}

void stop_vpnserver(int serverNum)
{
	int argc;
	char *argv[9];
	char buffer[BUF_SIZE];

	sprintf(&buffer[0], "stop_vpnserver", serverNum);
	if (getpid() != 1) {
		notify_rc(&buffer[0]);
		return;
	}

	vpnlog(VPN_LOG_INFO,"Stopping VPN GUI server backend.");

	// Remove cron job
	vpnlog(VPN_LOG_EXTRA,"Removing cron job");
	argv[0] = "cru";
	argv[1] = "d";
	sprintf(&buffer[0], "CheckVPNserver", serverNum);
	argv[2] = &buffer[0];
	argv[3] = NULL;
	_eval(argv, NULL, 0, NULL);
	vpnlog(VPN_LOG_EXTRA,"Done removing cron job");

	// Remove firewall rules
	vpnlog(VPN_LOG_EXTRA,"Removing firewall rules.");
	sprintf(&buffer[0], "/etc/openvpn/fw/server-fw.sh", serverNum);
	argv[0] = "sed";
	argv[1] = "-i";
	argv[2] = "s/-A/-D/g;s/-I/-D/g;s/FORWARD\\ [0-9]\\ /FORWARD\\ /g";
	argv[3] = &buffer[0];
	argv[4] = NULL;
	if (!_eval(argv, NULL, 0, NULL))
	{
		argv[0] = &buffer[0];
		argv[1] = NULL;
		_eval(argv, NULL, 0, NULL);
	}
	vpnlog(VPN_LOG_EXTRA,"Done removing firewall rules.");

	// Stop the VPN server
	vpnlog(VPN_LOG_EXTRA,"Stopping OpenVPN server.");
	sprintf(&buffer[0], "vpnserver", serverNum);
	if ( !ovpn_waitfor(&buffer[0]) )
		vpnlog(VPN_LOG_EXTRA,"OpenVPN server stopped.");

	// NVRAM setting for device type could have changed, just try to remove both
	vpnlog(VPN_LOG_EXTRA,"Removing VPN device.");
	sprintf(&buffer[0], "openvpn --rmtun --dev tap21", serverNum+SERVER_IF_START);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	_eval(argv, NULL, 0, NULL);

	sprintf(&buffer[0], "openvpn --rmtun --dev tun21", serverNum+SERVER_IF_START);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	_eval(argv, NULL, 0, NULL);
	vpnlog(VPN_LOG_EXTRA,"VPN device removed.");

	modprobe_r("tun");

	if ( nvram_get_int("vpn_debug") <= VPN_LOG_EXTRA )
	{
		vpnlog(VPN_LOG_EXTRA,"Removing generated files.");
		// Delete all files for this server
		sprintf(&buffer[0], "rm -rf /etc/openvpn/server /etc/openvpn/fw/server-fw.sh /etc/openvpn/vpnserver",serverNum,serverNum,serverNum);
		for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
		_eval(argv, NULL, 0, NULL);

		// Attempt to remove directories.  Will fail if not empty
		rmdir("/etc/openvpn/fw");
		rmdir("/etc/openvpn");
		vpnlog(VPN_LOG_EXTRA,"Done removing generated files.");
	}

#ifdef LINUX26
//	sprintf(&buffer[0], "vpn_server", serverNum);
//	allow_fastnat(buffer, 1);
//	try_enabling_fastnat();
#endif

	sprintf(&buffer[0], "vpn_server_state", serverNum);
	nvram_set(&buffer[0], "0");
	sprintf(&buffer[0], "vpn_server_errno", serverNum);
	nvram_set(&buffer[0], "0");

	vpnlog(VPN_LOG_INFO,"VPN GUI server backend stopped.");
}

void start_vpn_eas()
{
	char buffer[16], *cur;
	int nums[4], i;

	if (strlen(nvram_safe_get("vpn_serverx_start")) == 0 && strlen(nvram_safe_get("vpn_clientx_eas")) == 0) return;
	// wait for time sync for a while
	i = 10;
	while (time(0) < 1325376000 && i--) {
		sleep(1);
	}

	// Parse and start servers
	strlcpy(&buffer[0], nvram_safe_get("vpn_serverx_start"), sizeof(buffer));
	if ( strlen(&buffer[0]) != 0 ) vpnlog(VPN_LOG_INFO, "Starting OpenVPN server (eas): 1", &buffer[0]);
	i = 0;
	//for( cur = strtok(&buffer[0],","); cur != NULL && i < 4; cur = strtok(NULL, ",")) { nums[i++] = atoi(cur); }
	nums[i] = 0;
	//for( i = 0; nums[i] > 0; i++ )
	//{

		sprintf(&buffer[0], "vpnserver", nums[i]);
		if ( pidof(&buffer[0]) >= 0 )
		{
			vpnlog(VPN_LOG_INFO, "Stopping OpenVPN server 1 (eas)", nums[i]);
			stop_vpnserver(1);
		}

		vpnlog(VPN_LOG_INFO, "Starting OpenVPN server 1 (eas)", nums[i]);
		start_vpnserver(1);
	//}

	// Parse and start clients
	strlcpy(&buffer[0], nvram_safe_get("vpn_clientx_eas"), sizeof(buffer));
	if ( strlen(&buffer[0]) != 0 ) vpnlog(VPN_LOG_INFO, "Starting clients (eas): 1", &buffer[0]);
	i = 0;
	//for( cur = strtok(&buffer[0],","); cur != NULL && i < 4; cur = strtok(NULL, ",")) { nums[i++] = atoi(cur); }
	nums[i] = 0;
	//for( i = 0; nums[i] > 0; i++ )
	//{
		sprintf(&buffer[0], "vpnclient", nums[i]);
		if ( pidof(&buffer[0]) >= 0 )
		{
			vpnlog(VPN_LOG_INFO, "Stopping OpenVPN client 1 (eas)", nums[i]);
			stop_vpnclient(1);
		}

		vpnlog(VPN_LOG_INFO, "Starting OpenVPN client 1 (eas)", nums[i]);
		start_vpnclient(1);
	//}
}

void stop_vpn_eas()
{
	char buffer[16], *cur;
	int nums[4], i;

	// Parse and stop servers
	strlcpy(&buffer[0], nvram_safe_get("vpn_serverx_start"), sizeof(buffer));
	if ( strlen(&buffer[0]) != 0 ) vpnlog(VPN_LOG_INFO, "Stopping OpenVPN servers (eas): 1", &buffer[0]);
	i = 0;
	//for( cur = strtok(&buffer[0],","); cur != NULL && i < 4; cur = strtok(NULL, ",")) { nums[i++] = atoi(cur); }
	nums[i] = 0;
	//for( i = 0; nums[i] > 0; i++ )
	//{
		sprintf(&buffer[0], "vpnserver", nums[i]);
		if ( pidof(&buffer[0]) >= 0 )
		{
			vpnlog(VPN_LOG_INFO, "Stopping OpenVPN server 1 (eas)", nums[i]);
			stop_vpnserver(1);
		}
	//}

	// Parse and stop clients
	strlcpy(&buffer[0], nvram_safe_get("vpn_clientx_eas"), sizeof(buffer));
	if ( strlen(&buffer[0]) != 0 ) vpnlog(VPN_LOG_INFO, "Stopping OpenVPN clients (eas): 1", &buffer[0]);
	i = 0;
	//for( cur = strtok(&buffer[0],","); cur != NULL && i < 4; cur = strtok(NULL, ",")) { nums[i++] = atoi(cur); }
	nums[i] = 0;
	//for( i = 0; nums[i] > 0; i++ )
	//{
		sprintf(&buffer[0], "vpnclient", nums[i]);
		if ( pidof(&buffer[0]) >= 0 )
		{
			vpnlog(VPN_LOG_INFO, "Stopping OpenVPN client 1 (eas)", nums[i]);
			stop_vpnclient(1);
		}
	//}
}

void run_vpn_firewall_scripts()
{
	DIR *dir;
	struct dirent *file;
	char *fn;
	char *argv[3];

	if ( chdir("/etc/openvpn/fw") )
		return;

	dir = opendir("/etc/openvpn/fw");

	vpnlog(VPN_LOG_EXTRA,"Beginning all firewall scripts...");
	while ( (file = readdir(dir)) != NULL )
	{
		fn = file->d_name;
		if ( fn[0] == '.' )
			continue;
		vpnlog(VPN_LOG_INFO,"Running firewall script: %s", fn);
		argv[0] = "/bin/sh";
		argv[1] = fn;
		argv[2] = NULL;
		_eval(argv, NULL, 0, NULL);
	}
	vpnlog(VPN_LOG_EXTRA,"Done with all firewall scripts...");

	closedir(dir);
}

void write_vpn_dnsmasq_config(FILE* f)
{
	char nv[16];
	char buf[24];
	char *pos, ch;
	int cur, ch2;
	DIR *dir;
	struct dirent *file;
	FILE *dnsf;

	strlcpy(&buf[0], nvram_safe_get("vpn_serverx_dns"), sizeof(buf));
	//for ( pos = strtok(&buf[0],","); pos != NULL; pos=strtok(NULL, ",") )
	//{
		cur = 1; //atoi(pos);
	//	if ( cur )
	//	{
			vpnlog(VPN_LOG_EXTRA, "Adding server interface to dns config", cur);
			snprintf(&nv[0], sizeof(nv), "vpn_server_if", cur);
			fprintf(f, "interface=%s21\n", nvram_safe_get(&nv[0]), SERVER_IF_START+cur);
	//	}
	//}

	if ( (dir = opendir("/etc/openvpn/dns")) != NULL )
	{
		while ( (file = readdir(dir)) != NULL )
		{
			if ( file->d_name[0] == '.' )
				continue;

			if ( sscanf(file->d_name, "client%d.resol%c", &cur, &ch) == 2 )
			{
				vpnlog(VPN_LOG_EXTRA, "Checking ADNS settings for client ", cur);
				snprintf(&buf[0], sizeof(buf), "vpn_client_adns", cur);
				if ( nvram_get_int(&buf[0]) == 2 )
				{
					vpnlog(VPN_LOG_INFO, "Adding strict-order to dnsmasq config for client ", cur);
					fprintf(f, "strict-order\n");
					break;
				}
			}

			if ( sscanf(file->d_name, "client%d.con%c", &cur, &ch) == 2 )
			{
				if ( (dnsf = fopen(file->d_name, "r")) != NULL )
				{
					vpnlog(VPN_LOG_INFO, "Adding Dnsmasq config from %s", file->d_name);

					while( !feof(dnsf) )
					{
						ch2 = fgetc(dnsf);
						fputc(ch2==EOF?'\n':ch2, f);
					}

					fclose(dnsf);
				}
			}
		}
	}
}

int write_vpn_resolv(FILE* f)
{
	DIR *dir;
	struct dirent *file;
	char *fn, ch, num, buf[24];
	FILE *dnsf;
	int strictlevel = 0, ch2;

	if ( chdir("/etc/openvpn/dns") )
		return 0;

	dir = opendir("/etc/openvpn/dns");

	vpnlog(VPN_LOG_EXTRA, "Adding DNS entries...");
	while ( (file = readdir(dir)) != NULL )
	{
		fn = file->d_name;

		if ( fn[0] == '.' )
			continue;

		if ( sscanf(fn, "client%d.resol%c", &num, &ch) == 2 )
		{
			if ( (dnsf = fopen(fn, "r")) == NULL )
				continue;

			vpnlog(VPN_LOG_INFO,"Adding DNS entries from %s", fn);

			while( !feof(dnsf) )
			{
				ch2 = fgetc(dnsf);
				fputc(ch2==EOF?'\n':ch2, f);
			}

			fclose(dnsf);

			snprintf(&buf[0], sizeof(buf), "vpn_client_adns", num);

			strictlevel = nvram_get_int(&buf[0]);
		}
	}
	vpnlog(VPN_LOG_EXTRA, "Done with DNS entries...");

	closedir(dir);

	return strictlevel;
}

void create_openvpn_passwd()
{
	unsigned char s[512];
	char *p;
	char salt[32];
	char *nv, *nvp, *b;
	char *username, *passwd;
	FILE *fp1, *fp2, *fp3;
	int id = 200;

	strcpy(salt, "$1$");
	f_read("/dev/urandom", s, 6);
	base64_encode(s, salt + 3, 6);
	salt[3 + 8] = 0;
	p = salt;
	while (*p) {
		if (*p == '+') *p = '.';
		++p;
	}

	fp1=fopen("/etc/shadow.openvpn", "w");
	fp2=fopen("/etc/passwd.openvpn", "w");
	fp3=fopen("/etc/group.openvpn", "w");
	if (!fp1 || !fp2 || !fp3) return;

	nv = nvp = strdup(nvram_safe_get("vpn_serverx_clientlist"));

	if(nv) {
		while ((b = strsep(&nvp, "<")) != NULL) {
			if((vstrsep(b, ">", &username, &passwd)!=2)) continue;
			if(strlen(username)==0||strlen(passwd)==0) continue;

			p = crypt(passwd, salt);
			fprintf(fp1, "%s:%s:0:0:99999:7:0:0:\n", username, p);
			fprintf(fp2, "%s:x:%d:%d:::\n", username, id, id);
			fprintf(fp3, "%s:x:%d:\n", username, id);
			id++;
		}
		free(nv);
	}
	fclose(fp1);
	fclose(fp2);
	fclose(fp3);
}


int _check_ovpn_enabled(int unit, char *config){
        char tmp[2];

        if (unit > 9) return 0;

        sprintf(tmp,"%d", unit);

        if (strstr(nvram_safe_get(config), tmp)) return 1;

        return 0;
}

int check_ovpn_server_enabled(int unit){
	return _check_ovpn_enabled(unit, "vpn_serverx_start");
}

int check_ovpn_client_enabled(int unit){
	return _check_ovpn_enabled(unit, "vpn_clientx_eas");
}

