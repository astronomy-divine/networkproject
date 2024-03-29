/*
 * =====================================================================================
 *
 *       Filename:  uapi_mm.h
 *
 *    Description:  This Header file ocntains public APIs to be used by the application
 *
 *        Version:  1.0
 *        Created:  02/01/2020 10:00:27 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Er. Abhishek Sagar, Juniper Networks (https://csepracticals.com), sachinites@gmail.com
 *        Company:  Juniper Networks
 *
 *        This file is part of the Linux Memory Manager distribution (https://github.com/sachinites) 
 *        Copyright (c) 2019 Abhishek Sagar.
 *        This program is free software: you can redistribute it and/or modify it under the terms of the GNU General 
 *        Public License as published by the Free Software Foundation, version 3.
 *        
 *        This program is distributed in the hope that it will be useful, but
 *        WITHOUT ANY WARRANTY; without even the implied warranty of
 *        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *        General Public License for more details.
 *
 *        visit website : https://csepracticals.com for more courses and projects
 *                                  
 * =====================================================================================
 */

#ifndef __UAPI_MM__
#define __UAPI_MM__

#include <stdint.h>
typedef struct mm_instance_ mm_instance_t;

void *
xcalloc(mm_instance_t *mm_inst, char *struct_name, int units);
void *
xcalloc_buff(mm_instance_t *mm_inst, uint32_t bytes) ;
void
xfree(void *app_ptr);
void
xfree_inst(mm_instance_t *mm_inst, void *app_ptr);

/*Printing Functions*/
void mm_print_memory_usage(mm_instance_t *mm_inst,  unsigned char *struct_name);
void mm_print_block_usage(mm_instance_t *mm_inst);
void mm_print_registered_page_families(mm_instance_t *mm_inst);
void mm_print_variable_buffers(mm_instance_t *mm_inst);

/*Initialization Functions*/
void
mm_init();

mm_instance_t *
mm_init_new_instance();

/*Registration function*/
void
mm_instantiate_new_page_family(
        mm_instance_t *mm_inst,
        const char *struct_name,
        uint32_t struct_size);

#define XCALLOC(mm_inst, units, struct_name) \
    (xcalloc(mm_inst, #struct_name, units))

#define XCALLOC_BUFF(mm_inst, size_in_bytes) \
    (xcalloc_buff(mm_inst, size_in_bytes) )

#define MM_REG_STRUCT(mm_inst, struct_name)  \
    (mm_instantiate_new_page_family(mm_inst, #struct_name, sizeof(struct_name)))

#define XFREE(ptr)  \
   xfree(ptr)

#define XFREE_INST(mm_inst, ptr)    \
    xfree_inst(mm_inst, ptr);

#endif /* __UAPI_MM__ */
