#include "test_atomics.hpp"
#include "test_fixtures.hpp"
#include "logger/log.hpp"
#include "persistent/in/file_reader.hpp"
#include "text/encoding.hpp"
#include "text/transcoding_views.hpp"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <ranges>
#include <algorithm>

namespace tests::transcoding_views {

  // Test fixture for transcoding view tests
  class TranscodingViewsTestFixture : public ::testing::Test {
  protected:
    std::filesystem::path test_dir;
    std::filesystem::path utf8_file;
    std::filesystem::path iso8859_file;

    void SetUp() override {
      logger::scope_logger log_raii{logger::development_trace, "TranscodingViewsTestFixture::SetUp"};

      // Create temporary test directory
      test_dir = std::filesystem::temp_directory_path() / "cratchit_test_transcoding_views";
      std::filesystem::create_directories(test_dir);

      // Create UTF-8 test file with Swedish content
      utf8_file = test_dir / "utf8_test.txt";
      {
        std::ofstream ofs(utf8_file, std::ios::binary);
        std::string utf8_content = "Swedish characters: åäöÅÄÖ\nMore content: Test123\n";
        ofs.write(utf8_content.data(), utf8_content.size());
      }

      // Create ISO-8859-1 test file
      iso8859_file = test_dir / "iso8859_test.txt";
      {
        std::ofstream ofs(iso8859_file, std::ios::binary);
        // ISO-8859-1 encoded: "åäö" (0xE5 = å, 0xE4 = ä, 0xF6 = ö)
        unsigned char iso_content[] = {
          0xE5, 0xE4, 0xF6, '\n',  // åäö
          'T', 'e', 's', 't', '\n'
        };
        ofs.write(reinterpret_cast<char*>(iso_content), sizeof(iso_content));
      }

      logger::development_trace("Test files created in: {}", test_dir.string());
    }

    void TearDown() override {
      logger::scope_logger log_raii{logger::development_trace, "TranscodingViewsTestFixture::TearDown"};
      std::filesystem::remove_all(test_dir);
    }
  };

  // Test lazy decoding: bytes (detected encoding) → Unicode code points
  TEST_F(TranscodingViewsTestFixture, LazyDecodingDetectedEncodingToUnicode) {
    logger::scope_logger log_raii{logger::development_trace, "TEST(TranscodingViews, LazyDecodingDetectedEncodingToUnicode)"};

    // Read UTF-8 file
    auto buffer_result = persistent::in::path_to_byte_buffer_shortcut(utf8_file);
    ASSERT_TRUE(buffer_result) << "Expected successful file read";

    // Create lazy decoding view: bytes → Unicode code points
    auto unicode_view = text::encoding::views::bytes_to_unicode(
      buffer_result.value(),
      text::encoding::DetectedEncoding::UTF8
    );

    // Demonstrate lazy evaluation: take only first 10 Unicode code points
    auto first_10_codepoints = unicode_view | std::views::take(10);

    std::vector<char32_t> codepoints;
    for (char32_t cp : first_10_codepoints) {
      codepoints.push_back(cp);
    }

    EXPECT_LE(codepoints.size(), 10) << "Expected at most 10 code points";
    EXPECT_GT(codepoints.size(), 0) << "Expected some code points";

    // Verify we got Unicode code points (should include Swedish characters if present in first 10)
    logger::development_trace("Decoded {} Unicode code points lazily", codepoints.size());
    for (size_t i = 0; i < codepoints.size(); ++i) {
      logger::development_trace("  Code point {}: U+{:04X}", i, static_cast<uint32_t>(codepoints[i]));
    }
  }

  // Test lazy encoding: Unicode code points → runtime encoding bytes
  TEST_F(TranscodingViewsTestFixture, LazyEncodingUnicodeToRuntimeEncoding) {
    logger::scope_logger log_raii{logger::development_trace, "TEST(TranscodingViews, LazyEncodingUnicodeToRuntimeEncoding)"};

    // Create a small vector of Unicode code points
    std::vector<char32_t> unicode_codepoints = {
      U'H', U'e', U'l', U'l', U'o', U' ',
      U'å', U'ä', U'ö',  // Swedish characters
      U'!', U'\n'
    };

    // Create lazy encoding view: Unicode → platform encoding bytes
    auto encoding_view = text::encoding::views::unicode_to_runtime_encoding(unicode_codepoints);

    // Demonstrate lazy evaluation: take only first 20 bytes
    auto first_20_bytes = encoding_view | std::views::take(20);

    std::string partial_result;
    for (char byte : first_20_bytes) {
      partial_result.push_back(byte);
    }

    EXPECT_LE(partial_result.size(), 20) << "Expected at most 20 bytes";
    EXPECT_GT(partial_result.size(), 0) << "Expected some bytes";

    logger::development_trace("Encoded {} bytes lazily from Unicode", partial_result.size());
    logger::development_trace("Partial result: {}", partial_result);
  }

  // Test combined lazy pipeline: bytes → decode → Unicode → encode → bytes
  TEST_F(TranscodingViewsTestFixture, LazyTranscodingFullPipeline) {
    logger::scope_logger log_raii{logger::development_trace, "TEST(TranscodingViews, LazyTranscodingFullPipeline)"};

    // Read UTF-8 file
    auto buffer_result = persistent::in::path_to_byte_buffer_shortcut(utf8_file);
    ASSERT_TRUE(buffer_result) << "Expected successful file read";

    // Create full lazy transcoding pipeline: bytes → Unicode → platform encoding
    auto unicode_view = text::encoding::views::bytes_to_unicode(
      buffer_result.value(),
      text::encoding::DetectedEncoding::UTF8
    );
    auto platform_view = text::encoding::views::unicode_to_runtime_encoding(unicode_view);

    // Demonstrate lazy evaluation: take only first 50 bytes from the full pipeline
    auto first_50_bytes = platform_view | std::views::take(50);

    std::string partial_result;
    for (char byte : first_50_bytes) {
      partial_result.push_back(byte);
    }

    EXPECT_LE(partial_result.size(), 50) << "Expected at most 50 bytes";
    EXPECT_GT(partial_result.size(), 0) << "Expected some bytes";

    // Verify the content makes sense (should start with "Swedish characters:" if UTF-8 encoded correctly)
    logger::development_trace("Full lazy pipeline produced {} bytes", partial_result.size());
    logger::development_trace("Content: {}", partial_result);
  }

  // Test lazy view for memory efficiency (refactored from old test)
  // Demonstrates that we can process only part of a file without materializing everything
  TEST_F(TranscodingViewsTestFixture, LazyViewForMemoryEfficiency) {
    logger::scope_logger log_raii{logger::development_trace, "TEST(TranscodingViews, LazyViewForMemoryEfficiency)"};

    // Read file to buffer
    auto buffer_result = persistent::in::path_to_byte_buffer_shortcut(utf8_file);
    ASSERT_TRUE(buffer_result) << "Expected successful file read";

    // Detect encoding directly (or use known encoding for test)
    auto encoding_result = text::encoding::icu::to_detetced_encoding(
      buffer_result.value(),
      text::encoding::icu::DEFAULT_CONFIDENCE_THERSHOLD
    );
    auto detected_encoding = encoding_result
      ? encoding_result->encoding
      : text::encoding::DetectedEncoding::UTF8;

    // Create lazy view directly using view primitives
    auto unicode_view = text::encoding::views::bytes_to_unicode(
      buffer_result.value(),
      detected_encoding
    );
    auto platform_view = text::encoding::views::unicode_to_runtime_encoding(unicode_view);

    // Take only first 100 bytes - demonstrating lazy evaluation and memory efficiency
    auto first_100 = platform_view | std::views::take(100);

    std::string partial_result;
    for (char byte : first_100) {
      partial_result.push_back(byte);
    }

    EXPECT_LE(partial_result.size(), 100) << "Expected at most 100 bytes";
    EXPECT_GT(partial_result.size(), 0) << "Expected some content";

    logger::development_trace("Lazy view first 100 bytes: {}", partial_result);
  }

} // namespace tests::transcoding_views
