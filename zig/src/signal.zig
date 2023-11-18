const std = @import("std");

pub fn Signal(comptime Data: type) type {
    return struct {
        const This = @This();

        callback: fn (*Data) void,

        pub fn init(allocator: std.mem.Allocator) This {
            _ = allocator;
        }

        pub fn connect(this: *This, comptime callback: fn (*Data) void) !void {
            this.callback = callback;
        }

        pub fn emit(this: *This, data: Data) !void {
            this.callback(data);
        }
    };
}

test "simple_signal" {

}