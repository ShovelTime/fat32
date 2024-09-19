#include "zig.h"
zig_extern uint64_t utf16_to_utf8(uint64_t const in_len, uint16_t const *const in_string, uint8_t *const dest);
zig_extern uint64_t utf8_to_utf16(uint64_t const in_len, uint8_t const *const in_string, uint16_t *const dest);
zig_extern int32_t lfn_to_sfn_candidate(uint64_t const in_len, uint16_t const *const lfn_string, uint8_t *const dest);
zig_extern int32_t is_sfn_compliant_utf16(uint64_t const len, uint16_t const *const string);
zig_extern int32_t is_sfn_compliant_utf8(uint64_t const len, uint8_t const *const string);
zig_extern uint64_t find_extension_utf8(uint64_t const len, uint8_t const *const string);
zig_extern uintptr_t getauxval(uintptr_t const a0); // Huh?
