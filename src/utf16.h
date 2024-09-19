#include "zig.h"
zig_extern uint64_t utf16_to_utf8(uint64_t const a0, uint16_t const *const a1, uint8_t *const a2);
zig_extern uint64_t utf8_to_utf16(uint64_t const a0, uint8_t const *const a1, uint16_t *const a2);
zig_extern int32_t lfn_to_sfn_candidate(uint64_t const a0, uint16_t const *const a1, uint8_t *const a2);
zig_extern int32_t is_sfn_compliant_utf16(uint64_t const a0, uint16_t const *const a1);
zig_extern int32_t is_sfn_compliant_utf8(uint64_t const a0, uint8_t const *const a1);
zig_extern uint64_t find_extension_utf8(uint64_t const a0, uint8_t const *const a1);
zig_extern uintptr_t getauxval(uintptr_t const a0);
