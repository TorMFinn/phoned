const std = @import("std");
const log = std.log;
const c = @cImport({
    @cInclude("SDL2/SDL.h");
});

const allocator = std.heap.page_allocator;

pub const DialtoneError = error{ SDLInitError, NoHardwareAvailable };

const SineData = struct { step: f32, frequency: f32, time: f32 };

const sample_rate: f32 = 8000;
const sample_size = 4096;

fn audio_callback(userdata: ?*anyopaque, stream: [*c]u8, len: c_int) callconv(.C) void {
    if (userdata) |data| {
        var sine_data: *SineData = @ptrCast(@alignCast(data));

        var buf: [*c]u16 = @ptrCast(@alignCast(stream));
        var i: usize = 0;

        log.info("sine data: {}", .{sine_data});

        while (i < @divTrunc(len, 2)) : (i += 1) {
            @setRuntimeSafety(false);
            var sample: f32 = 15000 * std.math.sin(sine_data.time * 440);
            buf[i] = @intFromFloat(sample);
            sine_data.time += sine_data.step;
            if (sine_data.time >= std.math.pi * 2) {
                sine_data.time -= std.math.pi * 2;
            }
        }
    }
    log.info("Callback", .{});
}

pub const Dialtone = struct {
    /// Device handle returned by SDL:
    dev_handle: c.SDL_AudioDeviceID,

    //// Data containing info on how the generate the sine wave.
    sine_data: SineData,

    //// Config describing the type of dialtone.
    pub const Config = struct {
        tone_frequency: f32 = 420, // Default is the European dialtone.
    };

    pub fn init(config: Config) !Dialtone {
        if (c.SDL_Init(c.SDL_INIT_AUDIO) < 0) {
            log.err("failed to init SDL: {s}", .{c.SDL_GetError()});
            return DialtoneError.SDLInitError;
        }

        const sine_data = SineData{ .step = (2 * 3.14) / sample_rate, .frequency = config.tone_frequency, .time = 0 };
        return Dialtone{ .dev_handle = 0, .sine_data = sine_data };
    }

    pub fn deinit(self: *Dialtone) void {
        self.stop();
        c.SDL_CloseAudioDevice(self.dev_handle);
    }

    pub fn start(self: *Dialtone) !void {
        log.info("starting playback", .{});
        if (self.dev_handle == 0) {
            try self.open_sound_device();
        }
        c.SDL_PauseAudioDevice(self.dev_handle, 0);
    }

    pub fn stop(self: *Dialtone) void {
        log.info("stopping audio", .{});
        c.SDL_PauseAudioDevice(self.dev_handle, 1);
    }

    fn open_sound_device(self: *Dialtone) !void {
        var desired_spec = std.mem.zeroInit(c.SDL_AudioSpec, .{});
        var obtained_spec = std.mem.zeroInit(c.SDL_AudioSpec, .{});

        desired_spec.channels = 1; // The phone only has one speaker.
        desired_spec.format = c.AUDIO_S16LSB;
        desired_spec.freq = sample_rate;
        desired_spec.samples = sample_size;
        desired_spec.callback = audio_callback;
        desired_spec.userdata = &self.sine_data;

        self.dev_handle = c.SDL_OpenAudioDevice(null, 0, &desired_spec, &obtained_spec, 0);
        log.info("got dev handle: {d}", .{self.dev_handle});
        if (self.dev_handle < 0) {
            log.err("failed to open sound device: {s}", .{c.SDL_GetError()});
            return DialtoneError.NoHardwareAvailable;
        }
    }
};
