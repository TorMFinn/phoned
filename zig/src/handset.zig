const std = @import("std");
const log = std.log;

const c = @cImport({
    @cInclude("linux/gpio.h");
});

const HandsetError = error{
    GpioNotAvailable,
};

const Handset = struct {
    gpio_fd: std.fs.File.Handle,
    alloctor: std.mem.Allocator,
    read_thd: std.Thread,
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
        var current_state = try init_gpio(gpio_fd);

        return Handset {
            .allocator = allocator,
            .current_state = current_state,
            .gpio_fd = gpio_fd,
            .read_thd = std.Thread,
        };
    }

    pub fn cleanup(self: *Handset) void {
        _ = self;
    }

    fn init_gpio(gpio_fd: std.fs.File.Handle) !Handset.State {
        var req = std.mem.zeroInit(c.gpioevent_request, .{});
        var data = std.mem.zeroInit(c.gpiohandle_data, .{});

        req.lineoffset = handset_gpio_num;
        req.handleflags = c.GPIOHANDLE_REQUEST_INPUT;
        req.eventflags = c.GPIOEVENT_REQUEST_BOTH_EDGES;

        if (std.os.linux.ioctl(gpio_fd, c.GPIO_GET_LINEEVENT_IOCTL, &req) < 0) {
            log.err("failed to open gpioline", .{});
            return error.GpioNotAvailable;
        }

        // Read the initial state
        if (std.os.linux.ioctl(req.fd, c.GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data)) {
            log.err("failed to read initial gpio state", {});
            return error.GpioNotAvailable;
        }

        return Handset.State(data.values[0]);
    }
};
