const std = @import("std");
const log = std.log;

const audio = @import("dialtone_pulse.zig");
const handset = @import("handset.zig");

var dialtone: audio.Dialtone = undefined;

fn handset_state_handler(state: handset.Handset.State) void {
    if (state == handset.Handset.State.lifted) {
        std.debug.print("starting \n", .{});
        dialtone.start() catch unreachable;
    } else {
        std.debug.print("stopping\n", .{});
        dialtone.stop();
    }
}

pub fn main() !void {
    dialtone = try audio.Dialtone.init(.Norway);
    defer dialtone.stop();

    var handset_hw = try handset.Handset.init();
    defer handset_hw.cleanup();

    handset_hw.set_state_change_callback(&handset_state_handler);

    while (true) {
        std.time.sleep(std.time.ns_per_s * 1);
    }
}
