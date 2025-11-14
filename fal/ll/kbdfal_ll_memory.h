/*
 * kbdfal_ll_memory.h - This header file declares kaleidoscope_adapter low-level
 *                      long-term memory functions. Must make sure all functions
 *                      needed for keyboard_api configuration are declared here
 *                      as well as the internal functions used only in the
 *                      kaleidoscope_adapter space.
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

#ifndef __KBDFAL_LL_MEMORY_H_
#define __KBDFAL_LL_MEMORY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "kbd_memory.h"

    result_t kbdfal_ll_memory_init( const kbdmem_config_t * p_config );

    result_t kbdfal_ll_memory_item_request( kbdmem_item_type_t item_type, const void ** pp_item );
    result_t kbdfal_ll_memory_data_save( const void * p_mem_target, const void * p_data, uint16_t data_len );

#ifdef __cplusplus
}
#endif

#endif /* __KBDFAL_LL_MEMORY_H_ */
