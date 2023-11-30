/*
 * Common PPC CPU core abstraction header
 *
 * Copyright (c) 2023 Intel Corporation
 * Author: Zhao Liu <zhao1.liu@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HW_PPC_CORE_H
#define HW_PPC_CORE_H

#include "hw/cpu/core.h"
#include "hw/qdev-core.h"
#include "qom/object.h"

#define TYPE_POWERPC_CORE "powerpc-core"

OBJECT_DECLARE_TYPE(PowerPCCore, PowerPCCoreClass, POWERPC_CORE)

struct PowerPCCoreClass {
    /*< private >*/
    CPUCoreClass parent_class;

    /*< public >*/
    DeviceRealize parent_realize;
};

struct PowerPCCore {
    /*< private >*/
    CPUCore parent_obj;

    /*< public >*/
    /*
     * The system-wide id for core, not the sub core-id within the
     * parent container (which is above the core level).
     */
    int core_id;
};

/*
 * Note: topology field names need to be kept in sync with
 * 'CpuInstanceProperties'
 */
#define POWERPC_CORE_PROP_CORE_ID "core-id"

#endif /* HW_PPC_CORE_H */
