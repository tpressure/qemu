/*
 * CPU book abstract device
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

#include "qemu/osdep.h"
#include "hw/cpu/book.h"

static void cpu_book_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    CPUTopoClass *tc = CPU_TOPO_CLASS(oc);

    set_bit(DEVICE_CATEGORY_CPU_DEF, dc->categories);

    tc->level = CPU_TOPO_BOOK;
}

static const TypeInfo cpu_book_type_info = {
    .name = TYPE_CPU_BOOK,
    .parent = TYPE_CPU_TOPO,
    .class_init = cpu_book_class_init,
    .instance_size = sizeof(CPUBook),
};

static void cpu_book_register_types(void)
{
    type_register_static(&cpu_book_type_info);
}

type_init(cpu_book_register_types)
