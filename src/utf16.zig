const std = @import("std");
const uni = std.unicode;

export fn utf16_to_utf8(len: u64, utf16_str: [*]u16, dest: [*]u8) u64 {

    //big case of "trust me bro" here, C-land needs to make sure both passed strings follows the proper bounds. As fat32 names are not null terminated.
    const utf16_slc = utf16_str[0 .. len - 1];
    const dest_slc = dest[0 .. len * 2 - 1];

    const res: u64 = uni.utf16leToUtf8(dest_slc, utf16_slc) catch std.math.maxInt(u64);
    return res;
}

//create SFN representation, defaults to ~1.
export fn lfn_to_sfn_candidate(len: u64, lfn_str: [*]u16, dest: [*]u8) i32 {
    if (lfn_str[len] != 0) //where is the null boi
    {}
    var u8_str: [512:0]u8 = std.mem.zeroes([512:0]u8);
    utf16_to_utf8(len, lfn_str, &u8_str);
    const ext_offset = find_extension_utf8(u8_str);
    if (ext_offset != std.math.maxInt(u64)) {
        var max_diff = 3;
        if (ext_offset > std.math.maxInt(u64) - 4) {
            max_diff = std.math.maxInt(u64) - 1 - ext_offset;
        }
        @memcpy(dest[8 .. 8 + max_diff], u8_str[ext_offset + 1 .. ext_offset + max_diff]);
    }

    const filename_slice = dest[0..5];
    @memset(dest[0..5], 0);
    dest[6] = "~";
    dest[7] = "1";
    var lfn_offset = 0;
    for (filename_slice) |*char| {
        if (lfn_offset == ext_offset) { // we ran out the filename, stop.
            break;
        }
        string_traversal: while (lfn_offset < ext_offset) : (lfn_offset += 1) {
            const current_char: u8 = u8_str[lfn_offset];

            switch (current_char) { // not optimized or simplified, but every case is sorted numerically by their code value.
                0 => std.debug.panic("A lfn string with null value has been passed! check your null bounds in C : {any}", lfn_str), //ruh oh AKA someone fucked up the bounds check somewhere in C-land, they should not be visible here.

                1...32 | 34 => continue :string_traversal, //Control characters, and spaces are skipped

                '!' | '#'...')' => {
                    char.* = current_char;
                    break :string_traversal;
                },

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

                '{' | '}'...'~' => {
                    char.* = current_char;
                    break :string_traversal;
                },

                '|' => continue :string_traversal,

                127...255 => //DEL and extended ASCII should be skipped
                continue :string_traversal,

                //128...255 => { allowed by some specs, but DOS does not support this under NLS mode, and therefore is skipped.
                //    char.* = current_char;
                //    break string_traversal
                //        break :string_traversal;
                //},
            }
        }
    }
}

inline fn toUpperCase(c: u8) u8 {
    return c + 32;
}

export fn find_extension_utf8(len: u64, str: [*]u8) u64 {
    var count = len - 1;
    while (count > 0) : (count -= 1) {
        if (str[count] == '.') {
            return count;
        }
    }
    return std.math.maxInt(u64);
}
