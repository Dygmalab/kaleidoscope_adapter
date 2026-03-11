/* BaseDygma - Dygma base plugin for Kaleidoscope
 * Copyright (C) 2026 Dygma Lab S.L. www.dygma.com
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "BaseDygma.h"
#include "kaleidoscope/Runtime.h"

namespace kaleidoscope
{
namespace plugin
{
    BaseDygma::BaseDygma(void)
    {
    }

    kaleidoscope::EventHandlerResult BaseDygma::beforeReportingState(void)
    {
        if ( key_report_enabled == false )
        {
            Runtime.device().hid().keyboard().releaseAllKeys();
        }

        return kaleidoscope::EventHandlerResult::OK;
    }

    result_t BaseDygma::key_report_enable( void )
    {
        key_report_enabled = true;

        return RESULT_OK;
    }

    result_t BaseDygma::key_report_disable( void )
    {
        key_report_enabled = false;

        return RESULT_OK;
    }

} // namespace plugin
} //  namespace kaleidoscope

kaleidoscope::plugin::BaseDygma BaseDygma;
