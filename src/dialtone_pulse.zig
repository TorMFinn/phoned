const std = @import("std");

const c = @cImport({
    @cInclude("pulse/simple.h");
    @cInclude("pulse/error.h");
});

const m2pi: f32 = 2 * std.math.pi;
const bufsize = 2048;

pub const Dialtone = struct {
    pa_simple: *c.pa_simple = undefined,
    audio_thd: ?std.Thread = null,
    preset: Preset,
    enable_tone: bool = false,
    mutex: std.Thread.Mutex,

    /// What kind of dialtone to use.
    /// Essentially, this is the frequency.
    const Preset = enum(u32) {
        Norway = 420,
    };

    pub fn init(preset: Dialtone.Preset) !Dialtone {
        var instance = Dialtone{ .preset = preset, .mutex = std.Thread.Mutex{} };
        try instance.open_sound();
        return instance;
    }

    fn open_sound(self: *Dialtone) !void {
        var spec = std.mem.zeroInit(c.pa_sample_spec, .{});
        spec.channels = 1;
        spec.format = c.PA_SAMPLE_S16LE;
        spec.rate = 8000;

        var err: c_int = 0;
        self.pa_simple = c.pa_simple_new(null, "phoned_audio", c.PA_STREAM_PLAYBACK, null, "dialtone", &spec, null, null, &err) orelse undefined;
        std.debug.print("initialized pulseaudio: {*}\n", .{self.pa_simple});
    }

    fn write_audio_func(self: *Dialtone) void {
        var audio_buf: [bufsize]i16 = undefined;
        const step = (m2pi) / 8000.0;
        var time: f32 = 0;

        while (self.enable_tone) {
            for (&audio_buf) |*value| {
                // value.* = @intFromFloat(32767 * std.math.sin(time * @as(f32, 420)));
                value.* = @intFromFloat(12767 * std.math.sin(time * @as(f32, 420)));
                time += step;
                if (time >= m2pi) {
                    time -= m2pi;
                }
            }
            if (c.pa_simple_write(self.pa_simple, &audio_buf, bufsize * 2, null) < 0) {
                std.debug.print("failed to write audio\n", .{});
            }
        }
        if (c.pa_simple_flush(self.pa_simple, null) < 0) {
            std.debug.print("failed to flush\n", .{});
        }
    }

    pub fn start(self: *Dialtone) !void {
        self.mutex.lock();
        defer self.mutex.unlock();

        if (self.enable_tone) {
            return;
        }

        self.enable_tone = true;
        self.audio_thd = try std.Thread.spawn(.{}, write_audio_func, .{self});
    }

    pub fn stop(self: *Dialtone) void {
        self.mutex.lock();
        defer self.mutex.unlock();

        self.enable_tone = false;
        if (self.audio_thd) |handle| {
            handle.join();
        }
    }
};
