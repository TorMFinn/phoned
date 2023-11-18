//! This module implements a SimpleConfig file parser.
//! SimpleConfig works just as a INI file and can have
//! integers, strings, bools and floats as valid value types.
//!
//! Entries are structured as a key value pair and they are seperated by a space.
//! A new entry is terminated by a new line.

const std = @import("std");

pub const Value = union(enum) {
    None,
    String: []const u8,
    Integer: i64,
    Float: f64,
    Bool: bool,
};

pub const Entry = struct {
    key: []const u8,
    value: Value,
};

pub const ParseError = error{InvalidEntry};

const Parser = struct {
    fn parse_value(allocator: std.mem.Allocator, value_str: []const u8) !?Value {
        // Remove any spaces
        var trimmed = std.mem.trim(u8, value_str, " ");

        // Check if this is a bool.
        if (std.mem.eql(u8, trimmed, "true")) {
            return Value{ .Bool = true };
        }

        if (std.mem.eql(u8, trimmed, "false")) {
            return Value{ .Bool = false };
        }

        // Is it a string?
        if (trimmed[0] == '"' and trimmed[trimmed.len - 1] == '"') {
            var str = try allocator.dupe(u8, trimmed[1 .. trimmed.len - 1]);
            return Value{ .String = str };
        }

        // Is it a float?
        var comma_idx = std.mem.indexOf(u8, trimmed, ".");
        if (comma_idx != null) {
            return Value{ .Float = try std.fmt.parseFloat(f64, trimmed) };
        }

        // Must be an int, else it's invalid.
        return Value{ .Integer = try std.fmt.parseInt(i64, trimmed, 10) };
    }

    pub fn parse_line(allocator: std.mem.Allocator, line: []const u8) ParseError!?Entry {
        // Remove any space on the left side of config entry.
        var cfg_str = std.mem.trimLeft(u8, line, " ");

        // If the line is empty, then return a Error.
        if (cfg_str.len == 0) {
            return null;
        }

        // Ignore any comments
        if (cfg_str[0] == '#') {
            return null;
        }

        // Find the first space.
        var seperator_idx = std.mem.indexOf(u8, cfg_str, " ") orelse {
            // Return a empty value but with a key.
            return Entry{ .key = cfg_str, .value = .None };
        };

        var key = cfg_str[0..seperator_idx];
        var value_str = cfg_str[seperator_idx..];

        var value_opt = parse_value(allocator, value_str) catch return ParseError.InvalidEntry;
        if (value_opt == null) return null;

        return Entry{ .key = key, .value = value_opt.? };
    }
};

pub const SimpleConfig = struct {
    const Self = @This();

    allocator: std.mem.Allocator,
    entries: std.StringHashMap(Value),

    fn init(allocator: std.mem.Allocator) Self {
        return Self{ .allocator = allocator, .entries = std.StringHashMap(Value).init(allocator) };
    }

    pub fn cleanup(self: *Self) void {
        var key_it = self.entries.keyIterator();
        while (key_it.next()) |key| {

            // If the value is a string, we must free that as well.
            var value = self.entries.get(key.*) orelse unreachable;
            if (value == Value.String) {
                self.allocator.free(value.String);
            }

            self.allocator.free(key.*);
        }
        self.entries.deinit();
    }

    pub fn load_from_file(self: *Self, file_path: []const u8) !void {
        var file = try std.fs.cwd().openFile(file_path, .{});
        defer file.close();

        var reader = file.reader();

        var line_buf: [256]u8 = undefined;
        while (try reader.readUntilDelimiterOrEof(&line_buf, '\n')) |line| {
            var entry = try Parser.parse_line(self.allocator, line) orelse continue;
            var key = try self.allocator.dupe(u8, entry.key);
            try self.entries.put(key, entry.value);
        }
    }

    pub fn get(self: *Self, key: []const u8) ?Value {
        return self.entries.get(key);
    }
};

test "can parse values" {
    const test_file = "src/testing/simple_config.conf";
    var allocator = std.testing.allocator;

    var config = SimpleConfig.init(allocator);
    defer config.cleanup();

    try config.load_from_file(test_file);

    var true_bool = config.get("true_bool") orelse unreachable;
    try std.testing.expect(true_bool.Bool == true);

    var false_bool = config.get("false_bool") orelse unreachable;
    try std.testing.expect(false_bool.Bool == false);

    var my_string = config.get("my_string") orelse unreachable;
    try std.testing.expect(std.mem.eql(u8, my_string.String, "this is a string"));

    var pos_int = config.get("pos_int") orelse unreachable;
    try std.testing.expect(pos_int.Integer == 1234);

    var important_key = config.get("important_key") orelse unreachable;
    try std.testing.expect(important_key == Value.None);
}

test "can parse badly formatted config file" {
    const test_file = "src/testing/simple_config_bad_fmt.conf";
    var allocator = std.testing.allocator;

    var config = SimpleConfig.init(allocator);
    defer config.cleanup();

    try config.load_from_file(test_file);

    var true_bool = config.get("true_bool") orelse unreachable;
    try std.testing.expect(true_bool.Bool == true);

    var false_bool = config.get("false_bool") orelse unreachable;
    try std.testing.expect(false_bool.Bool == false);

    var my_string = config.get("my_string") orelse unreachable;
    try std.testing.expect(std.mem.eql(u8, my_string.String, "this is a string"));

    var pos_int = config.get("pos_int") orelse unreachable;
    try std.testing.expect(pos_int.Integer == 1234);

    var important_key = config.get("important_key") orelse unreachable;
    try std.testing.expect(important_key == Value.None);
}