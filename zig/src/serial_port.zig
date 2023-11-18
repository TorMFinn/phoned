const std = @import("std");

const c = @cImport({
    @cInclude("termios.h");
    @cInclude("unistd.h");
});

pub const SerialPortError = error{
    NoSuchDevice,
    ConfigureError,
};

const SerialPort = struct {
    file_handle: std.fs.File,

    pub fn init(device_path: []const u8, baudrate: u32) SerialPortError!SerialPort {
        var file_handle = std.fs.openFileAbsolute(device_path, .{ .mode = .read_write }) catch return SerialPortError.NoSuchDevice;
        try configure_port(file_handle, baudrate);
        return SerialPort{ .file_handle = file_handle };
    }

    pub fn cleanup(self: *SerialPort) void {
        self.file_handle.close();
    }

    fn configure_port(file_handle: std.fs.File, baudrate: u32) SerialPortError!void {
        var termios = std.mem.zeroInit(c.termios, .{});
        c.cfmakeraw(&termios);
        c.cfsetspeed(&termios, baudrate);
        if (c.tcsetattr(file_handle.handle, c.TCSANOW, &termios) < 0) {
            return SerialPortError.ConfigureError;
        }
    }
};

test "can open dummy port" {
    
}
