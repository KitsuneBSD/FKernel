-- Configuration surface — every knob the user can tune lives here.
-- Rule: one named option per knob, never raw CONFIG_* defines.

option("initrd_mode")
    set_default("busybox")
    set_values("busybox", "openrc")
    set_description("Select initrd system style")
option_end()
