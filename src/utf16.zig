const std = @import("std");
const uni = std.unicode;

export fn utf16_to_utf8(len: u64, utf16_str: [len]u16, dest: [len * 2:0]u8) i32 {
    const res : u64= uni.utf16leToUtf8(dest, utf16_str) catch std.math.maxInt(u64);
    return res;
}

//create SFN representation, defaults to ~1. 
export fn lfn_to_sfn_candidate(len: u64, lfn_str: [len]u16, dest: [11]u8) i32
{
    var u8_str : [len*2:0]u8 = {};
    utf16_to_utf8(len, lfn_str, &u8_str);
    const ext_offset = find_extension_utf8(u8_str);
    if(ext_offset == std.math.maxInt(u64))
    {
        return -1;
    }
    @memcpy(dest[8..10], u8_str[ext_offset + 1..ext_offset + 3]);
    const filename_slice = dest[0..7];
    @memset(dest[0..7], 32); //Set entire name section to spaces for padding.
    var lfn_offset = 0;
    for(filename_slice) |*char|
    {
        if(lfn_offset == ext_offset)
        {
            break;
        }
        string_traversal: while(lfn_offset < ext_offset) : (lfn_offset += 1) {
        
            const current_char : u8 = u8_str[lfn_offset];
            switch(current_char) { // not optimized or simplifying LOC wise, but every case is sorted numerically by their code value.
                0 => std.debug.panic("A lfn string with null value has been passed! check your null bounds in C : {any}", lfn_str), //ruh oh AKA someone fucked up the bounds check somewhere in C-land.
                1...31 => continue : string_traversal, //Control characters are ignored
                32...''
                ':'...'?'  => continue : string_traversal,
                '@'...'Z' => { //Upper Case Alphabetics and the @ sign does not need any modifications
                    char.* = current_char;
                    break string_traversal;
                },


                'a'...'z' => {
                    char.* = std.ascii.toUpper(current_char);
                    break string_traversal;
                },

                else => {},

            }

    }

}


export fn find_extension_utf8(len:u64, str:[len]u8) u64
{
    var count = len - 1;
    while(count > 0) : ( count -= 1)
    {
        if(str[count] == '.')
        {
            return count;
        }
    }

    return std.math.maxInt(u64);
    
}


