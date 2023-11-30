const std = @import("std");
const log = std.log;

//const audio = @import("audio.zig");
const audio = @import("dialtone_pulse.zig");

pub fn main() !void {
    //var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    var dialtone = try audio.Dialtone.init(.Norway);

    dialtone.stop();
    try dialtone.start();
    std.time.sleep(std.time.ns_per_s * 2);


    std.debug.print("Stopped the dialtone\n", .{});
    std.time.sleep(std.time.ns_per_s * 2);

    try dialtone.start();
    std.time.sleep(std.time.ns_per_s * 2);

    //var handset = try hw.handset.Handset.init(gpa.allocator());
    //defer handset.cleanup();

    //var ringer = hw.Ringer.init(.{.device_path = "/dev/gpiochip0"}) catch |err| {
    //log.err("failed to init ringer: {}", .{err});
    //return;
    //};
    //defer ringer.deinit();

    //try dialtone.start();
}
