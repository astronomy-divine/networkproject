#include "../../tcp_public.h"
#include "isis_struct.h"

/* Buffer passed shuld be minimum 32B*/
const c_string
isis_lan_id_tostring (isis_lan_id_t *lan_id, const c_string buffer) {

    unsigned char ip_addr_str[16];
    tcp_ip_covert_ip_n_to_p (lan_id->rtr_id, ip_addr_str);
    snprintf ((char *)buffer, 32, "%s-%d", ip_addr_str, lan_id->pn_id);
    return buffer;
}

int
isis_lan_id_compare (isis_lan_id_t *lan_id1,
                                    isis_lan_id_t *lan_id2) {

    if (lan_id1->rtr_id > lan_id2->rtr_id) return CMP_PREFERRED;
    if (lan_id1->rtr_id < lan_id2->rtr_id) return CMP_NOT_PREFERRED;

    if (lan_id1->pn_id > lan_id2->pn_id) return CMP_PREFERRED;
    if (lan_id1->pn_id < lan_id2->pn_id) return CMP_NOT_PREFERRED;

     return CMP_PREF_EQUAL;
}

const c_string
isis_lsp_id_tostring (isis_lsp_id_t *lsp_id, const c_string buffer) {

    unsigned char ip_addr_str[16];
    tcp_ip_covert_ip_n_to_p (lsp_id->sys_id.rtr_id, ip_addr_str);
    snprintf ((char *)buffer, 32, "%s-%d-%d",
        ip_addr_str, lsp_id->sys_id.pn_id, lsp_id->fragment);
    return buffer;
}

int
isis_lsp_id_compare (isis_lsp_id_t *lsp_id1,
                                    isis_lsp_id_t *lsp_id2) {

    int8_t rc = isis_lan_id_compare (&lsp_id1->sys_id, &lsp_id2->sys_id);
    if (rc != CMP_PREF_EQUAL) return rc;
    if (lsp_id1->fragment > lsp_id2->fragment) return CMP_PREFERRED;
    if (lsp_id1->fragment < lsp_id2->fragment) return CMP_NOT_PREFERRED;
    return CMP_PREF_EQUAL;
}

const c_string
isis_system_id_tostring (isis_system_id_t *sys_id, const c_string buffer) {

    unsigned char ip_addr_str[16];
    tcp_ip_covert_ip_n_to_p (sys_id->rtr_id, ip_addr_str);
    snprintf ((char *)buffer, 32, "%s-%d", ip_addr_str, sys_id->pn_id);
    return buffer;
}

int
isis_system_id_compare (isis_system_id_t *sys_id1,
                                         isis_system_id_t *sys_id2) {

    if (sys_id1->rtr_id > sys_id2->rtr_id) return CMP_PREFERRED;
    if (sys_id1->rtr_id < sys_id2->rtr_id) return CMP_NOT_PREFERRED;
    if (sys_id1->pn_id > sys_id2->pn_id) return CMP_PREFERRED;
    if (sys_id1->pn_id < sys_id2->pn_id) return CMP_NOT_PREFERRED;
    return CMP_PREF_EQUAL;
}
