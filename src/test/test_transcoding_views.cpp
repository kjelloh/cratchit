#include "test_atomics.hpp"
#include "test_fixtures.hpp"
#include "logger/log.hpp"
#include "persistent/in/raw_text_read.hpp"
#include "text/encoding.hpp"
#include "text/to_inferred_encoding.hpp"
#include "text/transcoding_views.hpp"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <ranges>
#include <algorithm>

