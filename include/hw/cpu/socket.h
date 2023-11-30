/*
 * CPU socket abstract device
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

#ifndef HW_CPU_SOCKET_H
#define HW_CPU_SOCKET_H

#include "hw/core/cpu-topo.h"
#include "hw/qdev-core.h"

#define TYPE_CPU_SOCKET "cpu-socket"

OBJECT_DECLARE_SIMPLE_TYPE(CPUSocket, CPU_SOCKET)

struct CPUSocket {
    /*< private >*/
    CPUTopoState parent_obj;

    /*< public >*/
};

#endif /* HW_CPU_SOCKET_H */
