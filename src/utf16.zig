const std = @import("std");
const uni = std.unicode;
const expect = std.testing.expect;

pub export fn utf16_to_utf8(len: u64, utf16_str: [*]const u16, dest: [*]u8) u64 {

    //big case of "trust me bro" here, C-land needs to make sure both passed strings follows the proper bounds. As fat32 names are not null terminated.
    const utf16_slc = utf16_str[0..len];
    const dest_slc = dest[0..len];

    const res: u64 = uni.utf16leToUtf8(dest_slc, utf16_slc) catch std.math.maxInt(u64);
    return res;
}

export fn utf8_to_utf16(len: u64, utf8_str: [*]const u8, dest: [*]u16) u64 {
    const utf8_slc = utf8_str[0..len];
    const dest_slc = dest[0..len];

    const res: u64 = uni.utf8ToUtf16Le(dest_slc, utf8_slc) catch std.math.maxInt(u64);
    return res;
}

//create SFN representation, defaults to ~1.
pub export fn lfn_to_sfn_candidate(len: u64, lfn_str: [*]const u16, dest: [*]u8) i32 {
    //if (lfn_str[len] != 0) //where is the null boi
    //{}
    var u8_str: [256]u8 = std.mem.zeroes([256]u8);
    const res = utf16_to_utf8(len, lfn_str, &u8_str);
    if (res == std.math.maxInt(u64)) {
        return -1;
    }
    @memset(dest[0..11], 0x20); //Preemptively pad the output
    const ext_offset = find_extension_utf8(256, &u8_str);
    if (ext_offset != std.math.maxInt(u64) and ext_offset != len - 1) {
        const max_diff: u64 = @min(3, len - (ext_offset + 1));
        std.debug.print("max_diff calculated: {d}\n", .{max_diff});
        str_to_sfn_compliant(u8_str[ext_offset + 1 .. (ext_offset + 1 + max_diff)], dest[8..(8 + max_diff)]);
    }

    dest[6] = '~';
    dest[7] = '1';

    str_to_sfn_compliant(u8_str[0..@min(len, ext_offset)], dest[0..6]);

    return 1;
}

fn str_to_sfn_compliant(str: []const u8, dest: []u8) void {
    var offset: u64 = 0;
    for (dest) |*char| {
        string_traversal: while (offset < str.len) : (offset += 1) {
            const current_char = str[offset];
            std.debug.print("Selected char: {c}\n", .{current_char});

            switch (current_char) { // not optimized or simplified, but every case is sorted numerically by their code value.
                0 => {
                    std.debug.print("A lfn string with null value has been passed! check your null bounds in C :", .{});
                    for (str[0..offset]) |oops| {
                        std.debug.print("{c}", .{oops});
                    }
                    std.debug.panic("\n", .{});
                }, //ruh oh AKA someone fucked up the bounds check somewhere in C-land, they should not be visible here.

                1...32, 34 => continue :string_traversal, //Control characters, and spaces are skipped

                '!', '#'...')', '-' => {
                    char.* = current_char;
                    break :string_traversal;
                },

                '.' => continue :string_traversal,

                '0'...'9' => {
                    char.* = current_char;
                    break :string_traversal;
                },

                ':'...'?' => continue :string_traversal,

                '@'...'Z' => { //Upper Case Alphabetics and the @ sign does not need any modifications
                    char.* = current_char;
                    break :string_traversal;
                },

                '['...']' => // [/] is skipped
                continue :string_traversal,

                '^'...'`' => {
                    char.* = current_char;
                    break :string_traversal;
                },

                'a'...'z' => {
                    char.* = toUpperCase(current_char);
                    break :string_traversal;
                },

                '{', '~', '}' => {
                    char.* = current_char;
                    break :string_traversal;
                },

                '|' => continue :string_traversal,

                127...255 => //DEL and extended ASCII should be skipped
                continue :string_traversal,

                else => continue :string_traversal,
                //128...255 => { allowed by some specs, but DOS does not support this under NLS mode, and therefore is skipped.
                //    char.* = current_char;
                //    break string_traversal
                //        break :string_traversal;
                //},
            }
        }
        offset += 1;
    }
    std.debug.print("Final Str: ", .{});
    for (dest) |char| {
        std.debug.print("{c}", .{char});
    }
    std.debug.print("\n", .{});
}

pub export fn is_sfn_compliant_utf16(len: u64, string: [*]const u16) i32 {
    if (len > 11) return 0;

    var str_utf8: [11]u8 = [_]u8{0x20} ** 11;
    _ = utf16_to_utf8(len, string, @ptrCast(&str_utf8[0]));
    return is_sfn_compliant_utf8(len, @ptrCast(&str_utf8[0]));
}

pub export fn is_sfn_compliant_utf8(len: u64, string: [*]const u8) i32 {
    if (len > 11) return 0; // total length over 11 is not possible under SFN
    const offset = find_extension_utf8(len, string);
    if (offset == std.math.maxInt(u64)) {
        if (len > 8) return 0;
    } else {
        if (len - offset > 4) return 0;
    }

    var pos: i8 = -1;

    for (string[0..len]) |char| {
        pos += 1;
        switch (char) {
            0 => {
                std.debug.print("A lfn string with null value has been passed! check your null bounds in C :", .{});
                for (string[0..len]) |oops| {
                    std.debug.print("{c}", .{oops});
                }
                std.debug.panic("\n", .{});
            }, //ruh oh AKA someone fucked up the bounds check somewhere in C-land, they should not be visible here.

            1...32 | 34 => return 0, //Control characters, and spaces are skipped

            '!' | '#'...')' => continue,

            '.' => {
                if (pos != offset) return 0 else continue;
            },

            '0'...'9' => continue,

            ':'...'?' => return 0,

            '@'...'Z' => //Upper Case Alphabetics and the @ sign does not need any modifications
            continue,

            '['...']' => // [/] is skipped
            return 0,

            '^'...'`' => continue,

            'a'...'z' => return 0,

            '{', '~', '}' => continue,

            '|' => return 0,

            127...255 => //DEL and extended ASCII should be skipped
            return 0,

            else => return 0,

            //128...255 => { allowed by some specs, but DOS does not support this under NLS mode, and therefore is skipped.
            //    char.* = current_char;
            //    break string_traversal
            //        break :string_traversal;
            //},
        }
    }

    return 1;
}

inline fn toUpperCase(c: u8) u8 {
    return c - 32;
}

pub export fn find_extension_utf8(len: u64, str: [*]const u8) u64 {
    var count = len - 1;
    while (count > 0) : (count -= 1) {
        if (str[count] == '.') {
            return count;
        }
    }
    return std.math.maxInt(u64);
}

test "to Upper Case" {
    try expect(toUpperCase('c') == 'C');
    try expect(toUpperCase('a') == 'A');
    try expect(toUpperCase('b') == 'B');
    try expect(toUpperCase('e') == 'E');
}

test "LFN TO SFN no extension" {
    const lfn_str_u8 = "System Volume Information";

    var sfn_str: [11]u8 = [_]u8{0x20} ** 11;
    var lfn_str_u16: [lfn_str_u8.len]u16 = [_]u16{0x0020} ** lfn_str_u8.len;
    try std.testing.expect(utf8_to_utf16(lfn_str_u8.len, lfn_str_u8, &lfn_str_u16) != std.math.maxInt(u64));
    var it = std.unicode.Utf16LeIterator.init(&lfn_str_u16);
    while (try it.nextCodepoint()) |codepoint| {
        var buf: [4]u8 = [_]u8{0x0000} ** 4;
        const len = try std.unicode.utf8Encode(codepoint, &buf);
        std.debug.print("{s}", .{buf[0..len]});
    }
    std.debug.print("\n", .{});

    const res = find_extension_utf8(lfn_str_u8.len, lfn_str_u8);
    try std.testing.expect(res == std.math.maxInt(u64));

    _ = lfn_to_sfn_candidate(lfn_str_u8.len, &lfn_str_u16, &sfn_str);
    std.debug.print("{s}\n", .{sfn_str[0..10]});
    try std.testing.expect(std.mem.eql(u8, &sfn_str, "SYSTEM~1   "));
}

test "LFN TO SFN with extension" {
    const lfn_str_u8 = "lazy-lock.json";

    var sfn_str: [11]u8 = [_]u8{0x20} ** 11;
    var lfn_str_u16: [lfn_str_u8.len]u16 = [_]u16{0x0020} ** lfn_str_u8.len;
    try std.testing.expect(utf8_to_utf16(lfn_str_u8.len, lfn_str_u8, &lfn_str_u16) != std.math.maxInt(u64));
    var it = std.unicode.Utf16LeIterator.init(&lfn_str_u16);
    while (try it.nextCodepoint()) |codepoint| {
        var buf: [4]u8 = [_]u8{0x0000} ** 4;
        const len = try std.unicode.utf8Encode(codepoint, &buf);
        std.debug.print("{s}", .{buf[0..len]});
    }
    std.debug.print("\n", .{});

    const res = find_extension_utf8(lfn_str_u8.len, lfn_str_u8);
    try std.testing.expect(res != std.math.maxInt(u64));

    _ = lfn_to_sfn_candidate(lfn_str_u8.len, &lfn_str_u16, &sfn_str);
    std.debug.print("{s}\n", .{sfn_str[0..11]});
    try std.testing.expect(std.mem.eql(u8, &sfn_str, "LAZY-L~1JSO"));
}

test "LFN TO SFN with extension len < 3" {
    const lfn_str_u8 = "lazy-lock.js";

    var sfn_str: [11]u8 = [_]u8{0x20} ** 11;
    var lfn_str_u16: [lfn_str_u8.len]u16 = [_]u16{0x0020} ** lfn_str_u8.len;
    try std.testing.expect(utf8_to_utf16(lfn_str_u8.len, lfn_str_u8, &lfn_str_u16) != std.math.maxInt(u64));
    var it = std.unicode.Utf16LeIterator.init(&lfn_str_u16);
    while (try it.nextCodepoint()) |codepoint| {
        var buf: [4]u8 = [_]u8{0x0000} ** 4;
        const len = try std.unicode.utf8Encode(codepoint, &buf);
        std.debug.print("{s}", .{buf[0..len]});
    }
    std.debug.print("\n", .{});

    const res = find_extension_utf8(lfn_str_u8.len, lfn_str_u8);
    try std.testing.expect(res != std.math.maxInt(u64));

    _ = lfn_to_sfn_candidate(lfn_str_u8.len, &lfn_str_u16, &sfn_str);
    std.debug.print("{s}\n", .{sfn_str[0..11]});
    try std.testing.expect(std.mem.eql(u8, &sfn_str, "LAZY-L~1JS "));
}

test "LFN TO SFN with filename len < 8" {
    const lfn_str_u8 = "lazy.jso";

    var sfn_str: [11]u8 = [_]u8{0x20} ** 11;
    var lfn_str_u16: [lfn_str_u8.len]u16 = [_]u16{0x0020} ** lfn_str_u8.len;
    try std.testing.expect(utf8_to_utf16(lfn_str_u8.len, lfn_str_u8, &lfn_str_u16) != std.math.maxInt(u64));
    var it = std.unicode.Utf16LeIterator.init(&lfn_str_u16);
    while (try it.nextCodepoint()) |codepoint| {
        var buf: [4]u8 = [_]u8{0x0000} ** 4;
        const len = try std.unicode.utf8Encode(codepoint, &buf);
        std.debug.print("{s}", .{buf[0..len]});
    }
    std.debug.print("\n", .{});

    const res = find_extension_utf8(lfn_str_u8.len, lfn_str_u8);
    try std.testing.expect(res != std.math.maxInt(u64));

    _ = lfn_to_sfn_candidate(lfn_str_u8.len, &lfn_str_u16, &sfn_str);
    std.debug.print("{s}\n", .{sfn_str[0..11]});
    try std.testing.expect(std.mem.eql(u8, &sfn_str, "LAZY  ~1JSO"));
}

test "SFN validation" {
    const test1 = "LAZY.JSO";
    try expect(is_sfn_compliant_utf8(test1.len, test1) == 1);

    const test2 = "LAZY-LOCK.JSON";
    try expect(is_sfn_compliant_utf8(test2.len, test2) == 0);

    const test3 = "lazy.jso";
    try expect(is_sfn_compliant_utf8(test3.len, test3) == 0);
}
