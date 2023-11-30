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

static void powerpc_core_realize(DeviceState *dev, Error **errp)
{
    CPUCore *core = CPU_CORE(dev);
    PowerPCCoreClass *ppc_class = POWERPC_CORE_GET_CLASS(dev);

    if (core->plugged_threads != -1 &&
        core->nr_threads != core->plugged_threads) {
        error_setg(errp, "nr_threads and plugged-threads must be equal");
        return;
    }

    ppc_class->parent_realize(dev, errp);
}

static void powerpc_core_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    PowerPCCoreClass *ppc_class = POWERPC_CORE_CLASS(oc);

    /*
     * PPC cores support hotplug and must be created after
     * qemu_init_board().
     */
    clear_bit(DEVICE_CATEGORY_CPU_DEF, dc->categories);
    object_class_property_add(oc, "core-id", "int",
                              powerpc_core_prop_get_core_id,
                              powerpc_core_prop_set_core_id,
                              NULL, NULL);
    device_class_set_parent_realize(dc, powerpc_core_realize,
                                    &ppc_class->parent_realize);
}

static const TypeInfo powerpc_core_type_info = {
    .name = TYPE_POWERPC_CORE,
    .parent = TYPE_CPU_CORE,
    .abstract = true,
    .class_size = sizeof(PowerPCCoreClass),
    .class_init = powerpc_core_class_init,
    .instance_size = sizeof(PowerPCCore),
};

static void powerpc_core_register_types(void)
{
    type_register_static(&powerpc_core_type_info);
}

type_init(powerpc_core_register_types)
