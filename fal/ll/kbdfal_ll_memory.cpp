/*
 * kbdfal_ll_memory.h - This file contains sources for configuring and enabling
 *                      kaleidoscope_adapter long-term memory.
 *
 * Copyright (C) 2025 Dygma Lab S.L. www.dygma.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "kbdfal_ll_memory.h"

typedef struct
{
    /* Callbacks */
    void * p_instance;
    kbdmem_item_request_cb item_request_cb;
    kbdmem_data_save_cb data_save_cb;
} kbdfal_mem_t;

static kbdfal_mem_t kbdfal_mem;

static INLINE result_t _init( kbdfal_mem_t * p_mem, const kbdmem_config_t * p_config )
{
    /* Save the callback */
    p_mem->p_instance = p_config->p_instance;
    p_mem->item_request_cb = p_config->item_request_cb;
    p_mem->data_save_cb = p_config->data_save_cb;

    return RESULT_OK;
}

static result_t _item_request( kbdfal_mem_t * p_mem, kbdmem_item_type_t item_type, const void ** pp_item )
{
    if( p_mem->item_request_cb == NULL )
    {
        return RESULT_ERR;
    }

    return p_mem->item_request_cb( p_mem->p_instance, item_type, pp_item );
}

static result_t _data_save( kbdfal_mem_t * p_mem, const void * p_mem_target, const void * p_data, uint16_t data_len )
{
    if( p_mem->data_save_cb == NULL )
    {
        return RESULT_ERR;
    }

    return p_mem->data_save_cb( p_mem->p_instance, p_mem_target, p_data, data_len );
}

/*********************************************************/
/*                          API                          */
/*********************************************************/

result_t kbdfal_ll_memory_init( const kbdmem_config_t * p_config )
{
    return _init( &kbdfal_mem, p_config );
}

result_t kbdfal_ll_memory_item_request( kbdmem_item_type_t item_type, const void ** pp_item )
{
    return _item_request( &kbdfal_mem, item_type, pp_item );
}

result_t kbdfal_ll_memory_data_save( const void * p_mem_target, const void * p_data, uint16_t data_len )
{
    return _data_save( &kbdfal_mem, p_mem_target, p_data, data_len );
}
