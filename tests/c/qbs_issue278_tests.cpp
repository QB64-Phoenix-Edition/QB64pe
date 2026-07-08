#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

struct ReallocControl {
    void *target = nullptr;
    std::size_t old_size = 0;
    std::size_t calls = 0;
    std::size_t last_size = 0;
    std::vector<std::size_t> requested_sizes;
    std::size_t fail_above = std::numeric_limits<std::size_t>::max();
    bool fail = false;
    bool force_move = false;
    bool fake_success = false;
};

static ReallocControl realloc_control;

static void *qbs_test_realloc(void *ptr, std::size_t bytes) {
    if (ptr == realloc_control.target) {
        ++realloc_control.calls;
        realloc_control.last_size = bytes;
        realloc_control.requested_sizes.push_back(bytes);

        if (realloc_control.fail || bytes > realloc_control.fail_above)
            return nullptr;

        if (realloc_control.fake_success)
            return ptr;

        if (realloc_control.force_move) {
            void *replacement = std::malloc(bytes);
            if (!replacement)
                return nullptr;

            std::memcpy(replacement, ptr, std::min(realloc_control.old_size, bytes));
            std::free(ptr);
            return replacement;
        }
    }

    return std::realloc(ptr, bytes);
}

// Include the implementation directly so the test can exercise the file-local
// arena state and overflow helpers without exporting production internals.
#define QB64_QBS_TEST 1
#include "../../../internal/c/libqb/src/qbs.cpp"
#undef QB64_QBS_TEST

struct QbsError {
    int32_t code;
};

void error(int32_t code) { throw QbsError{code}; }
void field_free(qbs *) {}
void qbs_remove_cmem(qbs *) {}
bool qbs_new_fixed_cmem(uint8_t *, uint32_t, uint8_t, qbs *) { return false; }
void qbs_move_cmem(qbs *, qbs *) {}
void qbs_copy_cmem(qbs *, qbs *) {}
void qbs_create_cmem(int32_t, uint8_t, qbs *) { std::abort(); }

struct TestFailure : std::exception {
    std::string message;
    explicit TestFailure(std::string text) : message(std::move(text)) {}
    const char *what() const noexcept override { return message.c_str(); }
};

#define REQUIRE(condition)                                                                                               \
    do {                                                                                                                 \
        if (!(condition))                                                                                                \
            throw TestFailure(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " #condition);             \
    } while (false)

static void reset_realloc_control() {
    realloc_control = {};
    qbs_data_realloc = qbs_test_realloc;
}

static void reset_runtime() {
    reset_realloc_control();

    std::free(qbs_data);
    qbs_data = static_cast<uint8_t *>(std::malloc(1048576));
    REQUIRE(qbs_data != nullptr);
    qbs_data_size = 1048576;
    qbs_sp = 0;

    std::free(qbs_list);
    qbs_list = static_cast<intptr_t *>(std::malloc(65536 * sizeof(intptr_t)));
    REQUIRE(qbs_list != nullptr);
    qbs_list_lasti = 65535;
    qbs_list_nexti = 0;

    std::free(qbs_tmp_list);
    qbs_tmp_list = static_cast<intptr_t *>(std::calloc(65536, sizeof(intptr_t)));
    REQUIRE(qbs_tmp_list != nullptr);
    qbs_tmp_list_lasti = 65535;
    qbs_tmp_list_nexti = 0;

    std::memset(qbs_malloc, 0, sizeof(qbs) * 65536);
    qbs_malloc_next = 0;
    qbs_malloc_freed_num = 0;
}

template <typename Function> static void require_error_512(Function &&function) {
    bool caught = false;
    try {
        function();
    } catch (const QbsError &raised) {
        REQUIRE(raised.code == 512);
        caught = true;
    }
    REQUIRE(caught);
}

static void fill_pattern(qbs *text, uint32_t seed) {
    for (int32_t index = 0; index < text->len; ++index)
        text->chr[index] = static_cast<uint8_t>((uint32_t(index) * 131u + seed) & 0xffu);
}

static void verify_pattern(const qbs *text, uint32_t seed) {
    for (int32_t index = 0; index < text->len; ++index)
        REQUIRE(text->chr[index] == static_cast<uint8_t>((uint32_t(index) * 131u + seed) & 0xffu));
}

static void test_platform_sized_arena_types() {
    static_assert(std::is_same_v<decltype(qbs_data_size), std::size_t>);
    static_assert(std::is_same_v<decltype(qbs_sp), std::size_t>);
    REQUIRE(sizeof(qbs_data_size) == sizeof(std::size_t));
    REQUIRE(sizeof(qbs_sp) == sizeof(std::size_t));
}

static void test_checked_size_addition_boundaries() {
    std::size_t result = 0;
    const std::size_t maximum = std::numeric_limits<std::size_t>::max();

    REQUIRE(!qbs_size_add_overflow(0, 0, &result));
    REQUIRE(result == 0);
    REQUIRE(!qbs_size_add_overflow(maximum - 1, 1, &result));
    REQUIRE(result == maximum);

    result = 0x1234;
    REQUIRE(qbs_size_add_overflow(maximum, 1, &result));
    REQUIRE(result == 0x1234);
    REQUIRE(qbs_size_add_overflow(maximum - 7, 8, &result));
}

static void test_data_space_boundaries() {
    reset_runtime();
    REQUIRE(qbs_data_has_space(qbs_data, qbs_data_size));
    REQUIRE(qbs_data_has_space(qbs_data + qbs_data_size, 0));
    REQUIRE(!qbs_data_has_space(qbs_data + qbs_data_size, 1));

    const uintptr_t beyond = reinterpret_cast<uintptr_t>(qbs_data) + qbs_data_size + 1;
    REQUIRE(!qbs_data_has_space(reinterpret_cast<uint8_t *>(beyond), 0));
}

static void test_negative_allocation_reports_oom() {
    reset_runtime();
    const std::size_t old_sp = qbs_sp;
    const uint32_t old_count = qbs_list_nexti;
    require_error_512([] { (void)qbs_new(-1, 0); });
    REQUIRE(qbs_sp == old_sp);
    REQUIRE(qbs_list_nexti == old_count);
}

static void test_fixed_and_variable_copy_paths() {
    reset_runtime();

    uint8_t fixed_buffer[8] = {};
    qbs *fixed_text = qbs_new_fixed(fixed_buffer, sizeof(fixed_buffer), 0);
    qbs_set(fixed_text, qbs_new_txt_len("abc", 3));
    REQUIRE(std::memcmp(fixed_buffer, "abc     ", 8) == 0);

    qbs *long_destination = qbs_new(32, 0);
    std::memset(long_destination->chr, 'x', long_destination->len);
    qbs *short_source = qbs_new(7, 0);
    std::memcpy(short_source->chr, "shorter", 7);
    uint8_t *long_destination_address = long_destination->chr;
    qbs_set(long_destination, short_source);
    REQUIRE(long_destination->chr == long_destination_address);
    REQUIRE(long_destination->len == 7);
    REQUIRE(std::memcmp(long_destination->chr, "shorter", 7) == 0);

    qbs *relocated_destination = qbs_new(8, 0);
    qbs *blocker = qbs_new(4096, 0);
    qbs *long_source = qbs_new(65536, 0);
    fill_pattern(blocker, 17);
    fill_pattern(long_source, 91);
    uint8_t *old_destination_address = relocated_destination->chr;
    qbs_set(relocated_destination, long_source);
    REQUIRE(relocated_destination->chr != old_destination_address);
    REQUIRE(relocated_destination->len == long_source->len);
    verify_pattern(relocated_destination, 91);
    verify_pattern(blocker, 17);

    qbs *acquiring_destination = qbs_new(4, 0);
    qbs *temporary_source = qbs_new(32768, 1);
    fill_pattern(temporary_source, 203);
    uint8_t *temporary_data = temporary_source->chr;
    const uint32_t temporary_list_index = temporary_source->listi;
    const uint32_t temporary_tracking_index = temporary_source->tmplisti;
    qbs_set(acquiring_destination, temporary_source);
    REQUIRE(acquiring_destination->chr == temporary_data);
    REQUIRE(acquiring_destination->listi == temporary_list_index);
    REQUIRE(qbs_tmp_list[temporary_tracking_index] == -1);
    verify_pattern(acquiring_destination, 203);
}

static void test_concatenation_and_binary_copy() {
    reset_runtime();

    const char left_bytes[] = {'A', '\0', 'B', 'C'};
    const char right_bytes[] = {'D', 'E', '\0', 'F', 'G'};
    qbs *joined = qbs_add(qbs_new_txt_len(left_bytes, sizeof(left_bytes)), qbs_new_txt_len(right_bytes, sizeof(right_bytes)));

    const char expected[] = {'A', '\0', 'B', 'C', 'D', 'E', '\0', 'F', 'G'};
    REQUIRE(joined != nullptr);
    REQUIRE(joined->len == static_cast<int32_t>(sizeof(expected)));
    REQUIRE(std::memcmp(joined->chr, expected, sizeof(expected)) == 0);
    REQUIRE(joined->tmp == 1);
}

static void test_concatenation_length_overflow_reports_oom() {
    reset_runtime();

    uint8_t dummy = 0;
    qbs left{};
    qbs right{};
    left.chr = &dummy;
    right.chr = &dummy;
    left.len = std::numeric_limits<int32_t>::max();
    right.len = 1;

    const std::size_t old_sp = qbs_sp;
    const uint32_t old_count = qbs_list_nexti;
    require_error_512([&] { (void)qbs_add(&left, &right); });
    REQUIRE(qbs_sp == old_sp);
    REQUIRE(qbs_list_nexti == old_count);
}

static void test_forced_realloc_move_rebases_every_live_string() {
    reset_runtime();

    std::vector<qbs *> strings;
    std::vector<std::size_t> offsets;
    for (int index = 0; index < 12; ++index) {
        qbs *text = qbs_new(40000 + index * 97, 0);
        fill_pattern(text, 1000u + uint32_t(index));
        strings.push_back(text);
        offsets.push_back(qbs_data_offset(text->chr));
    }

    uint8_t *old_base = qbs_data;
    realloc_control.target = qbs_data;
    realloc_control.old_size = qbs_data_size;
    realloc_control.force_move = true;

    qbs *large_text = qbs_new(2 * 1024 * 1024, 0);
    REQUIRE(large_text != nullptr);
    REQUIRE(realloc_control.calls == 1);
    REQUIRE(qbs_data != old_base);

    for (std::size_t index = 0; index < strings.size(); ++index) {
        REQUIRE(qbs_data_offset(strings[index]->chr) == offsets[index]);
        verify_pattern(strings[index], 1000u + uint32_t(index));
    }
}

static void test_fragmentation_compaction_preserves_data() {
    reset_runtime();

    qbs *first = qbs_new(180000, 0);
    qbs *discarded = qbs_new(210000, 0);
    qbs *third = qbs_new(190000, 0);
    fill_pattern(first, 41);
    fill_pattern(discarded, 42);
    fill_pattern(third, 43);

    const std::size_t third_offset_before = qbs_data_offset(third->chr);
    qbs_free(discarded);

    qbs *large_text = qbs_new(800000, 0);
    REQUIRE(large_text != nullptr);
    REQUIRE(qbs_data_offset(third->chr) < third_offset_before);
    verify_pattern(first, 41);
    verify_pattern(third, 43);
}

static void test_realloc_failure_reports_oom_and_preserves_live_data() {
    reset_runtime();

    qbs *survivor = qbs_new(240000, 0);
    fill_pattern(survivor, 77);
    uint8_t *old_base = qbs_data;
    const std::size_t old_size = qbs_data_size;
    const uint32_t old_count = qbs_list_nexti;

    realloc_control.target = qbs_data;
    realloc_control.old_size = qbs_data_size;
    realloc_control.fail = true;

    require_error_512([] { (void)qbs_new(2 * 1024 * 1024, 0); });
    REQUIRE(realloc_control.calls == 2);
    REQUIRE(realloc_control.requested_sizes[1] < realloc_control.requested_sizes[0]);
    REQUIRE(qbs_data == old_base);
    REQUIRE(qbs_data_size == old_size);
    REQUIRE(qbs_list_nexti == old_count);
    verify_pattern(survivor, 77);
}

static void test_failed_optional_growth_uses_existing_capacity() {
    reset_runtime();

    const std::size_t old_size = qbs_data_size;
    uint8_t *old_base = qbs_data;
    qbs_list_nexti = 0;
    qbs_sp = 600000;

    const std::size_t bytesrequired = 100000;
    REQUIRE(qbs_sp + bytesrequired <= qbs_data_size);

    realloc_control.target = qbs_data;
    realloc_control.fail = true;

    qbs_concat(bytesrequired);

    REQUIRE(realloc_control.calls == 1);
    REQUIRE(qbs_data == old_base);
    REQUIRE(qbs_data_size == old_size);
    REQUIRE(qbs_sp + bytesrequired <= qbs_data_size);
}

static void test_failed_geometric_growth_retries_exact_requirement() {
    reset_runtime();

    qbs_list_nexti = 0;
    qbs_sp = qbs_data_size - 100000;
    const std::size_t bytesrequired = 200000;
    const std::size_t required_size = qbs_sp + bytesrequired;
    const std::size_t preferred_size = qbs_data_size * 2 + bytesrequired;

    realloc_control.target = qbs_data;
    realloc_control.fail_above = required_size;
    realloc_control.fake_success = true;

    qbs_concat(bytesrequired);

    REQUIRE(realloc_control.calls == 2);
    REQUIRE(realloc_control.requested_sizes.size() == 2);
    REQUIRE(realloc_control.requested_sizes[0] == preferred_size);
    REQUIRE(realloc_control.requested_sizes[1] == required_size);
    REQUIRE(qbs_data_size == required_size);
}

static void test_arena_overflow_and_geometric_growth_boundaries() {
    reset_runtime();
    const std::size_t maximum = std::numeric_limits<std::size_t>::max();
    uint8_t *actual_data = qbs_data;
    const std::size_t actual_size = qbs_data_size;

    qbs_list_nexti = 0;
    qbs_data_size = maximum - 1024;
    qbs_sp = maximum - 64;
    realloc_control.target = qbs_data;
    require_error_512([] { qbs_concat(128); });
    REQUIRE(realloc_control.calls == 0);

    reset_realloc_control();
    realloc_control.target = qbs_data;
    realloc_control.fake_success = true;
    qbs_data_size = maximum / 2 + 4096;
    qbs_sp = qbs_data_size - 128;
    qbs_concat(128);
    REQUIRE(realloc_control.calls == 0);

    reset_realloc_control();
    realloc_control.target = qbs_data;
    realloc_control.fake_success = true;
    qbs_data_size = maximum / 2 + 4096;
    qbs_sp = qbs_data_size - 64;
    const std::size_t exact_required = qbs_sp + 128;
    qbs_concat(128);
    REQUIRE(realloc_control.calls == 1);
    REQUIRE(realloc_control.last_size == exact_required);
    REQUIRE(qbs_data_size == exact_required);

    qbs_data = actual_data;
    qbs_data_size = actual_size;
    qbs_sp = 0;
    reset_realloc_control();
}

static void test_qbs_set_size_overflow_reports_oom() {
    reset_runtime();

    qbs *destination = qbs_new(1, 0);
    (void)qbs_new(1, 0); // Prevent destination from being the last live string.

    uint8_t source_bytes[64] = {};
    qbs source{};
    source.chr = source_bytes;
    source.len = sizeof(source_bytes);
    source.readonly = 1;

    const std::size_t actual_size = qbs_data_size;
    qbs_data_size = std::numeric_limits<std::size_t>::max();
    qbs_sp = std::numeric_limits<std::size_t>::max() - 4;
    require_error_512([&] { (void)qbs_set(destination, &source); });
    qbs_data_size = actual_size;
    qbs_sp = 0;
}

static void test_configurable_large_arena_stress() {
    reset_runtime();

    std::size_t stress_mib = 64;
    if (const char *setting = std::getenv("QBS_ISSUE278_STRESS_MIB")) {
        char *end = nullptr;
        const unsigned long long parsed = std::strtoull(setting, &end, 10);
        if (end != setting && *end == '\0' && parsed > 0)
            stress_mib = static_cast<std::size_t>(parsed);
    }

    const std::size_t chunk_size = 4 * 1024 * 1024;
    const std::size_t chunk_count = std::max<std::size_t>(1, (stress_mib * 1024 * 1024) / chunk_size);
    std::vector<qbs *> chunks;
    chunks.reserve(chunk_count);

    for (std::size_t chunk_index = 0; chunk_index < chunk_count; ++chunk_index) {
        qbs *text = qbs_new(static_cast<int32_t>(chunk_size), 0);
        REQUIRE(text != nullptr);
        const uint8_t marker = static_cast<uint8_t>((chunk_index * 29u + 7u) & 0xffu);
        text->chr[0] = marker;
        text->chr[chunk_size / 2] = static_cast<uint8_t>(marker ^ 0x5a);
        text->chr[chunk_size - 1] = static_cast<uint8_t>(marker ^ 0xa5);
        chunks.push_back(text);

        for (std::size_t verify_index = 0; verify_index < chunks.size(); ++verify_index) {
            const uint8_t expected = static_cast<uint8_t>((verify_index * 29u + 7u) & 0xffu);
            REQUIRE(chunks[verify_index]->chr[0] == expected);
            REQUIRE(chunks[verify_index]->chr[chunk_size / 2] == static_cast<uint8_t>(expected ^ 0x5a));
            REQUIRE(chunks[verify_index]->chr[chunk_size - 1] == static_cast<uint8_t>(expected ^ 0xa5));
        }
    }
}

using TestFunction = void (*)();

static int run_test(const char *name, TestFunction function) {
    try {
        function();
        std::printf("PASS  %s\n", name);
        return 0;
    } catch (const TestFailure &failure) {
        std::fprintf(stderr, "FAIL  %s\n      %s\n", name, failure.what());
    } catch (const QbsError &raised) {
        std::fprintf(stderr, "FAIL  %s\n      unexpected error(%d)\n", name, raised.code);
    } catch (const std::exception &failure) {
        std::fprintf(stderr, "FAIL  %s\n      %s\n", name, failure.what());
    } catch (...) {
        std::fprintf(stderr, "FAIL  %s\n      unknown exception\n", name);
    }
    return 1;
}

int main() {
    const std::pair<const char *, TestFunction> tests[] = {
        {"platform-sized arena counters", test_platform_sized_arena_types},
        {"checked size addition boundaries", test_checked_size_addition_boundaries},
        {"arena space boundaries", test_data_space_boundaries},
        {"negative allocation -> OOM", test_negative_allocation_reports_oom},
        {"fixed and variable copy paths", test_fixed_and_variable_copy_paths},
        {"binary-safe concatenation", test_concatenation_and_binary_copy},
        {"concatenation length overflow -> OOM", test_concatenation_length_overflow_reports_oom},
        {"realloc move rebases live pointers", test_forced_realloc_move_rebases_every_live_string},
        {"fragmentation compaction", test_fragmentation_compaction_preserves_data},
        {"realloc failure -> OOM", test_realloc_failure_reports_oom_and_preserves_live_data},
        {"optional reserve failure uses current arena", test_failed_optional_growth_uses_existing_capacity},
        {"geometric failure retries exact size", test_failed_geometric_growth_retries_exact_requirement},
        {"SIZE_MAX growth boundaries", test_arena_overflow_and_geometric_growth_boundaries},
        {"qbs_set size overflow -> OOM", test_qbs_set_size_overflow_reports_oom},
        {"configurable large arena stress", test_configurable_large_arena_stress},
    };

    int failures = 0;
    for (const auto &test : tests)
        failures += run_test(test.first, test.second);

    if (failures) {
        std::fprintf(stderr, "\n%d qbs issue #278 test(s) failed.\n", failures);
        return 1;
    }

    const std::size_t reported_stress = std::getenv("QBS_ISSUE278_STRESS_MIB")
                                            ? static_cast<std::size_t>(std::strtoull(std::getenv("QBS_ISSUE278_STRESS_MIB"), nullptr, 10))
                                            : std::size_t{64};
    std::printf("\nAll qbs issue #278 tests passed. size_t=%zu bits, arena=%zu MiB stress.\n", sizeof(std::size_t) * 8,
                reported_stress);
    return 0;
}
