const std = @import("std");
const log = std.log;
const linux = std.os.linux;

const c = @cImport(@cInclude("linux/gpio.h"));

const GPIO_RINGER_PIN = 26;

const RingerError = error{DeviceInitError, SetValueError};

pub const Ringer = struct {
    gpio_handle: std.fs.File.Handle,
    gpio_thd: std.Thread = undefined,

    /// The current state of the GPIO pin.
    current_state: bool = false,

    /// Controls wether the thread should continue.
    do_ringing: bool = false,

    pub const Config = struct {
        device_path: []const u8,
    };

    pub fn init(cfg: Config) !Ringer {
        var file = try std.fs.openFileAbsolute(cfg.device_path, .{});

        var request = std.mem.zeroInit(c.gpiohandle_request, .{});
        request.lineoffsets[0] = GPIO_RINGER_PIN;
        request.lines = 1;
        request.flags = c.GPIOHANDLE_REQUEST_OUTPUT;

        if (linux.ioctl(file.handle, c.GPIO_GET_LINEHANDLE_IOCTL, @intFromPtr(&request)) < 0) {
            log.err("failed to get ioctl for linehandle", .{});
            return RingerError.DeviceInitError;
        }

        var instance = Ringer{ .gpio_handle = file.handle };

        instance.set_output_state(false) catch {
            file.close();
        };

        return instance;
    }

    fn set_output_state(self: *Ringer, state: bool) !void {
        var data = std.mem.zeroInit(c.gpiohandle_data, .{});
        data.values[0] = @intFromBool(state);
        if (linux.ioctl(self.gpio_handle, c.GPIOHANDLE_SET_LINE_VALUES_IOCTL, @intFromPtr(&data)) < 0) {
            return RingerError.SetValueError;
        }

        self.current_state = state;
    }

    fn toggle_output(self: *Ringer) void {
        self.set_output_state(!self.current_state);
    }

    pub fn deinit(self: *Ringer) void {
        // Maybe just let the file close this one?
        _ = linux.close(self.gpio_handle);
    }

    pub fn start(self: *Ringer) void {
        if (self.gpio_thd == undefined) {
            self.gpio_thd.spawn(.{}, self.gpio_thd_function, self);
        }
    }

    pub fn stop(self: *Ringer) void {
        _ = self;
    }

    fn gpio_thd_function(self: *Ringer) void {
        _ = self;
    }
};

