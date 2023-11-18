const std = @import("std");
const log = std.log;

const audio = @import("audio.zig");
const hw = @import("hw.zig");

pub fn main() !void {
    var dialtone = try audio.Dialtone.init(.{});
    defer dialtone.deinit();

    //var ringer = hw.Ringer.init(.{.device_path = "/dev/gpiochip0"}) catch |err| {
        //log.err("failed to init ringer: {}", .{err});
        //return;
    //};
    //defer ringer.deinit();

    try dialtone.start();
    std.time.sleep(std.time.ns_per_s * 2);
}
