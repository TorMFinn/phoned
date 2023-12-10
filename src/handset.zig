const std = @import("std");
const log = std.log;

const c = @cImport({
    @cInclude("linux/gpio.h");
});

const HandsetError = error{
    GpioNotAvailable,
};

const StateChangeCallback = ?*const fn (Handset.State) void;

pub const Handset = struct {
    /// Gpio stuff.
    gpiochip_fd: std.fs.File.Handle,
    gpio_fd: c_int = undefined,

    read_thd: ?std.Thread = null,
    quit_thd: bool = false,
    current_state: State = State.down,

    /// Set a pointer to this function in order to get notified of state changes.
    state_change_callback: StateChangeCallback = null,

    pub const State = enum {
        down,
        lifted,
    };

    /// The gpio line number of where the handset is connected.
    const handset_gpio_num = 13;

    /// Creates a new Handset object and initializes it
    pub fn init() !Handset {
        // Always use gpiochip0
        const gpio_fd = try std.fs.openFileAbsolute("/dev/gpiochip0", .{ .mode = .read_write });

        var instance = Handset{
            .gpiochip_fd = gpio_fd.handle,
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

    pub fn set_state_change_callback(self: *Handset, callback: StateChangeCallback) void {
        self.state_change_callback = callback;
        self.read_thd = std.Thread.spawn(.{}, thread_func, .{self}) catch unreachable;
    }

    fn thread_func(self: *Handset) !void {
        var data = std.mem.zeroInit(c.gpiohandle_data, .{});
        while (!self.quit_thd) {
            std.time.sleep(std.time.ns_per_ms * 200);
            if (std.os.linux.ioctl(self.gpio_fd, c.GPIOHANDLE_GET_LINE_VALUES_IOCTL, @intFromPtr(&data)) < 0) {
                log.err("failed to read gpio state", {});
                return;
            }

            const new_state: State = @enumFromInt(data.values[0]);
            if (new_state != self.current_state) {
                std.debug.print("state is new", .{});
                if (self.state_change_callback) |handler| {
                    std.debug.print("calling handler\n", .{});
                    handler(new_state);
                }
            }
            self.current_state = new_state;
        }
    }

    fn init_gpio(self: *Handset) !void {
        var req = std.mem.zeroInit(c.gpioevent_request, .{});

        req.lineoffset = handset_gpio_num;
        req.handleflags = c.GPIOHANDLE_REQUEST_INPUT;
        req.eventflags = c.GPIOEVENT_REQUEST_BOTH_EDGES;

        if (std.os.linux.ioctl(self.gpiochip_fd, c.GPIO_GET_LINEEVENT_IOCTL, @intFromPtr(&req)) < 0) {
            log.err("failed to open gpioline", .{});
            return error.GpioNotAvailable;
        }

        self.gpio_fd = req.fd;
    }
};
