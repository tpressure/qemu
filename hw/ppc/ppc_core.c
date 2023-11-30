/*
 * Common PPC CPU core abstraction
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

#include "hw/ppc/ppc_core.h"
#include "qapi/error.h"
#include "qapi/visitor.h"

static void powerpc_core_prop_get_core_id(Object *obj, Visitor *v,
                                          const char *name, void *opaque,
                                          Error **errp)
{
    PowerPCCore *core = POWERPC_CORE(obj);
    int64_t value = core->core_id;

    visit_type_int(v, name, &value, errp);
}

static void powerpc_core_prop_set_core_id(Object *obj, Visitor *v,
                                          const char *name, void *opaque,
                                          Error **errp)
{
    PowerPCCore *core = POWERPC_CORE(obj);
    int64_t value;

    if (!visit_type_int(v, name, &value, errp)) {
        return;
    }

    if (value < 0) {
        error_setg(errp, "Invalid core id %"PRId64, value);
        return;
    }

    core->core_id = value;
}

static void powerpc_core_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    object_class_property_add(oc, "core-id", "int",
                              powerpc_core_prop_get_core_id,
                              powerpc_core_prop_set_core_id,
                              NULL, NULL);
}

static const TypeInfo powerpc_core_type_info = {
    .name = TYPE_POWERPC_CORE,
    .parent = TYPE_CPU_CORE,
    .abstract = true,
    .class_init = powerpc_core_class_init,
    .instance_size = sizeof(PowerPCCore),
};

static void powerpc_core_register_types(void)
{
    type_register_static(&powerpc_core_type_info);
}

type_init(powerpc_core_register_types)
