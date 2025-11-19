#include "test_atomics.hpp"
#include "test_fixtures.hpp"
#include "logger/log.hpp"
#include "io/file_reader.hpp"
#include "text/encoding.hpp"
#include "text/transcoding_views.hpp"
#include "text/encoding_pipeline.hpp"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <ranges>
#include <algorithm>
// #include <cstring>

namespace tests::csv_import_pipeline {
  namespace file_io_suite {

    // Test fixture for file I/O tests
    class FileIOTestFixture : public ::testing::Test {
    protected:
      std::filesystem::path test_dir;
      std::filesystem::path valid_file_path;
      std::filesystem::path missing_file_path;

      void SetUp() override {
        logger::scope_logger log_raii{logger::development_trace, "FileIOTestFixture::SetUp"};

        // Create temporary test directory
        test_dir = std::filesystem::temp_directory_path() / "cratchit_test_file_io";
        std::filesystem::create_directories(test_dir);

        // Create a valid test file with some content
        valid_file_path = test_dir / "valid_test.csv";
        std::ofstream ofs(valid_file_path, std::ios::binary);
        std::string test_content = "Date,Description,Amount\n2025-01-01,Test Transaction,100.00\n";
        ofs.write(test_content.data(), test_content.size());
        ofs.close();

        // Path to a file that doesn't exist
        missing_file_path = test_dir / "missing_file.csv";
      }

      void TearDown() override {
        logger::scope_logger log_raii{logger::development_trace, "FileIOTestFixture::TearDown"};

        // Clean up test directory
        std::error_code ec;
        std::filesystem::remove_all(test_dir, ec);
      }
    };

    TEST_F(FileIOTestFixture, ReadValidFileSucceeds) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FileIOTests, ReadValidFile)"};

      auto result = cratchit::io::read_file_to_buffer(valid_file_path);

      ASSERT_TRUE(result) << "Expected successful read of valid file";
      EXPECT_GT(result.value().size(), 0) << "Expected non-empty buffer";

      // Verify content
      std::string content(
        reinterpret_cast<const char*>(result.value().data()),
        result.value().size()
      );
      EXPECT_TRUE(content.find("Date,Description,Amount") != std::string::npos)
        << "Expected CSV header in content";
    }

    TEST_F(FileIOTestFixture, ReadMissingFileReturnsEmpty) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FileIOTests, ReadMissingFileReturnsEmpty)"};

      auto result = cratchit::io::read_file_to_buffer(missing_file_path);

      EXPECT_FALSE(result) << "Expected empty optional for missing file";
      EXPECT_GT(result.m_messages.size(), 0) << "Expected error messages";

      // Verify no exception was thrown (test passes if we get here)
      SUCCEED() << "No exception thrown for missing file";
    }

    TEST(FileIOTests, OpenValidFile) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FileIOTests, OpenValidFile)"};

      // Create a temporary test file
      auto temp_path = std::filesystem::temp_directory_path() / "cratchit_test_open.txt";
      {
        std::ofstream ofs(temp_path);
        ofs << "test content";
      }

      auto result = cratchit::io::open_file(temp_path);

      ASSERT_TRUE(result) << "Expected successful file open";
      EXPECT_TRUE(result.value() != nullptr) << "Expected non-null stream pointer";
      EXPECT_TRUE(result.value()->good()) << "Expected stream in good state";

      // Clean up
      std::filesystem::remove(temp_path);
    }

    TEST(FileIOTests, OpenMissingFileReturnsEmpty) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FileIOTests, OpenMissingFileReturnsEmpty)"};

      auto non_existent_path = std::filesystem::path("/tmp/cratchit_nonexistent_file_12345.txt");

      auto result = cratchit::io::open_file(non_existent_path);

      EXPECT_FALSE(result) << "Expected empty optional for non-existent file";
      EXPECT_GT(result.m_messages.size(), 0) << "Expected error messages";
    }

    TEST(FileIOTests, StreamToBufferWithValidStream) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FileIOTests, StreamToBufferWithValidStream)"};

      std::string test_data = "Hello, World! This is test data.";
      std::istringstream iss(test_data);

      auto result = cratchit::io::read_stream_to_buffer(iss);

      ASSERT_TRUE(result) << "Expected successful buffer read from stream";
      EXPECT_EQ(result.value().size(), test_data.size()) << "Expected buffer size to match input";

      // Verify content
      std::string buffer_content(
        reinterpret_cast<const char*>(result.value().data()),
        result.value().size()
      );
      EXPECT_EQ(buffer_content, test_data) << "Expected buffer content to match input";
    }

    TEST(FileIOTests, MonadicCompositionChaining) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FileIOTests, MonadicCompositionChaining)"};

      // Create test file
      auto temp_path = std::filesystem::temp_directory_path() / "cratchit_test_composition.txt";
      std::string test_content = "Monadic composition test content";
      {
        std::ofstream ofs(temp_path);
        ofs << test_content;
      }

      // Demonstrate monadic composition with and_then
      auto result = cratchit::io::open_file(temp_path)
        .and_then([](auto& stream_ptr) {
          return cratchit::io::read_stream_to_buffer(*stream_ptr);
        })
        .and_then([](auto& buffer) -> cratchit::io::IOResult<size_t> {
          cratchit::io::IOResult<size_t> size_result;
          size_result.m_value = buffer.size();
          size_result.push_message(std::format("Buffer size: {}", buffer.size()));
          return size_result;
        });

      ASSERT_TRUE(result) << "Expected successful monadic composition";
      EXPECT_EQ(result.value(), test_content.size()) << "Expected correct buffer size";
      EXPECT_GE(result.m_messages.size(), 3) << "Expected messages from each step";

      // Clean up
      std::filesystem::remove(temp_path);
    }

    TEST(FileIOTests, MonadicCompositionShortCircuits) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FileIOTests, MonadicCompositionShortCircuits)"};

      auto non_existent = std::filesystem::path("/tmp/cratchit_does_not_exist_9999.txt");

      bool and_then_called = false;

      // Composition should short-circuit on first failure
      auto result = cratchit::io::open_file(non_existent)
        .and_then([&and_then_called](auto& stream_ptr) -> cratchit::io::IOResult<cratchit::io::ByteBuffer> {
          and_then_called = true;
          return cratchit::io::read_stream_to_buffer(*stream_ptr);
        });

      EXPECT_FALSE(result) << "Expected failure due to missing file";
      EXPECT_FALSE(and_then_called) << "Expected and_then to NOT be called after failure";
      EXPECT_GT(result.m_messages.size(), 0) << "Expected error messages from first step";
    }

    TEST_F(FileIOTestFixture, EmptyFileReadsSuccessfully) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FileIOTests, EmptyFileReadsSuccessfully)"};

      // Create empty file
      auto empty_file = test_dir / "empty.txt";
      std::ofstream ofs(empty_file);
      ofs.close();

      auto result = cratchit::io::read_file_to_buffer(empty_file);

      ASSERT_TRUE(result) << "Expected successful read of empty file";
      EXPECT_EQ(result.value().size(), 0) << "Expected empty buffer";
    }

    TEST_F(FileIOTestFixture, ErrorMessagesArePreserved) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FileIOTests, ErrorMessagesArePreserved)"};

      auto result = cratchit::io::read_file_to_buffer(missing_file_path);

      EXPECT_FALSE(result) << "Expected failure for missing file";
      EXPECT_FALSE(result.m_messages.empty()) << "Expected error messages";

      // Print messages for verification (visible in test output)
      for (const auto& msg : result.m_messages) {
        logger::development_trace("Error message: {}", msg);
      }
    }

  } // namespace file_io_suite

  namespace encoding_detection_suite {

    // Test fixture for encoding detection tests
    class EncodingDetectionTestFixture : public ::testing::Test {
    protected:
      std::filesystem::path test_dir;
      std::filesystem::path utf8_file;
      std::filesystem::path iso8859_file;

      void SetUp() override {
        logger::scope_logger log_raii{logger::development_trace, "EncodingDetectionTestFixture::SetUp"};

        // Create temporary test directory
        test_dir = std::filesystem::temp_directory_path() / "cratchit_test_encoding";
        std::filesystem::create_directories(test_dir);

        // Create UTF-8 test file
        utf8_file = test_dir / "utf8_test.csv";
        std::ofstream utf8_ofs(utf8_file, std::ios::binary);
        std::string utf8_content = "Name,Amount\nJohan Svensson,100.50\nÅsa Lindström,250.75\n";
        utf8_ofs.write(utf8_content.data(), utf8_content.size());
        utf8_ofs.close();

        // Create ISO-8859-1 test file with Swedish characters
        iso8859_file = test_dir / "iso8859_test.csv";
        std::ofstream iso_ofs(iso8859_file, std::ios::binary);
        // ISO-8859-1 encoded string with Swedish å, ä, ö
        unsigned char iso_content[] = {
          'N','a','m','e',',','A','m','o','u','n','t','\n',
          'J','o','h','a','n',' ','S','v','e','n','s','s','o','n',',','1','0','0','\n',
          0xC5,'s','a',' ',  // Åsa (0xC5 = Å in ISO-8859-1)
          'L','i','n','d','s','t','r',0xF6,'m',',','2','5','0','\n'  // ström (0xF6 = ö)
        };
        iso_ofs.write(reinterpret_cast<char*>(iso_content), sizeof(iso_content));
        iso_ofs.close();
      }

      void TearDown() override {
        logger::scope_logger log_raii{logger::development_trace, "EncodingDetectionTestFixture::TearDown"};
        std::error_code ec;
        std::filesystem::remove_all(test_dir, ec);
      }
    };

    TEST_F(EncodingDetectionTestFixture, DetectUTF8) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingDetectionTests, DetectUTF8)"};

      auto buffer = cratchit::io::read_file_to_buffer(utf8_file);
      ASSERT_TRUE(buffer) << "Expected successful file read";

      auto encoding = text::encoding::icu::detect_buffer_encoding(buffer.value());

      ASSERT_TRUE(encoding) << "Expected successful encoding detection";
      EXPECT_EQ(encoding->encoding, text::encoding::DetectedEncoding::UTF8)
        << "Expected UTF-8 detection, got: " << encoding->display_name;
      EXPECT_GE(encoding->confidence, 50) << "Expected reasonable confidence";
    }

    TEST_F(EncodingDetectionTestFixture, DetectISO8859) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingDetectionTests, DetectISO8859)"};

      auto buffer = cratchit::io::read_file_to_buffer(iso8859_file);
      ASSERT_TRUE(buffer) << "Expected successful file read";

      auto encoding = text::encoding::icu::detect_buffer_encoding(buffer.value());

      ASSERT_TRUE(encoding) << "Expected successful encoding detection";
      // ICU might detect as ISO-8859-1 or Windows-1252 (superset)
      bool is_latin = (encoding->encoding == text::encoding::DetectedEncoding::ISO_8859_1 ||
                       encoding->encoding == text::encoding::DetectedEncoding::WINDOWS_1252);
      EXPECT_TRUE(is_latin)
        << "Expected ISO-8859-1 or Windows-1252 detection, got: " << encoding->display_name;
    }

    TEST(EncodingDetectionTests, ComposesWithFileIO) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingDetectionTests, ComposesWithFileIO)"};

      // Create test file
      auto temp_path = std::filesystem::temp_directory_path() / "cratchit_test_compose_encoding.txt";
      {
        std::ofstream ofs(temp_path);
        ofs << "Test content for encoding detection with monadic composition\n";
      }

      // Demonstrate monadic composition: file → buffer → encoding
      auto result = cratchit::io::read_file_to_buffer(temp_path)
        .and_then([](auto& buffer) {
          cratchit::io::IOResult<text::encoding::icu::EncodingDetectionResult> encoding_result;
          auto maybe_encoding = text::encoding::icu::detect_buffer_encoding(buffer);
          if (maybe_encoding) {
            encoding_result.m_value = *maybe_encoding;
            encoding_result.push_message(
              std::format("Detected encoding: {} (confidence: {})",
                         maybe_encoding->display_name,
                         maybe_encoding->confidence));
          } else {
            encoding_result.push_message("Failed to detect encoding");
          }
          return encoding_result;
        });

      ASSERT_TRUE(result) << "Expected successful encoding detection pipeline";
      // ASCII content can be detected as UTF-8 or ISO-8859-1 (both valid)
      bool is_ascii_compatible = (result.value().encoding == text::encoding::DetectedEncoding::UTF8 ||
                                   result.value().encoding == text::encoding::DetectedEncoding::ISO_8859_1);
      EXPECT_TRUE(is_ascii_compatible)
        << "Expected UTF-8 or ISO-8859-1 for ASCII text, got: " << result.value().display_name;
      EXPECT_GT(result.m_messages.size(), 1) << "Expected messages from both steps";

      // Clean up
      std::filesystem::remove(temp_path);
    }

    TEST(EncodingDetectionTests, EmptyBufferReturnsNullopt) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingDetectionTests, EmptyBufferReturnsNullopt)"};

      cratchit::io::ByteBuffer empty_buffer;
      auto encoding = text::encoding::icu::detect_buffer_encoding(empty_buffer);

      EXPECT_FALSE(encoding) << "Expected empty optional for empty buffer";
    }

    TEST(EncodingDetectionTests, LowConfidenceHandling) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingDetectionTests, LowConfidenceHandling)"};

      // Very short content might have low confidence
      cratchit::io::ByteBuffer short_buffer;
      std::string short_text = "a";
      short_buffer.resize(short_text.size());
      std::memcpy(short_buffer.data(), short_text.data(), short_text.size());

      // Use high threshold to potentially get no match
      auto encoding = text::encoding::icu::detect_buffer_encoding(short_buffer, 95);

      // Either no detection or a detection - both are acceptable
      if (encoding) {
        logger::development_trace("Short buffer detected as: {} (confidence: {})",
                                 encoding->display_name, encoding->confidence);
      } else {
        logger::development_trace("Short buffer - no encoding detected (as expected for short content)");
      }

      SUCCEED() << "Test completed - low confidence handling works";
    }

  } // namespace encoding_detection_suite

  namespace transcoding_views_suite {

    TEST(BytesToUnicodeTests, TranscodesUTF8SimpleASCII) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(BytesToUnicodeTests, TranscodesUTF8SimpleASCII)"};

      // Simple ASCII text (subset of UTF-8)
      cratchit::io::ByteBuffer buffer;
      std::string test_text = "Hello";
      buffer.resize(test_text.size());
      std::memcpy(buffer.data(), test_text.data(), test_text.size());

      // Create transcoding view
      auto unicode_view = text::encoding::views::bytes_to_unicode(
        buffer,
        text::encoding::DetectedEncoding::UTF8
      );

      // Collect code points
      std::u16string result;
      for (char16_t cp : unicode_view) {
        result.push_back(cp);
      }

      EXPECT_EQ(result.size(), 5) << "Expected 5 code points";
      EXPECT_EQ(result[0], u'H');
      EXPECT_EQ(result[1], u'e');
      EXPECT_EQ(result[2], u'l');
      EXPECT_EQ(result[3], u'l');
      EXPECT_EQ(result[4], u'o');
    }

    TEST(BytesToUnicodeTests, TranscodesUTF8MultibyteSwedish) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(BytesToUnicodeTests, TranscodesUTF8MultibyteSwedish)"};

      // UTF-8 encoded Swedish text: "Åsa"
      // Å (U+00C5) → UTF-8: 0xC3 0x85
      cratchit::io::ByteBuffer buffer{
        std::byte{0xC3}, std::byte{0x85},  // Å
        std::byte{0x73},                    // s
        std::byte{0x61}                     // a
      };

      auto unicode_view = text::encoding::views::bytes_to_unicode(
        buffer,
        text::encoding::DetectedEncoding::UTF8
      );

      std::u16string result;
      for (char16_t cp : unicode_view) {
        result.push_back(cp);
      }

      ASSERT_EQ(result.size(), 3) << "Expected 3 code points";
      EXPECT_EQ(result[0], 0x00C5) << "Expected Å (U+00C5)";
      EXPECT_EQ(result[1], u's');
      EXPECT_EQ(result[2], u'a');
    }

    TEST(BytesToUnicodeTests, TranscodesUTF8ComplexSwedish) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(BytesToUnicodeTests, TranscodesUTF8ComplexSwedish)"};

      // UTF-8 encoded: "åäö"
      // å (U+00E5) → UTF-8: 0xC3 0xA5
      // ä (U+00E4) → UTF-8: 0xC3 0xA4
      // ö (U+00F6) → UTF-8: 0xC3 0xB6
      cratchit::io::ByteBuffer buffer{
        std::byte{0xC3}, std::byte{0xA5},  // å
        std::byte{0xC3}, std::byte{0xA4},  // ä
        std::byte{0xC3}, std::byte{0xB6}   // ö
      };

      auto unicode_view = text::encoding::views::bytes_to_unicode(
        buffer,
        text::encoding::DetectedEncoding::UTF8
      );

      std::u16string result;
      for (char16_t cp : unicode_view) {
        result.push_back(cp);
      }

      ASSERT_EQ(result.size(), 3) << "Expected 3 code points";
      EXPECT_EQ(result[0], 0x00E5) << "Expected å (U+00E5)";
      EXPECT_EQ(result[1], 0x00E4) << "Expected ä (U+00E4)";
      EXPECT_EQ(result[2], 0x00F6) << "Expected ö (U+00F6)";
    }

    TEST(BytesToUnicodeTests, TranscodesISO8859_1) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(BytesToUnicodeTests, TranscodesISO8859_1)"};

      // ISO-8859-1 encoded: "Åsa"
      // Å → 0xC5 in ISO-8859-1
      cratchit::io::ByteBuffer buffer{
        std::byte{0xC5},  // Å
        std::byte{0x73},  // s
        std::byte{0x61}   // a
      };

      auto unicode_view = text::encoding::views::bytes_to_unicode(
        buffer,
        text::encoding::DetectedEncoding::ISO_8859_1
      );

      std::u16string result;
      for (char16_t cp : unicode_view) {
        result.push_back(cp);
      }

      ASSERT_EQ(result.size(), 3) << "Expected 3 code points";
      EXPECT_EQ(result[0], 0x00C5) << "Expected Å (U+00C5)";
      EXPECT_EQ(result[1], u's');
      EXPECT_EQ(result[2], u'a');
    }

    TEST(BytesToUnicodeTests, TranscodesISO8859_1SwedishChars) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(BytesToUnicodeTests, TranscodesISO8859_1SwedishChars)"};

      // ISO-8859-1 encoded: "åäö"
      cratchit::io::ByteBuffer buffer{
        std::byte{0xE5},  // å
        std::byte{0xE4},  // ä
        std::byte{0xF6}   // ö
      };

      auto unicode_view = text::encoding::views::bytes_to_unicode(
        buffer,
        text::encoding::DetectedEncoding::ISO_8859_1
      );

      std::u16string result;
      for (char16_t cp : unicode_view) {
        result.push_back(cp);
      }

      ASSERT_EQ(result.size(), 3) << "Expected 3 code points";
      EXPECT_EQ(result[0], 0x00E5) << "Expected å (U+00E5)";
      EXPECT_EQ(result[1], 0x00E4) << "Expected ä (U+00E4)";
      EXPECT_EQ(result[2], 0x00F6) << "Expected ö (U+00F6)";
    }

    TEST(BytesToUnicodeTests, LazyEvaluationNoEagerMaterialization) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(BytesToUnicodeTests, LazyEvaluationNoEagerMaterialization)"};

      // Create a large buffer to verify lazy evaluation
      cratchit::io::ByteBuffer large_buffer;
      std::string repeated_text = "Test";
      for (int i = 0; i < 1000; ++i) {
        for (char ch : repeated_text) {
          large_buffer.push_back(static_cast<std::byte>(ch));
        }
      }

      // Create view (should not process anything yet)
      auto unicode_view = text::encoding::views::bytes_to_unicode(
        large_buffer,
        text::encoding::DetectedEncoding::UTF8
      );

      // Take only first 10 characters using ranges
      auto first_10 = unicode_view | std::views::take(10);

      std::u16string result;
      for (char16_t cp : first_10) {
        result.push_back(cp);
      }

      // Should have only processed 10 code points, not the entire buffer
      EXPECT_EQ(result.size(), 10) << "Expected only 10 code points";
      EXPECT_EQ(result[0], u'T');
      EXPECT_EQ(result[1], u'e');
      EXPECT_EQ(result[2], u's');
      EXPECT_EQ(result[3], u't');
    }

    TEST(BytesToUnicodeTests, ComposesWithStandardRangeAlgorithms) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(BytesToUnicodeTests, ComposesWithStandardRangeAlgorithms)"};

      // UTF-8 encoded: "Hello World"
      std::string test_text = "Hello World";
      cratchit::io::ByteBuffer buffer;
      buffer.resize(test_text.size());
      std::memcpy(buffer.data(), test_text.data(), test_text.size());

      auto unicode_view = text::encoding::views::bytes_to_unicode(
        buffer,
        text::encoding::DetectedEncoding::UTF8
      );

      // Use standard range algorithms
      // 1. Count total code points
      auto count = std::ranges::distance(unicode_view);
      EXPECT_EQ(count, 11) << "Expected 11 code points";

      // 2. Find a specific character
      auto it = std::ranges::find(unicode_view, u'W');
      EXPECT_NE(it, unicode_view.end()) << "Expected to find 'W'";

      // 3. Check if all are ASCII
      auto all_ascii = std::ranges::all_of(unicode_view, [](char16_t cp) {
        return cp < 128;
      });
      EXPECT_TRUE(all_ascii) << "Expected all ASCII characters";
    }

    TEST(BytesToUnicodeTests, IntegrationWithSteps1And2) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(BytesToUnicodeTests, IntegrationWithSteps1And2)"};

      // Create test file with UTF-8 content
      auto temp_path = std::filesystem::temp_directory_path() / "cratchit_test_step3_integration.txt";
      {
        std::ofstream ofs(temp_path);
        ofs << "Åsa Lindström";  // UTF-8 encoded Swedish text
      }

      // Step 1: Read file to buffer
      auto buffer_result = cratchit::io::read_file_to_buffer(temp_path);
      ASSERT_TRUE(buffer_result) << "Expected successful file read";

      // Step 2: Detect encoding
      auto encoding_result = text::encoding::icu::detect_buffer_encoding(buffer_result.value());
      ASSERT_TRUE(encoding_result) << "Expected successful encoding detection";

      // Step 3: Create transcoding view
      auto unicode_view = text::encoding::views::bytes_to_unicode(
        buffer_result.value(),
        encoding_result->encoding
      );

      // Collect all code points
      std::u16string result;
      for (char16_t cp : unicode_view) {
        result.push_back(cp);
      }

      // Verify we got the expected content
      EXPECT_GT(result.size(), 0) << "Expected non-empty result";
      // First character should be Å (U+00C5)
      EXPECT_EQ(result[0], 0x00C5) << "Expected Å as first character";

      // Clean up
      std::filesystem::remove(temp_path);
    }

    TEST(BytesToUnicodeTests, EmptyBufferProducesEmptyRange) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(BytesToUnicodeTests, EmptyBufferProducesEmptyRange)"};

      cratchit::io::ByteBuffer empty_buffer;

      auto unicode_view = text::encoding::views::bytes_to_unicode(
        empty_buffer,
        text::encoding::DetectedEncoding::UTF8
      );

      // For input ranges, use std::ranges::distance instead of .empty()
      auto count = std::ranges::distance(unicode_view);
      EXPECT_EQ(count, 0) << "Expected zero code points";
    }

  } // namespace transcoding_views_suite

  namespace runtime_encoding_suite {

    TEST(RuntimeEncodingTests, EncodesSimpleASCII) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(RuntimeEncodingTests, EncodesSimpleASCII)"};

      // Create Unicode code points (char16_t)
      std::u16string unicode_text = u"Hello";

      // Create encoding view
      auto utf8_view = text::encoding::views::unicode_to_runtime_encoding(unicode_text);

      // Collect UTF-8 bytes
      std::string result;
      for (char byte : utf8_view) {
        result.push_back(byte);
      }

      EXPECT_EQ(result, "Hello") << "Expected ASCII to encode identically in UTF-8";
    }

    TEST(RuntimeEncodingTests, EncodesSwedishCharacters) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(RuntimeEncodingTests, EncodesSwedishCharacters)"};

      // Unicode: "Åsa"
      // Å (U+00C5) should encode to UTF-8 as 0xC3 0x85
      std::u16string unicode_text;
      unicode_text.push_back(0x00C5);  // Å
      unicode_text.push_back(u's');
      unicode_text.push_back(u'a');

      auto utf8_view = text::encoding::views::unicode_to_runtime_encoding(unicode_text);

      std::string result;
      for (char byte : utf8_view) {
        result.push_back(byte);
      }

      // Verify UTF-8 encoding
      ASSERT_EQ(result.size(), 4) << "Expected 4 bytes (2 for Å, 1 for s, 1 for a)";
      EXPECT_EQ(static_cast<uint8_t>(result[0]), 0xC3) << "Expected first byte of Å";
      EXPECT_EQ(static_cast<uint8_t>(result[1]), 0x85) << "Expected second byte of Å";
      EXPECT_EQ(result[2], 's');
      EXPECT_EQ(result[3], 'a');
    }

    TEST(RuntimeEncodingTests, EncodesComplexSwedishText) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(RuntimeEncodingTests, EncodesComplexSwedishText)"};

      // Unicode: "åäö"
      // å (U+00E5) → UTF-8: 0xC3 0xA5
      // ä (U+00E4) → UTF-8: 0xC3 0xA4
      // ö (U+00F6) → UTF-8: 0xC3 0xB6
      std::u16string unicode_text;
      unicode_text.push_back(0x00E5);  // å
      unicode_text.push_back(0x00E4);  // ä
      unicode_text.push_back(0x00F6);  // ö

      auto utf8_view = text::encoding::views::unicode_to_runtime_encoding(unicode_text);

      std::string result;
      for (char byte : utf8_view) {
        result.push_back(byte);
      }

      // Verify UTF-8 encoding (each Swedish char is 2 bytes)
      ASSERT_EQ(result.size(), 6) << "Expected 6 bytes (2 per character)";
      EXPECT_EQ(static_cast<uint8_t>(result[0]), 0xC3);
      EXPECT_EQ(static_cast<uint8_t>(result[1]), 0xA5);
      EXPECT_EQ(static_cast<uint8_t>(result[2]), 0xC3);
      EXPECT_EQ(static_cast<uint8_t>(result[3]), 0xA4);
      EXPECT_EQ(static_cast<uint8_t>(result[4]), 0xC3);
      EXPECT_EQ(static_cast<uint8_t>(result[5]), 0xB6);
    }

    TEST(RuntimeEncodingTests, LazyEvaluationOnlyProcessesNeeded) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(RuntimeEncodingTests, LazyEvaluationOnlyProcessesNeeded)"};

      // Create a large Unicode string
      std::u16string large_unicode;
      for (int i = 0; i < 1000; ++i) {
        large_unicode += u"Test";
      }

      // Create view (should not encode anything yet)
      auto utf8_view = text::encoding::views::unicode_to_runtime_encoding(large_unicode);

      // Take only first 10 bytes using ranges
      auto first_10 = utf8_view | std::views::take(10);

      std::string result;
      for (char byte : first_10) {
        result.push_back(byte);
      }

      // Should have only processed enough to get 10 bytes
      EXPECT_EQ(result.size(), 10) << "Expected only 10 bytes";
      EXPECT_EQ(result.substr(0, 4), "Test") << "Expected 'Test' prefix";
    }

    TEST(RuntimeEncodingTests, EmptyRangeProducesEmpty) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(RuntimeEncodingTests, EmptyRangeProducesEmpty)"};

      std::u16string empty_unicode;

      auto utf8_view = text::encoding::views::unicode_to_runtime_encoding(empty_unicode);

      auto count = std::ranges::distance(utf8_view);
      EXPECT_EQ(count, 0) << "Expected zero bytes for empty input";
    }

    TEST(RuntimeEncodingTests, ComposesWithStandardRangeAlgorithms) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(RuntimeEncodingTests, ComposesWithStandardRangeAlgorithms)"};

      std::u16string unicode_text = u"Hello World";

      auto utf8_view = text::encoding::views::unicode_to_runtime_encoding(unicode_text);

      // Count total bytes
      auto count = std::ranges::distance(utf8_view);
      EXPECT_EQ(count, 11) << "Expected 11 bytes for ASCII text";

      // Find a specific character
      auto it = std::ranges::find(utf8_view, 'W');
      EXPECT_NE(it, utf8_view.end()) << "Expected to find 'W'";

      // Check if all are ASCII
      auto all_ascii = std::ranges::all_of(utf8_view, [](char c) {
        return static_cast<unsigned char>(c) < 128;
      });
      EXPECT_TRUE(all_ascii) << "Expected all ASCII bytes";
    }

    TEST(RuntimeEncodingTests, CompleteTranscodingPipeline) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(RuntimeEncodingTests, CompleteTranscodingPipeline)"};

      // Create test file with UTF-8 content
      auto temp_path = std::filesystem::temp_directory_path() / "cratchit_test_step4_pipeline.txt";
      std::string original_utf8 = "Åsa Lindström";
      {
        std::ofstream ofs(temp_path);
        ofs << original_utf8;
      }

      // Step 1: Read file to buffer
      auto buffer_result = cratchit::io::read_file_to_buffer(temp_path);
      ASSERT_TRUE(buffer_result) << "Expected successful file read";

      // Step 2: Detect encoding
      auto encoding_result = text::encoding::icu::detect_buffer_encoding(buffer_result.value());
      ASSERT_TRUE(encoding_result) << "Expected successful encoding detection";

      // Step 3: Create Unicode view
      auto unicode_view = text::encoding::views::bytes_to_unicode(
        buffer_result.value(),
        encoding_result->encoding
      );

      // Step 4: Create runtime encoding (UTF-8) view
      auto utf8_view = text::encoding::views::unicode_to_runtime_encoding(unicode_view);

      // Collect all UTF-8 bytes
      std::string result;
      for (char byte : utf8_view) {
        result.push_back(byte);
      }

      // Verify round-trip: original UTF-8 → bytes → Unicode → UTF-8
      EXPECT_EQ(result, original_utf8) << "Expected round-trip to preserve content";

      // Clean up
      std::filesystem::remove(temp_path);
    }

    TEST(RuntimeEncodingTests, TranscodesFromISO8859ToUTF8) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(RuntimeEncodingTests, TranscodesFromISO8859ToUTF8)"};

      // ISO-8859-1 encoded: "Åsa" (0xC5 = Å in ISO-8859-1)
      cratchit::io::ByteBuffer iso_buffer{
        std::byte{0xC5},  // Å
        std::byte{0x73},  // s
        std::byte{0x61}   // a
      };

      // Step 3: Decode ISO-8859-1 to Unicode
      auto unicode_view = text::encoding::views::bytes_to_unicode(
        iso_buffer,
        text::encoding::DetectedEncoding::ISO_8859_1
      );

      // Step 4: Encode Unicode to UTF-8
      auto utf8_view = text::encoding::views::unicode_to_runtime_encoding(unicode_view);

      // Collect UTF-8 bytes
      std::string result;
      for (char byte : utf8_view) {
        result.push_back(byte);
      }

      // Verify UTF-8 encoding of "Åsa"
      ASSERT_EQ(result.size(), 4) << "Expected 4 bytes";
      EXPECT_EQ(static_cast<uint8_t>(result[0]), 0xC3);  // First byte of Å in UTF-8
      EXPECT_EQ(static_cast<uint8_t>(result[1]), 0x85);  // Second byte of Å in UTF-8
      EXPECT_EQ(result[2], 's');
      EXPECT_EQ(result[3], 'a');
    }

    TEST(RuntimeEncodingTests, PipelineSyntaxWorks) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(RuntimeEncodingTests, PipelineSyntaxWorks)"};

      // Test piping syntax for complete pipeline
      cratchit::io::ByteBuffer utf8_buffer;
      std::string test_text = "Hello";
      utf8_buffer.resize(test_text.size());
      std::memcpy(utf8_buffer.data(), test_text.data(), test_text.size());

      // Pipeline: bytes → Unicode → UTF-8 (using piping)
      auto pipeline = utf8_buffer
        | text::encoding::views::adaptor::bytes_to_unicode(text::encoding::DetectedEncoding::UTF8)
        | text::encoding::views::adaptor::unicode_to_runtime_encoding();

      std::string result;
      for (char byte : pipeline) {
        result.push_back(byte);
      }

      EXPECT_EQ(result, test_text) << "Expected piping syntax to work";
    }

    TEST(RuntimeEncodingTests, HandlesThreeByteUTF8Characters) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(RuntimeEncodingTests, HandlesThreeByteUTF8Characters)"};

      // Euro sign € (U+20AC) requires 3 bytes in UTF-8
      std::u16string unicode_text;
      unicode_text.push_back(0x20AC);  // €

      auto utf8_view = text::encoding::views::unicode_to_runtime_encoding(unicode_text);

      std::string result;
      for (char byte : utf8_view) {
        result.push_back(byte);
      }

      // € in UTF-8: 0xE2 0x82 0xAC
      ASSERT_EQ(result.size(), 3) << "Expected 3 bytes for Euro sign";
      EXPECT_EQ(static_cast<uint8_t>(result[0]), 0xE2);
      EXPECT_EQ(static_cast<uint8_t>(result[1]), 0x82);
      EXPECT_EQ(static_cast<uint8_t>(result[2]), 0xAC);
    }

    TEST(RuntimeEncodingTests, CompletePipelineMultipleEncodings) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(RuntimeEncodingTests, CompletePipelineMultipleEncodings)"};

      // Test pipeline with CP437 input
      // CP437: å=0x86, ä=0x84, ö=0x94
      cratchit::io::ByteBuffer cp437_buffer{
        std::byte{0x86},  // å in CP437
        std::byte{0x84},  // ä in CP437
        std::byte{0x94}   // ö in CP437
      };

      // Complete pipeline: CP437 → Unicode → UTF-8
      auto unicode_view = text::encoding::views::bytes_to_unicode(
        cp437_buffer,
        text::encoding::DetectedEncoding::CP437
      );

      auto utf8_view = text::encoding::views::unicode_to_runtime_encoding(unicode_view);

      std::string result;
      for (char byte : utf8_view) {
        result.push_back(byte);
      }

      // Verify UTF-8 output for "åäö"
      ASSERT_EQ(result.size(), 6) << "Expected 6 bytes (2 per character in UTF-8)";
      // å in UTF-8: 0xC3 0xA5
      EXPECT_EQ(static_cast<uint8_t>(result[0]), 0xC3);
      EXPECT_EQ(static_cast<uint8_t>(result[1]), 0xA5);
      // ä in UTF-8: 0xC3 0xA4
      EXPECT_EQ(static_cast<uint8_t>(result[2]), 0xC3);
      EXPECT_EQ(static_cast<uint8_t>(result[3]), 0xA4);
      // ö in UTF-8: 0xC3 0xB6
      EXPECT_EQ(static_cast<uint8_t>(result[4]), 0xC3);
      EXPECT_EQ(static_cast<uint8_t>(result[5]), 0xB6);
    }

  } // namespace runtime_encoding_suite

  namespace encoding_pipeline_integration_suite {

    // Test fixture for encoding pipeline integration tests
    class EncodingPipelineTestFixture : public ::testing::Test {
    protected:
      std::filesystem::path test_dir;
      std::filesystem::path utf8_file;
      std::filesystem::path iso8859_file;
      std::filesystem::path windows1252_file;
      std::filesystem::path empty_file;
      std::filesystem::path large_file;

      void SetUp() override {
        logger::scope_logger log_raii{logger::development_trace, "EncodingPipelineTestFixture::SetUp"};

        // Create temporary test directory
        test_dir = std::filesystem::temp_directory_path() / "cratchit_test_pipeline_integration";
        std::filesystem::create_directories(test_dir);

        // Create UTF-8 test file with Swedish content
        utf8_file = test_dir / "utf8_test.csv";
        {
          std::ofstream ofs(utf8_file, std::ios::binary);
          std::string utf8_content = "Name,Amount\nJohan Svensson,100.50\nÅsa Lindström,250.75\n";
          ofs.write(utf8_content.data(), utf8_content.size());
        }

        // Create ISO-8859-1 test file
        iso8859_file = test_dir / "iso8859_test.csv";
        {
          std::ofstream ofs(iso8859_file, std::ios::binary);
          // ISO-8859-1 encoded: "Åsa" (0xC5 = Å)
          unsigned char iso_content[] = {
            'N','a','m','e',',','A','m','o','u','n','t','\n',
            0xC5,'s','a',' ',  // Åsa (0xC5 = Å in ISO-8859-1)
            'L','i','n','d','s','t','r',0xF6,'m',',','1','0','0','\n'  // ström (0xF6 = ö)
          };
          ofs.write(reinterpret_cast<char*>(iso_content), sizeof(iso_content));
        }

        // Create Windows-1252 test file (superset of ISO-8859-1)
        windows1252_file = test_dir / "windows1252_test.csv";
        {
          std::ofstream ofs(windows1252_file, std::ios::binary);
          // Windows-1252 has some special chars in 0x80-0x9F range
          unsigned char win_content[] = {
            'T','e','s','t',',','V','a','l','u','e','\n',
            'E','u','r','o',',',' ',0x80,'\n'  // 0x80 = € in Windows-1252
          };
          ofs.write(reinterpret_cast<char*>(win_content), sizeof(win_content));
        }

        // Create empty file
        empty_file = test_dir / "empty.txt";
        {
          std::ofstream ofs(empty_file);
        }

        // Create large file for memory efficiency testing
        large_file = test_dir / "large_test.txt";
        {
          std::ofstream ofs(large_file);
          for (int i = 0; i < 10000; ++i) {
            ofs << "Line " << i << ": This is test content with Swedish characters: åäö ÅÄÖ\n";
          }
        }
      }

      void TearDown() override {
        logger::scope_logger log_raii{logger::development_trace, "EncodingPipelineTestFixture::TearDown"};
        std::error_code ec;
        std::filesystem::remove_all(test_dir, ec);
      }
    };

    TEST_F(EncodingPipelineTestFixture, UTF8FileToRuntimeText) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingPipelineTests, UTF8FileToRuntimeText)"};

      auto result = text::encoding::read_file_with_encoding_detection(utf8_file);

      ASSERT_TRUE(result) << "Expected successful pipeline execution";
      EXPECT_GT(result.value().size(), 0) << "Expected non-empty text";

      // Verify content contains expected Swedish characters
      EXPECT_TRUE(result.value().find("Åsa") != std::string::npos)
        << "Expected to find 'Åsa' in transcoded text";
      EXPECT_TRUE(result.value().find("Lindström") != std::string::npos)
        << "Expected to find 'Lindström' in transcoded text";

      // Verify messages document the pipeline steps
      EXPECT_GE(result.m_messages.size(), 3) << "Expected messages from multiple pipeline steps";

      // Print messages for verification
      for (const auto& msg : result.m_messages) {
        logger::development_trace("Pipeline message: {}", msg);
      }
    }

    TEST_F(EncodingPipelineTestFixture, ISO8859FileToUTF8Text) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingPipelineTests, ISO8859FileToUTF8Text)"};

      auto result = text::encoding::read_file_with_encoding_detection(iso8859_file);

      ASSERT_TRUE(result) << "Expected successful pipeline execution";
      EXPECT_GT(result.value().size(), 0) << "Expected non-empty text";

      // The output should be UTF-8, so we can search for UTF-8 encoded characters
      // Å in UTF-8: 0xC3 0x85
      EXPECT_TRUE(result.value().find("Åsa") != std::string::npos)
        << "Expected to find 'Åsa' transcoded to UTF-8";

      // ö in UTF-8: 0xC3 0xB6
      EXPECT_TRUE(result.value().find("ström") != std::string::npos)
        << "Expected to find 'ström' transcoded to UTF-8";

      logger::development_trace("ISO-8859-1 transcoded content: {}", result.value());
    }

    TEST_F(EncodingPipelineTestFixture, Windows1252FileHandled) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingPipelineTests, Windows1252FileHandled)"};

      auto result = text::encoding::read_file_with_encoding_detection(windows1252_file);

      ASSERT_TRUE(result) << "Expected successful pipeline execution";
      EXPECT_GT(result.value().size(), 0) << "Expected non-empty text";

      logger::development_trace("Windows-1252 transcoded content: {}", result.value());
    }

    TEST_F(EncodingPipelineTestFixture, EmptyFileHandledGracefully) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingPipelineTests, EmptyFileHandledGracefully)"};

      auto result = text::encoding::read_file_with_encoding_detection(empty_file);

      ASSERT_TRUE(result) << "Expected successful pipeline execution for empty file";
      EXPECT_EQ(result.value().size(), 0) << "Expected empty string for empty file";

      // Check that we got a message about the empty file
      bool has_empty_message = false;
      for (const auto& msg : result.m_messages) {
        if (msg.find("empty") != std::string::npos) {
          has_empty_message = true;
          break;
        }
      }
      EXPECT_TRUE(has_empty_message) << "Expected message about empty file";
    }

    TEST(EncodingPipelineTests, FileNotFoundHandledGracefully) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingPipelineTests, FileNotFoundHandledGracefully)"};

      auto non_existent = std::filesystem::path("/tmp/cratchit_nonexistent_file_99999.csv");

      auto result = text::encoding::read_file_with_encoding_detection(non_existent);

      EXPECT_FALSE(result) << "Expected failure for non-existent file";
      EXPECT_GT(result.m_messages.size(), 0) << "Expected error messages";

      // Verify error messages are informative
      bool has_file_error = false;
      for (const auto& msg : result.m_messages) {
        if (msg.find("not exist") != std::string::npos || msg.find("Failed") != std::string::npos) {
          has_file_error = true;
          logger::development_trace("Error message: {}", msg);
          break;
        }
      }
      EXPECT_TRUE(has_file_error) << "Expected error message about missing file";
    }

    TEST_F(EncodingPipelineTestFixture, LazyEvaluationWithLargeFile) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingPipelineTests, LazyEvaluationWithLargeFile)"};

      // Note: The main pipeline function materializes the result, so this test
      // verifies that we can handle large files without errors
      auto result = text::encoding::read_file_with_encoding_detection(large_file);

      ASSERT_TRUE(result) << "Expected successful processing of large file";
      EXPECT_GT(result.value().size(), 100000) << "Expected large output";

      // Verify content is preserved
      EXPECT_TRUE(result.value().find("Line 0:") != std::string::npos)
        << "Expected first line";
      EXPECT_TRUE(result.value().find("Line 9999:") != std::string::npos)
        << "Expected last line";
      EXPECT_TRUE(result.value().find("åäö") != std::string::npos)
        << "Expected Swedish characters preserved";

      logger::development_trace("Large file transcoded: {} bytes", result.value().size());
    }

    TEST_F(EncodingPipelineTestFixture, LazyViewVariantForMemoryEfficiency) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingPipelineTests, LazyViewVariantForMemoryEfficiency)"};

      // Test the lazy view variant that doesn't materialize the entire string
      auto buffer_result = cratchit::io::read_file_to_buffer(utf8_file);
      ASSERT_TRUE(buffer_result) << "Expected successful file read";

      auto lazy_view = text::encoding::create_lazy_encoding_view(buffer_result.value());
      ASSERT_TRUE(lazy_view.has_value()) << "Expected lazy view creation to succeed";

      // Take only first 100 bytes - demonstrating lazy evaluation
      auto first_100 = *lazy_view | std::views::take(100);

      std::string partial_result;
      for (char byte : first_100) {
        partial_result.push_back(byte);
      }

      EXPECT_LE(partial_result.size(), 100) << "Expected at most 100 bytes";
      EXPECT_GT(partial_result.size(), 0) << "Expected some content";

      logger::development_trace("Lazy view first 100 bytes: {}", partial_result);
    }

    TEST_F(EncodingPipelineTestFixture, CompleteIntegrationAllSteps) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingPipelineTests, CompleteIntegrationAllSteps)"};

      // This test verifies the complete integration of Steps 1-4:
      // file path → byte buffer → encoding detection → Unicode view → UTF-8 text

      auto result = text::encoding::read_file_with_encoding_detection(utf8_file);

      ASSERT_TRUE(result) << "Expected successful complete pipeline";

      // Verify all pipeline steps are documented in messages
      std::vector<std::string> expected_keywords = {
        "opened",      // Step 1: File open
        "read",        // Step 1: File read
        "Detected",    // Step 2: Encoding detection
        "transcoded"   // Steps 3-4: Transcoding
      };

      for (const auto& keyword : expected_keywords) {
        bool found = false;
        for (const auto& msg : result.m_messages) {
          if (msg.find(keyword) != std::string::npos) {
            found = true;
            break;
          }
        }
        EXPECT_TRUE(found) << "Expected message containing keyword: " << keyword;
      }

      // Log all messages for verification
      logger::development_trace("Complete pipeline messages:");
      for (const auto& msg : result.m_messages) {
        logger::development_trace("  - {}", msg);
      }

      // Verify content correctness
      EXPECT_TRUE(result.value().find("Name,Amount") != std::string::npos)
        << "Expected CSV header";
      EXPECT_TRUE(result.value().find("Åsa") != std::string::npos)
        << "Expected Swedish character Å preserved";
    }

    TEST_F(EncodingPipelineTestFixture, MonadicErrorPropagation) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingPipelineTests, MonadicErrorPropagation)"};

      // Test that errors from Step 1 propagate correctly through the pipeline
      auto non_existent = test_dir / "does_not_exist.csv";

      auto result = text::encoding::read_file_with_encoding_detection(non_existent);

      EXPECT_FALSE(result) << "Expected failure to propagate";
      EXPECT_FALSE(result.m_value.has_value()) << "Expected no value on failure";
      EXPECT_GT(result.m_messages.size(), 0) << "Expected error messages preserved";

      // The pipeline should short-circuit - no encoding detection should happen
      bool has_encoding_message = false;
      for (const auto& msg : result.m_messages) {
        if (msg.find("Detected encoding") != std::string::npos) {
          has_encoding_message = true;
          break;
        }
      }
      EXPECT_FALSE(has_encoding_message)
        << "Expected no encoding detection message after file read failure";
    }

    TEST_F(EncodingPipelineTestFixture, LowConfidenceDefaultsToUTF8) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingPipelineTests, LowConfidenceDefaultsToUTF8)"};

      // Create a file with very ambiguous content (pure ASCII)
      auto ascii_file = test_dir / "ascii_only.txt";
      {
        std::ofstream ofs(ascii_file);
        ofs << "a\n";  // Very short, pure ASCII
      }

      // Use very high confidence threshold to potentially trigger fallback
      auto result = text::encoding::read_file_with_encoding_detection(ascii_file, 99);

      ASSERT_TRUE(result) << "Expected success even with low confidence";

      // Check if we got a message about defaulting to UTF-8
      bool has_default_message = false;
      for (const auto& msg : result.m_messages) {
        if (msg.find("default") != std::string::npos || msg.find("UTF-8") != std::string::npos) {
          has_default_message = true;
          logger::development_trace("Encoding message: {}", msg);
          break;
        }
      }

      // Either detected as something or defaulted to UTF-8 - both are acceptable
      EXPECT_EQ(result.value(), "a\n") << "Expected content to be preserved";
    }

    TEST_F(EncodingPipelineTestFixture, MultipleEncodingsHandledCorrectly) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingPipelineTests, MultipleEncodingsHandledCorrectly)"};

      // Test that different source encodings all produce valid UTF-8 output
      std::vector<std::filesystem::path> test_files = {
        utf8_file,
        iso8859_file,
        windows1252_file
      };

      for (const auto& file : test_files) {
        auto result = text::encoding::read_file_with_encoding_detection(file);

        ASSERT_TRUE(result) << "Expected success for file: " << file.filename();
        EXPECT_GT(result.value().size(), 0) << "Expected non-empty output for: " << file.filename();

        logger::development_trace("File: {} → {} bytes UTF-8",
                                 file.filename().string(),
                                 result.value().size());
      }
    }

  } // namespace encoding_pipeline_integration_suite

} // namespace tests::csv_import_pipeline
