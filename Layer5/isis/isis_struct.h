#ifndef __ISIS_STRUCT__
#define __ISIS_STRUCT__

#include <stdint.h>

#pragma pack (push,1)

typedef struct isis_system_id_ {

    uint32_t rtr_id;
     uint8_t pn_id;
} isis_system_id_t;

typedef struct isis_lan_id_ {

    uint32_t rtr_id;
    uint8_t pn_id;
}isis_lan_id_t;

typedef struct isis_lsp_id_ {

    isis_lan_id_t sys_id;
    uint8_t fragment;
}isis_lsp_id_t;

typedef enum ISIS_LVL_ {

    isis_level_1 = 1,
    isis_level_2 = 2,
    isis_level_12 = 3
} ISIS_LVL;
#pragma pack(pop)

#endif