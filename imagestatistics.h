#pragma once

#include <cstddef>

struct ImageStatistics
{
    size_t total_images;
    size_t was_saved;
    size_t downloading_error_count;
    size_t count_of_already_exists;
};

