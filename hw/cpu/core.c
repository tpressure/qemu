/*
 * CPU core abstract device
 *
 * Copyright (C) 2016 Bharata B Rao <bharata@linux.vnet.ibm.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"

#include "hw/boards.h"
#include "hw/cpu/core.h"
#include "qapi/error.h"
#include "qapi/visitor.h"

static void core_prop_get_nr_threads(Object *obj, Visitor *v, const char *name,
                                     void *opaque, Error **errp)
{
    CPUCore *core = CPU_CORE(obj);
    int64_t value = core->nr_threads;

    visit_type_int(v, name, &value, errp);
}

static void core_prop_set_nr_threads(Object *obj, Visitor *v, const char *name,
                                     void *opaque, Error **errp)
{
    CPUCore *core = CPU_CORE(obj);
    CPUTopoState *topo = CPU_TOPO(obj);
    int64_t value;

    if (!visit_type_int(v, name, &value, errp)) {
        return;
    }

    core->nr_threads = value;
    topo->max_children = core->nr_threads;
}

static void core_prop_set_plugged_threads(Object *obj, Visitor *v,
                                          const char *name, void *opaque,
                                          Error **errp)
{
    CPUCore *core = CPU_CORE(obj);
    int64_t value;

    if (!visit_type_int(v, name, &value, errp)) {
        return;
    }

    core->plugged_threads = value;
}

static void cpu_core_instance_init(Object *obj)
{
    CPUCore *core = CPU_CORE(obj);
    CPUTopoState *topo = CPU_TOPO(obj);

    /*
     * Only '-device something-cpu-core,help' can get us there before
     * the machine has been created. We don't care to set nr_threads
     * in this case since it isn't used afterwards.
     */
    if (current_machine) {
        core->nr_threads = current_machine->smp.threads;
    }

    core->plugged_threads = -1;
    /* Core's child can only be the thread. */
    topo->child_level = CPU_TOPO_THREAD;
}

static void cpu_core_realize(DeviceState *dev, Error **errp)
{
    CPUCore *core = CPU_CORE(dev);
    CPUCoreClass *cc = CPU_CORE_GET_CLASS(core);

    if (core->plugged_threads > core->nr_threads) {
        error_setg(errp, "Plugged threads (plugged-threads: %d) must "
                   "not be more than max threads (nr-threads: %d)",
                   core->plugged_threads, core->nr_threads);
        return;
    } else if (core->plugged_threads == -1) {
        core->plugged_threads = core->nr_threads;
    }

    cc->parent_realize(dev, errp);
}

static void cpu_core_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    CPUTopoClass *tc = CPU_TOPO_CLASS(oc);
    CPUCoreClass *cc = CPU_CORE_CLASS(oc);

    set_bit(DEVICE_CATEGORY_CPU_DEF, dc->categories);
    object_class_property_add(oc, "nr-threads", "int", core_prop_get_nr_threads,
                              core_prop_set_nr_threads, NULL, NULL);
    object_class_property_add(oc, "plugged-threads", "int", NULL,
                              core_prop_set_plugged_threads, NULL, NULL);
    device_class_set_parent_realize(dc, cpu_core_realize, &cc->parent_realize);

    tc->level = CPU_TOPO_CORE;
}

static const TypeInfo cpu_core_type_info = {
    .name = TYPE_CPU_CORE,
    .parent = TYPE_CPU_TOPO,
    .class_init = cpu_core_class_init,
    .class_size = sizeof(CPUCoreClass),
    .instance_size = sizeof(CPUCore),
    .instance_init = cpu_core_instance_init,
};

static void cpu_core_register_types(void)
{
    type_register_static(&cpu_core_type_info);
}

type_init(cpu_core_register_types)
