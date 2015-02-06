vpnd_state='<% nvram_get("vpn_server_state"); %>';
vpnd_errno='<% nvram_get("vpn_server_errno"); %>';

vpn_upload_state='<% nvram_get("vpn_upload_state"); %>';
vpn_crt_client_ca='<% nvram_clean_get("vpn_crt_client_ca"); %>';
vpn_crt_client_crt='<% nvram_clean_get("vpn_crt_client_crt"); %>';
vpn_crt_client_key='<% nvram_clean_get("vpn_crt_client_key"); %>';
vpn_crt_client_static='<% nvram_clean_get("vpn_crt_client_static"); %>';
vpn_crt_client_crl='<% nvram_get("vpn_crt_client_crl"); %>';

vpn_crt_server_ca='<% nvram_clean_get("vpn_crt_server_ca"); %>';
vpn_crt_server_crt='<% nvram_clean_get("vpn_crt_server_crt"); %>';
vpn_crt_server_key='<% nvram_clean_get("vpn_crt_server_key"); %>';
vpn_crt_server_dh='<% nvram_clean_get("vpn_crt_server_dh"); %>';
vpn_crt_server_crl='<% nvram_clean_get("vpn_crt_server_crl"); %>';
vpn_crt_server_static='<% nvram_clean_get("vpn_crt_server_static"); %>';

