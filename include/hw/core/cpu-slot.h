/*
 * CPU slot device abstraction
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

#ifndef CPU_SLOT_H
#define CPU_SLOT_H

#include "hw/core/cpu-topo.h"
#include "hw/cpu/core.h"
#include "hw/qdev-core.h"

#define TYPE_CPU_SLOT "cpu-slot"

OBJECT_DECLARE_SIMPLE_TYPE(CPUSlot, CPU_SLOT)

/**
 * CPUSlot:
 * @cores: Queue consisting of all the cores in the topology tree
 *     where the cpu-slot is the root. cpu-slot can maintain similar
 *     queues for other topology levels to facilitate traversal
 *     when necessary.
 */
struct CPUSlot {
    /*< private >*/
    CPUTopoState parent_obj;

    /*< public >*/
    QTAILQ_HEAD(, CPUCore) cores;
};

#endif /* CPU_SLOT_H */
