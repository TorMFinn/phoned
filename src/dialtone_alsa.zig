const std = @import("std");

const c = @cImport({
    @cInclude("asoundlib.h");
});

pub const Dialtone = struct {
    const Self = @This();

    /// What kind of dialtone to use.
    /// Essentially, this is the frequency.
    const Preset = enum(u32) {
        Norway = 420,
    };

    pub fn init(preset: Dialtone.Preset) Self {
        _ = preset;
    }

    pub fn open_sound(self: Dialtone) !void {
        // Always use the default device.
        const device = "plughw:0,0";
        const rate = 8000;
        const channels = 1;



    }

    fn generate_sine()

};