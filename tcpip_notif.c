/*
 * =====================================================================================
 *
 *       Filename:  tcpip_notif.c
 *
 *    Description: This file implements notif chain routines for TCP/IP stack lib 
 *
 *        Version:  1.0
 *        Created:  10/17/2020 02:20:43 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ABHISHEK SAGAR (), sachinites@gmail.com
 *   Organization:  Juniper Networks
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <memory.h>
#include "Interface/InterfaceUApi.h"
#include "net.h"
#include "tcpip_notif.h"
#include "pkt_block.h"

/* Create a notif chain for interface
 * config change notification to applications */
static notif_chain_t nfc_intf = {
	"Notif Chain for Interfaces",
	0, 0,
	{0, 0}
};

/* Interface notif chain elem do not have any keys */
void
nfc_intf_register_for_events(nfc_app_cb app_cb){

	notif_chain_elem_t nfce_template;

	memset(&nfce_template, 0 , sizeof(notif_chain_elem_t));

	nfce_template.app_cb = app_cb;
	nfce_template.is_key_set = false;
	init_glthread(&nfce_template.glue);
	nfc_register_notif_chain(&nfc_intf, &nfce_template);
}

void
nfc_intf_invoke_notification_to_sbscribers(
	Interface *intf,
	intf_prop_changed_t *old_intf_prop_changed,
	uint32_t change_flags) {

	intf_notif_data_t intf_notif_data;
	
	intf_notif_data.interface = intf;
	intf_notif_data.old_intf_prop_changed = old_intf_prop_changed;
	intf_notif_data.change_flags = change_flags;

	nfc_invoke_notif_chain(NULL, 
							&nfc_intf,
						   (void *) &intf_notif_data,
							sizeof(intf_notif_data_t),
							0, 0, TASK_PRIORITY_COMPUTE);
}


/* 
 * Notif chain used for printing application specific
 * pkts by  tracing infra
 * */

static notif_chain_t nfc_print_pkts = {
	"Notif Chain For Tracing appln Pkts",
	0, 0,
	{0, 0},
};

void
nfc_register_for_pkt_tracing(
	uint32_t protocol_no,
	nfc_app_cb app_cb) {

	notif_chain_elem_t nfce_template;

	memset(&nfce_template, 0 , sizeof(notif_chain_elem_t));

	memcpy(&nfce_template.key, (char *)&protocol_no, sizeof(protocol_no));
	nfce_template.key_size = sizeof(protocol_no);
	nfce_template.is_key_set = true;

	nfce_template.app_cb = app_cb;
	init_glthread(&nfce_template.glue);
	nfc_register_notif_chain(&nfc_print_pkts, &nfce_template);
}

int
nfc_pkt_trace_invoke_notif_to_sbscribers(
					uint32_t protocol_no,
					pkt_block_t *pkt_block,
					c_string pkt_print_buffer){

	pkt_info_t pkt_info;
	
	pkt_info.protocol_no = protocol_no;
	pkt_info.pkt_block = pkt_block;
	pkt_block_reference(pkt_block);
	pkt_info.pkt_print_buffer = pkt_print_buffer;
	pkt_info.bytes_written = 0;

	nfc_invoke_notif_chain(NULL,
						   &nfc_print_pkts,
						   (void *) &pkt_info,
						   sizeof(pkt_info_t),
						   (char *)&protocol_no,
						   sizeof(protocol_no),
                           TASK_PRIORITY_LOW);

	pkt_block_dereference(pkt_block);

	return pkt_info.bytes_written;
}

