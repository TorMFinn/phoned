const std = @import("std");
const fs = std.fs;

const ModemError = error{
    DeviceNotFound,
    PortNotOpen,
    ATParseError,
};

const Modem = struct {
    file_handle: std.fs.File,
    allocator: std.mem.Allocator,

    pub const Event = enum { incomming_call, hangup };
    pub const Command = enum { answer, hangup, start_audio, stop_audio, reset };

    pub const Config = struct {
        device_path: []const u8,
        baudrate: u32,
    };

    pub fn init(alloc: std.mem.Allocator, cfg: Config) !Modem {
        var file_handle = try fs.openFile(cfg.device_path);
        return Modem{ .file_handle = file_handle, .alloc = alloc };
    }

    pub fn dial_number(number: []const u8) !void {
        var buf: [256]u8 = undefined;
        var cmd = try std.fmt.bufPrint(&buf, "ATD{s};\r\n", .{number});
        _ = cmd;
    }

    fn handle_incomming_at_command(at_cmd: []const u8) !Event {
        _ = at_cmd;
        return ModemError.ATParseError;
    }
};

const ModemPortHandler = struct {
    file_handle: std.fs.File,
};

test "AT Command gives proper ModemEvent" {
    const incomming_call_at = "RING\r\n";
    try std.testing.expectEqual(Modem.Event.incomming_call, try Modem.handle_incomming_at_command(incomming_call_at));
}

test "dial number gives correct cmd" {
    try Modem.dial_number("51724910");
}
