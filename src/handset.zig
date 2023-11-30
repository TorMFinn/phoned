const std = @import("std");
const log = std.log;

const c = @cImport({
    @cInclude("linux/gpio.h");
});

const HandsetError = error{
    GpioNotAvailable,
};

pub const Handset = struct {
    gpio_fd: std.fs.File.Handle,
    line_handle: std.fs.File.Handle,
    allocator: std.mem.Allocator,
    read_thd: ?std.Thread = null,
    quit_thd: bool = false,
    current_state: State,

    const State = enum {
        lifted,
        down,
    };

    /// The gpio line number of where the handset is connected.
    const handset_gpio_num = 13;

    /// Creates a new Handset object and initializes it
    /// with the the given gpio_pin.
    pub fn init(allocator: std.mem.Allocator) !Handset {
        // Always use gpiochip0
        var gpio_fd = try std.fs.openFileAbsolute("/dev/gpiochip0", .{ .mode = .read_write });

        var instance = Handset{
            .allocator = allocator,
            .current_state = current_state,
            .gpio_fd = gpio_fd.handle,
        };

        try instance.init_gpio();

        return instance;
    }

    pub fn cleanup(self: *Handset) void {
        if (self.read_thd != null) {
            self.quit_thd = true;
            self.read_thd.?.join();
        }
    }

    pub fn current_state(self: *Handset) Handset.State {
        // Read the initial state
        self.current_state;
    }

    fn start_thread(self: *Handset) !void {
        while (!self.quit_thd) {}
    }

    fn init_gpio(self: *Handset) !void {
        var req = std.mem.zeroInit(c.gpioevent_request, .{});
        var data = std.mem.zeroInit(c.gpiohandle_data, .{});

        req.lineoffset = handset_gpio_num;
        req.handleflags = c.GPIOHANDLE_REQUEST_INPUT;
        req.eventflags = c.GPIOEVENT_REQUEST_BOTH_EDGES;

        if (std.os.linux.ioctl(self.gpio_fd, c.GPIO_GET_LINEEVENT_IOCTL, @intFromPtr(&req)) < 0) {
            log.err("failed to open gpioline", .{});
            return error.GpioNotAvailable;
        }

        if (std.os.linux.ioctl(req.fd, c.GPIOHANDLE_GET_LINE_VALUES_IOCTL, @intFromPtr(&data)) < 0) {
            log.err("failed to read initial gpio state", {});
            return error.GpioNotAvailable;
        }

        self.current_state = @enumFromInt(data.values[0]);
    }
};
