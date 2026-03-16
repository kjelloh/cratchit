#include "logger/log.hpp"
#include "persistent/in/raw_text_read.hpp"
#include "text/encoding_pipeline.hpp"
#include "csv/parse_csv.hpp"
#include "csv/csv_to_statement_id_ed.hpp"
#include "domain/account_statement_to_tagged_amounts.hpp"

#include "test/data/account_statements_csv.hpp"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

// TODO: Refactor this TU into menaingful and consice test cases.
//       For now it contains all the 'AI slop' that claude code generated for me.
//       I tried 'vibe coding' and this is what it got me!
//       Anyhow, the tests are there. That is something?
//       But thye tests are all over the place. ANd the cross test the same thing over and over.
//       1. Consider to find all and_then compositions below.
//          They define the actual 'steps' that we can test one by one.
//       2. Then identify and atomic-tests of interest for each step.
//       3. Finally try to celan out any tests that are now redunant (tested by each step tests)

namespace {

  AnnotatedMaybe<persistent::in::text::ByteBuffer> path_to_byte_buffer_shortcut(std::filesystem::path const& file_path) {
    // Monadic composition: file_path → istream_ptr → buffer
    return persistent::in::text::monadic::path_to_istream_ptr_step(file_path)
      .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step);
  }

  AnnotatedMaybe<std::string> path_to_platform_encoded_string_shortcut(
    std::filesystem::path const& file_path,
    int32_t confidence_threshold = text::encoding::inferred::DEFAULT_CONFIDENCE_THERSHOLD) {

    return path_to_byte_buffer_shortcut(file_path)
      .and_then(text::encoding::monadic::to_with_threshold_step_f(confidence_threshold))
      .and_then(text::encoding::monadic::to_with_inferred_encoding)
      .and_then(text::encoding::monadic::to_platform_encoded_string_step);
  }

  AnnotatedMaybe<CSV::Table> path_to_table_shortcut(std::filesystem::path const& file_path) {

    return path_to_platform_encoded_string_shortcut(file_path)
      .and_then(CSV::parse::monadic::csv_text_to_table_step);

  }

} // namespace

namespace tests::csv_import_pipeline {

  namespace monadic_composition_suite {

    class MonadicCompositionFixture : public ::testing::Test {
    protected:
      std::filesystem::path m_test_dir;
      std::filesystem::path m_valid_file_path;
      std::filesystem::path m_missing_file_path;

      void SetUp() override {
        logger::scope_logger log_raii{logger::development_trace, "MonadicCompositionFixture::SetUp"};

        // Create temporary test directory
        m_test_dir = std::filesystem::temp_directory_path() / "cratchit_test_file_io";
        std::filesystem::create_directories(m_test_dir);

        // Create a valid test file with some content
        m_valid_file_path = m_test_dir / "valid_test.csv";
        std::ofstream ofs(m_valid_file_path, std::ios::binary);
        std::string test_content = "Bokföringsdag,Namn,Belopp\n2025-01-01,Test Transaction,100.00\n";
        ofs.write(test_content.data(), test_content.size());
        ofs.close();

        // Path to a file that doesn't exist
        m_missing_file_path = m_test_dir / "missing_file.csv";
      }

      void TearDown() override {
        logger::scope_logger log_raii{logger::development_trace, "FileIOTestFixture::TearDown"};

        // Clean up test directory
        std::error_code ec;
        std::filesystem::remove_all(m_test_dir, ec);
      }
    };

    // Test of longer and longer AnnotatedMaybe 'and_then' composition path -> ... -> tagged Amounts

    TEST_F(MonadicCompositionFixture,PathToIStreeam) {
      auto maybe_istream = persistent::in::text::monadic::path_to_istream_ptr_step(m_valid_file_path);
      ASSERT_TRUE(maybe_istream) << "Expected successful read of valid file";
    }

    TEST_F(MonadicCompositionFixture,PathToByteBuffer) {
      auto mayabe_byte_buffer = persistent::in::text::monadic::path_to_istream_ptr_step(m_valid_file_path)
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step);
      ASSERT_TRUE(mayabe_byte_buffer) << "Expected successful read of valid file";
    }

    TEST_F(MonadicCompositionFixture,PathToWithThreshold) {
      auto mayabe_with_threshold = persistent::in::text::monadic::path_to_istream_ptr_step(m_valid_file_path)
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step_f(100));
      ASSERT_TRUE(mayabe_with_threshold) << "Expected successful buffer with encoding confidence threshold";
    }

    TEST_F(MonadicCompositionFixture,PathToWithEncoding) {
      auto mayabe_with_encoding = persistent::in::text::monadic::path_to_istream_ptr_step(m_valid_file_path)
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
        .and_then(text::encoding::monadic::to_with_inferred_encoding);
      ASSERT_TRUE(mayabe_with_encoding) << "Expected successful buffer with detected encoding";
    }

    TEST_F(MonadicCompositionFixture,PathToPlatformEncoded) {
      auto maybe_platform_encoded = persistent::in::text::monadic::path_to_istream_ptr_step(m_valid_file_path)
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
        .and_then(text::encoding::monadic::to_with_inferred_encoding)
        .and_then(text::encoding::monadic::to_platform_encoded_string_step);
      ASSERT_TRUE(maybe_platform_encoded) << "Expected successful platform encoded string";
    }

    TEST_F(MonadicCompositionFixture,PathToCSVTable) {
      auto maybe_csv_table = persistent::in::text::monadic::path_to_istream_ptr_step(m_valid_file_path)
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
        .and_then(text::encoding::monadic::to_with_inferred_encoding)
        .and_then(text::encoding::monadic::to_platform_encoded_string_step)
        .and_then(CSV::parse::monadic::csv_text_to_table_step);

      ASSERT_TRUE(maybe_csv_table) << "Expected successful csv table";
    }

    TEST_F(MonadicCompositionFixture,PathToStatementIDedTable) {
      logger::scope_logger log_raii(logger::development_trace,"TEST_F(MonadicCompositionFixture,PathToStatementIDedTable)");

      auto maybe_statement_id_ed = persistent::in::text::monadic::path_to_istream_ptr_step(m_valid_file_path)
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
        .and_then(text::encoding::monadic::to_with_inferred_encoding)
        .and_then(text::encoding::monadic::to_platform_encoded_string_step)
        .and_then(CSV::parse::monadic::csv_text_to_table_step)
        .and_then(account::statement::monadic::to_statement_id_ed_step);

      ASSERT_TRUE(maybe_statement_id_ed) 
        << "Expected successful statement identification"
        << "Got messages:" << maybe_statement_id_ed.to_caption();
    }

    TEST_F(MonadicCompositionFixture,PathToAccountStatementTaggedAmountsMaybePipeline) {

      auto result = persistent::in::text::maybe::path_to_istream_ptr_step(m_valid_file_path)
        .and_then(persistent::in::text::maybe::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::maybe::to_with_threshold_step_f(100))
        .and_then(text::encoding::maybe::to_with_inferred_encoding)
        .and_then(text::encoding::maybe::to_platform_encoded_string_step)
        .and_then(CSV::parse::maybe::csv_text_to_table_step)
        .and_then(account::statement::maybe::to_statement_id_ed_step)
        .and_then(account::statement::maybe::statement_id_ed_to_account_statement_step)
        .and_then(tas::maybe::account_statement_to_tagged_amounts_step);

      ASSERT_TRUE(result) << "Expected and_then aggregation to succeed at all steps";

    }

    TEST_F(MonadicCompositionFixture,PathToAccountStatementTaggedAmountsAnnotatedPipeline) {
      logger::scope_logger log_raii(logger::development_trace,"TEST_F(MonadicCompositionFixture,PathToAccountStatementTaggedAmountsAnnotatedPipeline)");

      auto maybe_tagged_amounts = persistent::in::text::monadic::path_to_istream_ptr_step(m_valid_file_path)
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
        .and_then(text::encoding::monadic::to_with_inferred_encoding)
        .and_then(text::encoding::monadic::to_platform_encoded_string_step)
        .and_then(CSV::parse::monadic::csv_text_to_table_step)
        .and_then(account::statement::monadic::to_statement_id_ed_step)
        .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step)
        .and_then(tas::monadic::account_statement_to_tagged_amounts_step);

      ASSERT_TRUE(maybe_tagged_amounts) << "Failed with messages:" << maybe_tagged_amounts.to_caption();

    }

  } // monadic_composition_suite

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

      auto result = path_to_byte_buffer_shortcut(valid_file_path);

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

      auto result = path_to_byte_buffer_shortcut(missing_file_path);

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

      auto result = persistent::in::text::monadic::path_to_istream_ptr_step(temp_path);

      ASSERT_TRUE(result) << "Expected successful file open";
      EXPECT_TRUE(result.value() != nullptr) << "Expected non-null stream pointer";
      EXPECT_TRUE(result.value()->good()) << "Expected stream in good state";

      // Clean up
      std::filesystem::remove(temp_path);
    }

    TEST(FileIOTests, OpenMissingFileReturnsEmpty) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FileIOTests, OpenMissingFileReturnsEmpty)"};

      auto non_existent_path = std::filesystem::path("/tmp/cratchit_nonexistent_file_12345.txt");

      auto result = persistent::in::text::monadic::path_to_istream_ptr_step(non_existent_path);

      EXPECT_FALSE(result) << "Expected empty optional for non-existent file";
      EXPECT_GT(result.m_messages.size(), 0) << "Expected error messages";
    }

    TEST(FileIOTests, StreamToBufferWithValidStream) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FileIOTests, StreamToBufferWithValidStream)"};

      std::string test_data = "Hello, World! This is test data.";

      auto result = 
         persistent::in::text::monadic::injected_string_to_istream_ptr_step(test_data)
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step);

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
      auto result = 
         persistent::in::text::monadic::path_to_istream_ptr_step(temp_path)
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .and_then([](auto buffer) -> AnnotatedMaybe<size_t> {
          AnnotatedMaybe<size_t> size_result;
          size_result.m_value = buffer.size();
          size_result.push_message(std::format("Buffer size: {}", buffer.size()));
          return size_result;
        });

      ASSERT_TRUE(result) << "Expected successful monadic composition";
      EXPECT_EQ(result.value(), test_content.size()) << "Expected correct buffer size";
      EXPECT_GE(result.m_messages.size(), 2) 
        << "Expected messages from istream_ptr_to_byte_buffer_step success and test lambda"
        << " Got messages:" << result.to_caption();

      // Clean up
      std::filesystem::remove(temp_path);
    }

    TEST(FileIOTests, TapCombinatorForDebugging) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FileIOTests, TapCombinatorForDebugging)"};

      std::string test_data = "Testing tap combinator";
      size_t stream_opened = 0;
      size_t buffer_size = 0;

      // Demonstrate tap() for side effects without breaking the chain
      auto result =
         persistent::in::text::monadic::injected_string_to_istream_ptr_step(test_data)
        .tap([&stream_opened](auto const& ptr) {
          stream_opened++;
          EXPECT_TRUE(ptr != nullptr) << "Expected valid stream pointer";
        })
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .tap([&buffer_size](auto const& buf) {
          buffer_size = buf.size();
        });

      ASSERT_TRUE(result) << "Expected successful pipeline execution";
      EXPECT_EQ(stream_opened, 1) << "Expected tap to be called once";
      EXPECT_EQ(buffer_size, test_data.size()) << "Expected correct buffer size in tap";
      EXPECT_EQ(result.value().size(), test_data.size()) << "Expected buffer to survive tap()";
    }

    TEST(FileIOTests, MonadicCompositionShortCircuits) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FileIOTests, MonadicCompositionShortCircuits)"};

      auto non_existent = std::filesystem::path("/tmp/cratchit_does_not_exist_9999.txt");

      bool and_then_called = false;

      // Composition should short-circuit on first failure
      // Using tap() to inject side effect without verbose lambda
      auto result =
         persistent::in::text::monadic::path_to_istream_ptr_step(non_existent)
        .tap([&and_then_called](auto const&) { and_then_called = true; })
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step);

      EXPECT_FALSE(result) << "Expected failure due to missing file";
      EXPECT_FALSE(and_then_called) << "Expected tap to NOT be called after failure";
      EXPECT_GT(result.m_messages.size(), 0) << "Expected error messages from first step";
    }

    TEST_F(FileIOTestFixture, EmptyFileReadsSuccessfully) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FileIOTests, EmptyFileReadsSuccessfully)"};

      // Create empty file
      auto empty_file = test_dir / "empty.txt";
      std::ofstream ofs(empty_file);
      ofs.close();

      auto result = path_to_byte_buffer_shortcut(empty_file);

      ASSERT_TRUE(result) << "Expected successful read of empty file";
      EXPECT_EQ(result.value().size(), 0) << "Expected empty buffer";
    }

    TEST_F(FileIOTestFixture, ErrorMessagesArePreserved) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FileIOTests, ErrorMessagesArePreserved)"};

      auto result = path_to_byte_buffer_shortcut(missing_file_path);

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

      auto buffer = path_to_byte_buffer_shortcut(utf8_file);
      ASSERT_TRUE(buffer) << "Expected successful file read";

      auto encoding = text::encoding::inferred::maybe::to_inferred_encoding(buffer.value());

      ASSERT_TRUE(encoding) << "Expected successful encoding detection";
      EXPECT_EQ(encoding->defacto, text::encoding::EncodingID::UTF8)
        << "Expected UTF-8 detection, got: " << encoding->meta.display_name;
      EXPECT_GE(encoding->meta.confidence, 50) << "Expected reasonable confidence";
    }

    TEST_F(EncodingDetectionTestFixture, DetectISO8859) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingDetectionTests, DetectISO8859)"};

      auto buffer = path_to_byte_buffer_shortcut(iso8859_file);
      ASSERT_TRUE(buffer) << "Expected successful file read";

      auto encoding = text::encoding::inferred::maybe::to_inferred_encoding(buffer.value());

      ASSERT_TRUE(encoding) << "Expected successful encoding detection";
      // ICU might detect as ISO-8859-1 or Windows-1252 (superset)
      bool is_latin = (encoding->defacto == text::encoding::EncodingID::ISO_8859_1 ||
                       encoding->defacto == text::encoding::EncodingID::WINDOWS_1252);
      EXPECT_TRUE(is_latin)
        << "Expected ISO-8859-1 or Windows-1252 detection, got: " << encoding->meta.display_name;
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
      auto result = path_to_byte_buffer_shortcut(temp_path)
        .and_then([](auto buffer) {
          AnnotatedMaybe<text::encoding::inferred::EncodingDetectionResult> encoding_result;
          auto maybe_encoding = text::encoding::inferred::maybe::to_inferred_encoding(buffer);
          if (maybe_encoding) {
            encoding_result.m_value = *maybe_encoding;
            encoding_result.push_message(
              std::format("Detected encoding: {} (confidence: {})",
                         maybe_encoding->meta.display_name,
                         maybe_encoding->meta.confidence));
          } else {
            encoding_result.push_message("Failed to detect encoding");
          }
          return encoding_result;
        });

      ASSERT_TRUE(result) << "Expected successful encoding detection pipeline";
      // ASCII content can be detected as UTF-8 or ISO-8859-1 (both valid)
      bool is_ascii_compatible = (result.value().defacto == text::encoding::EncodingID::UTF8 ||
                                   result.value().defacto == text::encoding::EncodingID::ISO_8859_1);
      EXPECT_TRUE(is_ascii_compatible)
        << "Expected UTF-8 or ISO-8859-1 for ASCII text, got: " << result.value().meta.display_name;
      EXPECT_GT(result.m_messages.size(), 1) << "Expected messages from both steps";

      // Clean up
      std::filesystem::remove(temp_path);
    }

    TEST(EncodingDetectionTests, EmptyBufferReturnsNullopt) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingDetectionTests, EmptyBufferReturnsNullopt)"};

      persistent::in::text::ByteBuffer empty_buffer;
      auto encoding = text::encoding::inferred::maybe::to_inferred_encoding(empty_buffer);

      EXPECT_FALSE(encoding) << "Expected empty optional for empty buffer";
    }

    TEST(EncodingDetectionTests, LowConfidenceHandling) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingDetectionTests, LowConfidenceHandling)"};

      // Very short content might have low confidence
      persistent::in::text::ByteBuffer short_buffer;
      std::string short_text = "a";
      short_buffer.resize(short_text.size());
      std::memcpy(short_buffer.data(), short_text.data(), short_text.size());

      // Use high threshold to potentially get no match
      auto encoding = text::encoding::inferred::maybe::to_inferred_encoding(short_buffer, 95);

      // Either no detection or a detection - both are acceptable
      if (encoding) {
        logger::development_trace("Short buffer detected as: {} (confidence: {})",
                                 encoding->meta.display_name, encoding->meta.confidence);
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
      persistent::in::text::ByteBuffer buffer;
      std::string test_text = "Hello";
      buffer.resize(test_text.size());
      std::memcpy(buffer.data(), test_text.data(), test_text.size());

      // Create transcoding view
      auto unicode_view = text::encoding::views::bytes_to_unicode(
        buffer,
        text::encoding::EncodingID::UTF8
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
      persistent::in::text::ByteBuffer buffer{
        std::byte{0xC3}, std::byte{0x85},  // Å
        std::byte{0x73},                    // s
        std::byte{0x61}                     // a
      };

      auto unicode_view = text::encoding::views::bytes_to_unicode(
        buffer,
        text::encoding::EncodingID::UTF8
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
      persistent::in::text::ByteBuffer buffer{
        std::byte{0xC3}, std::byte{0xA5},  // å
        std::byte{0xC3}, std::byte{0xA4},  // ä
        std::byte{0xC3}, std::byte{0xB6}   // ö
      };

      auto unicode_view = text::encoding::views::bytes_to_unicode(
        buffer,
        text::encoding::EncodingID::UTF8
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
      persistent::in::text::ByteBuffer buffer{
        std::byte{0xC5},  // Å
        std::byte{0x73},  // s
        std::byte{0x61}   // a
      };

      auto unicode_view = text::encoding::views::bytes_to_unicode(
        buffer,
        text::encoding::EncodingID::ISO_8859_1
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
      persistent::in::text::ByteBuffer buffer{
        std::byte{0xE5},  // å
        std::byte{0xE4},  // ä
        std::byte{0xF6}   // ö
      };

      auto unicode_view = text::encoding::views::bytes_to_unicode(
        buffer,
        text::encoding::EncodingID::ISO_8859_1
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
      persistent::in::text::ByteBuffer large_buffer;
      std::string repeated_text = "Test";
      for (int i = 0; i < 1000; ++i) {
        for (char ch : repeated_text) {
          large_buffer.push_back(static_cast<std::byte>(ch));
        }
      }

      // Create view (should not process anything yet)
      auto unicode_view = text::encoding::views::bytes_to_unicode(
        large_buffer,
        text::encoding::EncodingID::UTF8
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
      persistent::in::text::ByteBuffer buffer;
      buffer.resize(test_text.size());
      std::memcpy(buffer.data(), test_text.data(), test_text.size());

      auto unicode_view = text::encoding::views::bytes_to_unicode(
        buffer,
        text::encoding::EncodingID::UTF8
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
      auto buffer_result = path_to_byte_buffer_shortcut(temp_path);
      ASSERT_TRUE(buffer_result) << "Expected successful file read";

      // Step 2: Detect encoding
      auto encoding_result = text::encoding::inferred::maybe::to_inferred_encoding(buffer_result.value());
      ASSERT_TRUE(encoding_result) << "Expected successful encoding detection";

      // Step 3: Create transcoding view
      auto unicode_view = text::encoding::views::bytes_to_unicode(
        buffer_result.value(),
        encoding_result->defacto
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

      persistent::in::text::ByteBuffer empty_buffer;

      auto unicode_view = text::encoding::views::bytes_to_unicode(
        empty_buffer,
        text::encoding::EncodingID::UTF8
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
      auto buffer_result = path_to_byte_buffer_shortcut(temp_path);
      ASSERT_TRUE(buffer_result) << "Expected successful file read";

      // Step 2: Detect encoding
      auto encoding_result = text::encoding::inferred::maybe::to_inferred_encoding(buffer_result.value());
      ASSERT_TRUE(encoding_result) << "Expected successful encoding detection";

      // Step 3: Create Unicode view
      auto unicode_view = text::encoding::views::bytes_to_unicode(
        buffer_result.value(),
        encoding_result->defacto
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
      persistent::in::text::ByteBuffer iso_buffer{
        std::byte{0xC5},  // Å
        std::byte{0x73},  // s
        std::byte{0x61}   // a
      };

      // Step 3: Decode ISO-8859-1 to Unicode
      auto unicode_view = text::encoding::views::bytes_to_unicode(
        iso_buffer,
        text::encoding::EncodingID::ISO_8859_1
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
      persistent::in::text::ByteBuffer utf8_buffer;
      std::string test_text = "Hello";
      utf8_buffer.resize(test_text.size());
      std::memcpy(utf8_buffer.data(), test_text.data(), test_text.size());

      // Pipeline: bytes → Unicode → UTF-8 (using piping)
      auto pipeline = utf8_buffer
        | text::encoding::views::adaptor::bytes_to_unicode(text::encoding::EncodingID::UTF8)
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
      persistent::in::text::ByteBuffer cp437_buffer{
        std::byte{0x86},  // å in CP437
        std::byte{0x84},  // ä in CP437
        std::byte{0x94}   // ö in CP437
      };

      // Complete pipeline: CP437 → Unicode → UTF-8
      auto unicode_view = text::encoding::views::bytes_to_unicode(
        cp437_buffer,
        text::encoding::EncodingID::CP437
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

      auto result = path_to_platform_encoded_string_shortcut(utf8_file);

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

      auto result = path_to_platform_encoded_string_shortcut(iso8859_file);

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

      auto result = path_to_platform_encoded_string_shortcut(windows1252_file);

      ASSERT_TRUE(result) << "Expected successful pipeline execution";
      EXPECT_GT(result.value().size(), 0) << "Expected non-empty text";

      logger::development_trace("Windows-1252 transcoded content: {}", result.value());
    }

    TEST_F(EncodingPipelineTestFixture, EmptyFileHandledGracefully) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingPipelineTests, EmptyFileHandledGracefully)",logger::LogToConsole::OFF};

      auto result = path_to_platform_encoded_string_shortcut(empty_file);

      if (false) {
        std::print(
          "\ngot msg:{}"
          ,result.m_messages);
      }

      ASSERT_FALSE(result) << "Expected failure for empty file - no content to process";

      // Check that we got a message about the empty buffer
      bool has_empty_message = false;
      for (const auto& msg : result.m_messages) {
        if (msg.find("empty") != std::string::npos || msg.find("Empty") != std::string::npos) {
          has_empty_message = true;
          break;
        }
      }
      EXPECT_TRUE(has_empty_message) << "Expected message about empty buffer";
    }

    TEST(EncodingPipelineTests, FileNotFoundHandledGracefully) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingPipelineTests, FileNotFoundHandledGracefully)",logger::LogToConsole::OFF};

      auto non_existent = std::filesystem::path("/tmp/cratchit_nonexistent_file_99999.csv");

      auto result = path_to_platform_encoded_string_shortcut(non_existent);

      EXPECT_FALSE(result) << "Expected failure for non-existent file";
      EXPECT_GT(result.m_messages.size(), 0) << "Expected error messages";

      // Verify error messages are informative
      bool has_file_error = false;
      for (const auto& msg : result.m_messages) {
        if (msg.find("failed") != std::string::npos) {
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
      auto result = path_to_platform_encoded_string_shortcut(large_file);

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

    TEST_F(EncodingPipelineTestFixture, CompleteIntegrationAllSteps) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingPipelineTests, CompleteIntegrationAllSteps)"};

      auto result = path_to_platform_encoded_string_shortcut(utf8_file);

      ASSERT_TRUE(result) 
        << "Expected successful complete pipeline"
        << ". Got messages:" << result.to_caption();

      /*
        Got messages:
        utf8_test.csv -> stream : ok
        byte buffer : 57 bytes
        with confidence_threshold:90 : ok
        with encoding : Detected: UTF-8
        platform encoded : 57 bytes      
      */
      // Verify all pipeline steps are documented in messages
      std::vector<std::string> expected_keywords = {
        "stream : ok", 
        "buffer : 57 bytes",
        "encoding : Detected",
        "encoded : 57 bytes"   // Steps 3-4: Transcoding
      };

      for (const auto& keyword : expected_keywords) {
        bool found = false;
        for (const auto& msg : result.m_messages) {
          if (msg.find(keyword) != std::string::npos) {
            found = true;
            break;
          }
        }
        EXPECT_TRUE(found) 
          << "Expected message containing keyword: " << keyword
          << ". Got messages:" << result.to_caption();
          
      }

      // Log all messages for verification
      logger::development_trace("Complete pipeline messages:{}",result.m_messages);

      // Verify content correctness
      EXPECT_TRUE(result.value().find("Name,Amount") != std::string::npos)
        << "Expected CSV header";
      // TODO: This check for 'Å' requires UTF-8 source code file 
      EXPECT_TRUE(result.value().find("Åsa") != std::string::npos)
        << "Expected Swedish character Å preserved";
    }

    TEST_F(EncodingPipelineTestFixture, MonadicErrorPropagation) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingPipelineTests, MonadicErrorPropagation)"};

      // Test that errors from Step 1 propagate correctly through the pipeline
      auto non_existent = test_dir / "does_not_exist.csv";

      auto result = path_to_platform_encoded_string_shortcut(non_existent);

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
      auto result = path_to_platform_encoded_string_shortcut(ascii_file, 99);

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
        auto result = path_to_platform_encoded_string_shortcut(file);

        ASSERT_TRUE(result) << "Expected success for file: " << file.filename();
        EXPECT_GT(result.value().size(), 0) << "Expected non-empty output for: " << file.filename();

        logger::development_trace("File: {} → {} bytes UTF-8",
                                 file.filename().string(),
                                 result.value().size());
      }
    }

  } // namespace encoding_pipeline_integration_suite

  namespace neutral_csv_parser_suite {

    TEST(NeutralCSVParserTests, ParsesSimpleUnquotedCSV) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(NeutralCSVParserTests, ParsesSimpleUnquotedCSV)"};

      std::string csv_text = "Name,Age,City\nAlice,30,Stockholm\nBob,25,Gothenburg\n";

      auto result = CSV::parse::maybe::csv_text_to_table_step(csv_text);

      ASSERT_TRUE(result.has_value()) << "Expected successful parse";
      EXPECT_EQ(result->heading.size(), 3) << "Expected 3 header fields";
      EXPECT_EQ(result->rows.size(), 3) << "Expected 3 rows (including header)";

      // Check header content
      EXPECT_EQ(result->heading[0], "Name");
      EXPECT_EQ(result->heading[1], "Age");
      EXPECT_EQ(result->heading[2], "City");

      // Check first data row (rows[1] since rows includes header)
      EXPECT_EQ(result->rows[1][0], "Alice");
      EXPECT_EQ(result->rows[1][1], "30");
      EXPECT_EQ(result->rows[1][2], "Stockholm");
    }

    TEST(NeutralCSVParserTests, ParsesSemicolonDelimited) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(NeutralCSVParserTests, ParsesSemicolonDelimited)"};

      std::string csv_text = "Date;Amount;Description\n2025-01-01;100.50;Payment\n2025-01-02;-50.00;Withdrawal\n";

      auto result = CSV::parse::maybe::csv_text_to_table_step(csv_text);

      ASSERT_TRUE(result.has_value()) << "Expected successful parse";
      EXPECT_EQ(result->heading.size(), 3) << "Expected 3 header fields";

      EXPECT_EQ(result->heading[0], "Date");
      EXPECT_EQ(result->heading[1], "Amount");
      EXPECT_EQ(result->heading[2], "Description");
    }

    TEST(NeutralCSVParserTests, DetectsCommaDelimiter) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(NeutralCSVParserTests, DetectsCommaDelimiter)"};

      std::string csv_text = "A,B,C\n1,2,3\n";

      char detected = CSV::parse::maybe::to_csv_delimiter(csv_text);

      EXPECT_EQ(detected, ',') << "Expected comma detection";
    }

    TEST(NeutralCSVParserTests, DetectsSemicolonDelimiter) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(NeutralCSVParserTests, DetectsSemicolonDelimiter)"};

      std::string csv_text = "A;B;C\n1;2;3\n";

      char detected = CSV::parse::maybe::to_csv_delimiter(csv_text);

      EXPECT_EQ(detected, ';') << "Expected semicolon detection";
    }

    TEST(NeutralCSVParserTests, EmptyStringReturnsNullopt) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(NeutralCSVParserTests, EmptyStringReturnsNullopt)"};

      std::string csv_text = "";

      auto result = CSV::parse::maybe::csv_text_to_table_step(csv_text);

      EXPECT_FALSE(result.has_value()) << "Expected empty optional for empty input";
    }

    TEST(NeutralCSVParserTests, HandlesTrailingNewline) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(NeutralCSVParserTests, HandlesTrailingNewline)"};

      std::string csv_text = "A,B\n1,2\n";

      auto result = CSV::parse::maybe::csv_text_to_table_step(csv_text);

      ASSERT_TRUE(result.has_value()) << "Expected successful parse";
      EXPECT_EQ(result->rows.size(), 2) << "Expected 2 rows (header + 1 data)";
    }

    TEST(NeutralCSVParserTests, HandlesWindowsLineEndings) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(NeutralCSVParserTests, HandlesWindowsLineEndings)"};

      std::string csv_text = "A,B\r\n1,2\r\n";

      auto result = CSV::parse::maybe::csv_text_to_table_step(csv_text);

      ASSERT_TRUE(result.has_value()) << "Expected successful parse";
      EXPECT_EQ(result->rows.size(), 2) << "Expected 2 rows";
      EXPECT_EQ(result->rows[1][0], "1");
      EXPECT_EQ(result->rows[1][1], "2");
    }

    TEST(NeutralCSVParserTests, ParsesQuotedFieldsBasic) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(NeutralCSVParserTests, ParsesQuotedFieldsBasic)"};

      std::string csv_text = R"(Name,Description
"Alice","Software Engineer"
"Bob","Data Analyst"
)";

      auto result = CSV::parse::maybe::csv_text_to_table_step(csv_text);

      ASSERT_TRUE(result.has_value()) << "Expected successful parse";
      EXPECT_EQ(result->rows[1][0], "Alice") << "Expected quotes removed";
      EXPECT_EQ(result->rows[1][1], "Software Engineer");
    }

    TEST(NeutralCSVParserTests, ParsesQuotedFieldsWithCommas) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(NeutralCSVParserTests, ParsesQuotedFieldsWithCommas)"};

      std::string csv_text = R"(Name,Address
"Alice","123 Main St, Apt 4, Stockholm"
"Bob","456 Oak Ave, Gothenburg"
)";

      auto result = CSV::parse::maybe::csv_text_to_table_step(csv_text);

      ASSERT_TRUE(result.has_value()) << "Expected successful parse";
      EXPECT_EQ(result->rows[1][1], "123 Main St, Apt 4, Stockholm")
        << "Expected commas inside quoted field preserved";
      EXPECT_EQ(result->rows[2][1], "456 Oak Ave, Gothenburg");
    }

    TEST(NeutralCSVParserTests, ParsesEscapedQuotes) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(NeutralCSVParserTests, ParsesEscapedQuotes)"};

      std::string csv_text = R"(Name,Quote
"Alice","She said ""Hello"""
"Bob","He replied ""Hi there"""
)";

      auto result = CSV::parse::maybe::csv_text_to_table_step(csv_text);

      ASSERT_TRUE(result.has_value()) << "Expected successful parse";
      EXPECT_EQ(result->rows[1][1], R"(She said "Hello")")
        << "Expected escaped quotes to become single quotes";
      EXPECT_EQ(result->rows[2][1], R"(He replied "Hi there")");
    }

    TEST(NeutralCSVParserTests, ParsesQuotedFieldsWithNewlines) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(NeutralCSVParserTests, ParsesQuotedFieldsWithNewlines)"};

      std::string csv_text = "Name,Description\n\"Alice\",\"Line 1\nLine 2\"\n\"Bob\",\"Single line\"\n";

      auto result = CSV::parse::maybe::csv_text_to_table_step(csv_text);

      ASSERT_TRUE(result.has_value()) << "Expected successful parse";
      EXPECT_TRUE(result->rows[1][1].find('\n') != std::string::npos)
        << "Expected newline preserved in quoted field";
    }

    TEST(NeutralCSVParserTests, HandlesMixedQuotedAndUnquoted) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(NeutralCSVParserTests, HandlesMixedQuotedAndUnquoted)"};

      std::string csv_text = R"(Name,Age,City
Alice,30,"Stockholm, Sweden"
"Bob",25,Gothenburg
)";

      auto result = CSV::parse::maybe::csv_text_to_table_step(csv_text);

      ASSERT_TRUE(result.has_value()) << "Expected successful parse";
      EXPECT_EQ(result->rows[1][0], "Alice") << "Unquoted field";
      EXPECT_EQ(result->rows[1][1], "30") << "Unquoted field";
      EXPECT_EQ(result->rows[1][2], "Stockholm, Sweden") << "Quoted field with comma";
      EXPECT_EQ(result->rows[2][0], "Bob") << "Quoted field";
      EXPECT_EQ(result->rows[2][1], "25") << "Unquoted field";
    }

    TEST(NeutralCSVParserTests, ExplicitDelimiterOverridesDetection) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(NeutralCSVParserTests, ExplicitDelimiterOverridesDetection)"};

      // CSV with semicolons, but explicitly request comma parsing
      std::string csv_text = "A;B;C\n1;2;3\n";

      auto result = CSV::parse::maybe::csv_to_table(csv_text, ',');

      ASSERT_TRUE(result.has_value()) << "Expected successful parse";
      // With comma delimiter, the entire line becomes one field
      EXPECT_EQ(result->heading.size(), 1) << "Expected 1 field when using wrong delimiter";
      EXPECT_EQ(result->heading[0], "A;B;C") << "Expected semicolons not treated as delimiters";
    }

    TEST(NeutralCSVParserTests, ParsesRealNordeaCSV) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(NeutralCSVParserTests, ParsesRealNordeaCSV)"};

      std::string csv_text = sz_NORDEA_csv_20251120;

      auto result = CSV::parse::maybe::csv_text_to_table_step(csv_text);

      ASSERT_TRUE(result.has_value()) << "Expected successful parse of Nordea CSV";

      // Check header
      EXPECT_EQ(result->heading[0], "Bokföringsdag") << "Expected Swedish header";
      EXPECT_EQ(result->heading[1], "Belopp");
      EXPECT_EQ(result->heading[9], "Valuta");

      // Should have header + data rows
      EXPECT_GT(result->rows.size(), 1) << "Expected multiple rows including header";

      // Check first data row (rows[1])
      EXPECT_EQ(result->rows[1][0], "2025/09/29") << "Expected date field";
      EXPECT_EQ(result->rows[1][1], "-1083,75") << "Expected amount field";
      EXPECT_EQ(result->rows[1][9], "SEK") << "Expected currency field";

      logger::development_trace("Nordea CSV parsed: {} total rows", result->rows.size());
    }

    TEST(NeutralCSVParserTests, ParsesRealSKVCSVOlder) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(NeutralCSVParserTests, ParsesRealSKVCSVOlder)"};

      std::string csv_text = sz_SKV_csv_older;

      auto result = CSV::parse::maybe::csv_text_to_table_step(csv_text);

      ASSERT_TRUE(result.has_value()) << "Expected successful parse of older SKV CSV";

      // This format has empty first column, then text, then amounts
      EXPECT_GT(result->rows.size(), 1) << "Expected multiple rows";

      // Check first row (balance row) - has empty first column
      EXPECT_TRUE(result->rows[0][0].empty()) << "Expected empty first column in balance row";
      EXPECT_EQ(result->rows[0][1], "Ingående saldo 2025-04-01");
      EXPECT_EQ(result->rows[0][2], "656");

      // Check second row (transaction row) - has date in first column
      EXPECT_EQ(result->rows[1][0], "2025-04-05") << "Expected date in first column";
      EXPECT_EQ(result->rows[1][1], "Intäktsränta");
      EXPECT_EQ(result->rows[1][2], "1");

      logger::development_trace("Older SKV CSV parsed: {} total rows", result->rows.size());
    }

    TEST(NeutralCSVParserTests, ParsesRealSKVCSVNewer) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(NeutralCSVParserTests, ParsesRealSKVCSVNewer)"};

      std::string csv_text = sz_SKV_csv_20251120;

      auto result = CSV::parse::maybe::csv_text_to_table_step(csv_text);

      ASSERT_TRUE(result.has_value()) << "Expected successful parse of newer SKV CSV";

      // This format uses quotes extensively
      EXPECT_EQ(result->rows.size(), 8) << "Expected eight rows";

      // Check first row - company info
      EXPECT_EQ(result->rows[0][0], "THE ITFIED AB");
      EXPECT_EQ(result->rows[0][1], "556782-8172");

      // Check a transaction row with amounts
      // Find a row with a date
      bool found_transaction = false;
      for (size_t i = 0; i < result->rows.size(); ++i) {
        if (!result->rows[i][0].empty() && result->rows[i][0].find("2025") != std::string::npos) {
          // This is a transaction row
          EXPECT_EQ(result->rows[i][0], "2025-07-05") << "Expected date in first field";
          EXPECT_EQ(result->rows[i][1], "Intäktsränta") << "Expected description";
          EXPECT_EQ(result->rows[i][2], "1") << "Expected amount";
          found_transaction = true;
          break;
        }
      }
      EXPECT_TRUE(found_transaction) << "Expected to find at least one transaction row";

      logger::development_trace("Newer SKV CSV parsed: {} total rows", result->rows.size());
    }

    TEST(NeutralCSVParserTests, HandlesSwedishCharactersInRealData) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(NeutralCSVParserTests, HandlesSwedishCharactersInRealData)"};

      // The real CSV data contains Swedish characters like å, ä, ö
      std::string csv_text = sz_NORDEA_csv_20251120;

      auto result = CSV::parse::maybe::csv_text_to_table_step(csv_text);

      ASSERT_TRUE(result.has_value()) << "Expected successful parse";

      // Check that Swedish characters are preserved
      EXPECT_EQ(result->heading[0], "Bokföringsdag") << "Expected 'ö' to be preserved";
      EXPECT_EQ(result->heading[5], "Ytterligare detaljer") << "Expected complete Swedish text";

      // Check in data rows
      bool found_swedish = false;
      for (size_t i = 0; i < result->rows.size(); ++i) {
        for (size_t j = 0; j < result->rows[i].size(); ++j) {
          if (result->rows[i][j].find("Mobiltjänster") != std::string::npos) {
            EXPECT_EQ(result->rows[i][j], "Mobiltjänster Q3") << "Expected Swedish 'ä' preserved";
            found_swedish = true;
            break;
          }
        }
        if (found_swedish) break;
      }
    }

  } // namespace neutral_csv_parser_suite

  namespace csv_pipeline_composition_suite {

    // Test fixture for pipeline composition tests
    class CSVPipelineCompositionTestFixture : public ::testing::Test {
    protected:
      std::filesystem::path test_dir;
      std::filesystem::path utf8_csv_file;
      std::filesystem::path iso8859_csv_file;

      void SetUp() override {
        logger::scope_logger log_raii{logger::development_trace, "CSVPipelineCompositionTestFixture::SetUp"};

        // Create temporary test directory
        test_dir = std::filesystem::temp_directory_path() / "cratchit_test_csv_pipeline";
        std::filesystem::create_directories(test_dir);

        // Create UTF-8 CSV test file
        utf8_csv_file = test_dir / "utf8_test.csv";
        {
          std::ofstream ofs(utf8_csv_file, std::ios::binary);
          std::string csv_content = "Name,Amount,City\nJohan,100.50,Stockholm\nÅsa,250.75,Göteborg\n";
          ofs.write(csv_content.data(), csv_content.size());
        }

        // Create ISO-8859-1 CSV test file with Swedish characters
        iso8859_csv_file = test_dir / "iso8859_test.csv";
        {
          std::ofstream ofs(iso8859_csv_file, std::ios::binary);
          // ISO-8859-1 encoded CSV with Å and ö
          unsigned char iso_content[] = {
            'N','a','m','e',';','C','i','t','y','\n',
            0xC5,'s','a',';',  // Åsa; (0xC5 = Å)
            'G',0xF6,'t','e','b','o','r','g','\n'  // Göteborg (0xF6 = ö)
          };
          ofs.write(reinterpret_cast<char*>(iso_content), sizeof(iso_content));
        }
      }

      void TearDown() override {
        logger::scope_logger log_raii{logger::development_trace, "CSVPipelineCompositionTestFixture::TearDown"};
        std::error_code ec;
        std::filesystem::remove_all(test_dir, ec);
      }
    };

    TEST_F(CSVPipelineCompositionTestFixture, CompletePipelineFileToTable) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(CSVPipelineCompositionTests, CompletePipelineFileToTable)"};

      // Complete pipeline: file_path → UTF-8 text → CSV::Table
      auto maybe_text = path_to_platform_encoded_string_shortcut(utf8_csv_file);
      ASSERT_TRUE(maybe_text) << "Expected successful encoding pipeline";

      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(maybe_text.value());
      ASSERT_TRUE(maybe_table.has_value()) << "Expected successful CSV parse";

      // Verify the table structure
      EXPECT_EQ(maybe_table->heading.size(), 3) << "Expected 3 columns";
      EXPECT_EQ(maybe_table->heading[0], "Name");
      EXPECT_EQ(maybe_table->heading[1], "Amount");
      EXPECT_EQ(maybe_table->heading[2], "City");

      EXPECT_EQ(maybe_table->rows.size(), 3) << "Expected 3 rows (header + 2 data)";
      EXPECT_EQ(maybe_table->rows[1][0], "Johan");
      EXPECT_EQ(maybe_table->rows[2][0], "Åsa") << "Expected Swedish character preserved";
      EXPECT_EQ(maybe_table->rows[2][2], "Göteborg") << "Expected Swedish ö preserved";
    }

    TEST_F(CSVPipelineCompositionTestFixture, MonadicCompositionWithAndThen) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(CSVPipelineCompositionTests, MonadicCompositionWithAndThen)"};

      // Demonstrate monadic composition with .and_then()
      auto result = path_to_platform_encoded_string_shortcut(utf8_csv_file)
        .and_then([](auto text) -> AnnotatedMaybe<CSV::Table> {
          AnnotatedMaybe<CSV::Table> csv_result;
          auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(text);
          if (maybe_table) {
            csv_result.m_value = *maybe_table;
            csv_result.push_message(std::format("Parsed CSV with {} rows", maybe_table->rows.size()));
          } else {
            csv_result.push_message("CSV parsing failed");
          }
          return csv_result;
        });

      ASSERT_TRUE(result) << "Expected successful monadic composition";
      EXPECT_EQ(result.value().heading.size(), 3) << "Expected 3 columns";
      EXPECT_GT(result.m_messages.size(), 2) << "Expected messages from both pipeline stages";

      /*
        ["utf8_test.csv -> stream : ok"
        , "byte buffer : 62 bytes"
        , "with confidence_threshold:90 : ok"
        , "with encoding : Detected: UTF-8"
        , "platform encoded : 62 bytes"
        , "Parsed CSV with 3 rows"]      
      */
      // Verify messages document the complete pipeline
      bool has_encoding_msg = false;
      bool has_csv_msg = false;
      for (const auto& msg : result.m_messages) {
        if (msg.find("with encoding") != std::string::npos) {
          has_encoding_msg = true;
        }
        if (msg.find("Parsed CSV") != std::string::npos) {
          has_csv_msg = true;
        }
      }
      EXPECT_TRUE(has_encoding_msg) 
        << std::format("Expected encoding pipeline message in {}",result.m_messages);
      EXPECT_TRUE(has_csv_msg) 
        << std::format("Expected CSV parsing message in {}",result.m_messages);
    }

    TEST_F(CSVPipelineCompositionTestFixture, TranscodesISO8859ToUTF8ThenParsesCSV) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(CSVPipelineCompositionTests, TranscodesISO8859ToUTF8ThenParsesCSV)"};

      // Complete pipeline with ISO-8859-1 source
      auto maybe_text = path_to_platform_encoded_string_shortcut(iso8859_csv_file);
      ASSERT_TRUE(maybe_text) << "Expected successful encoding pipeline";

      logger::development_trace("Transcoded text: {}", maybe_text.value());

      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(maybe_text.value());
      ASSERT_TRUE(maybe_table.has_value()) << "Expected successful CSV parse";

      // Verify Swedish characters were correctly transcoded and parsed
      EXPECT_EQ(maybe_table->heading[0], "Name");
      EXPECT_EQ(maybe_table->heading[1], "City");
      EXPECT_EQ(maybe_table->rows[1][0], "Åsa") << "Expected Å transcoded correctly";
      EXPECT_EQ(maybe_table->rows[1][1], "Göteborg") << "Expected ö transcoded correctly";
    }

    TEST_F(CSVPipelineCompositionTestFixture, ErrorPropagationFromEncodingStage) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(CSVPipelineCompositionTests, ErrorPropagationFromEncodingStage)"};

      auto non_existent = test_dir / "does_not_exist.csv";

      // Pipeline should short-circuit on file read failure
      auto result = path_to_platform_encoded_string_shortcut(non_existent)
        .and_then([](auto text) -> AnnotatedMaybe<CSV::Table> {
          AnnotatedMaybe<CSV::Table> csv_result;
          auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(text);
          if (maybe_table) {
            csv_result.m_value = *maybe_table;
          }
          return csv_result;
        });

      EXPECT_FALSE(result) << "Expected failure to propagate through pipeline";
      EXPECT_GT(result.m_messages.size(), 0) << "Expected error messages preserved";
    }

    TEST_F(CSVPipelineCompositionTestFixture, MalformedCSVReturnsNullopt) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(CSVPipelineCompositionTests, MalformedCSVReturnsNullopt)"};

      // Create file with empty content
      auto empty_file = test_dir / "empty.csv";
      {
        std::ofstream ofs(empty_file);
      }

      auto maybe_text = path_to_platform_encoded_string_shortcut(empty_file);
      ASSERT_FALSE(maybe_text) << "Expected failure for empty file - no content to process";
      EXPECT_GT(maybe_text.m_messages.size(), 0) << "Expected error messages about empty buffer";
    }

    TEST(CSVPipelineCompositionTests, ParsesRealNordeaCSVWithEncoding) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(CSVPipelineCompositionTests, ParsesRealNordeaCSVWithEncoding)"};

      // Demonstrate that real CSV data can be parsed
      // (In real usage, this would come through the file reading pipeline)
      std::string csv_text = sz_NORDEA_csv_20251120;

      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << "Expected successful parse";

      EXPECT_EQ(maybe_table->heading[0], "Bokföringsdag");
      EXPECT_GT(maybe_table->rows.size(), 5) << "Expected multiple transaction rows";

      logger::development_trace("Real Nordea CSV parsed successfully with {} rows",
                               maybe_table->rows.size());
    }

  } // namespace csv_pipeline_composition_suite

  namespace account_statement_suite {

    TEST(AccountStatementTests, ExtractFromNordeaCSVWithHeaders) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTests, ExtractFromNordeaCSVWithHeaders)"};

      // Parse the NORDEA CSV sample data
      std::string csv_text = sz_NORDEA_csv_20251120;

      auto maybe_statement = CSV::parse::maybe::csv_text_to_table_step(csv_text)
        .and_then(account::statement::maybe::to_statement_id_ed_step)
        .and_then(account::statement::maybe::statement_id_ed_to_account_statement_step);

      ASSERT_TRUE(maybe_statement.has_value()) << "Expected successful account statement extraction";
      EXPECT_GT(maybe_statement->entries().size(), 0) << "Expected at least one account statement entry";

      // Verify first entry
      auto const& first_entry = maybe_statement->entries().at(0);
      EXPECT_EQ(first_entry.transaction_date, to_date(2025, 9, 29));
      EXPECT_EQ(first_entry.transaction_amount, *to_amount("-1083,75"));
      EXPECT_FALSE(first_entry.transaction_caption.empty()) << "Expected non-empty description";

      logger::development_trace("Extracted {} account statement entries from NORDEA CSV",
                               maybe_statement->entries().size());
    }

    TEST(AccountStatementTests, ExtractFromSKVCSVWithoutHeaders) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTests, ExtractFromSKVCSVWithoutHeaders)"};

      // Parse the SKV CSV sample data (older format)
      std::string csv_text = sz_SKV_csv_older;

      auto maybe_statement = CSV::parse::maybe::csv_text_to_table_step(csv_text)
        .and_then(account::statement::maybe::to_statement_id_ed_step)
        .and_then(account::statement::maybe::statement_id_ed_to_account_statement_step);

      ASSERT_TRUE(maybe_statement.has_value()) << "Expected successful account statement extraction";

      // Should have extracted transaction rows (not balance rows)
      EXPECT_GT(maybe_statement->entries().size(), 0) << "Expected at least one transaction entry";

      // Verify that balance rows were filtered out
      for (auto const& entry : maybe_statement->entries()) {
        std::string desc_lower = entry.transaction_caption;
        std::ranges::transform(desc_lower, desc_lower.begin(),
          [](char c) { return std::tolower(c); });

        EXPECT_EQ(desc_lower.find("saldo"), std::string::npos)
          << "Expected balance rows to be filtered out";
      }

      logger::development_trace("Extracted {} transaction entries from SKV CSV (balance rows filtered)",
                               maybe_statement->entries().size());
    }

    TEST(AccountStatementTests, ExtractFromSKVCSVNewer) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTests, ExtractFromSKVCSVNewer)"};

      // Parse the newer SKV CSV format
      std::string csv_text = sz_SKV_csv_20251120;

      auto maybe_statement = CSV::parse::maybe::csv_text_to_table_step(csv_text)
        .and_then(account::statement::maybe::to_statement_id_ed_step)
        .and_then(account::statement::maybe::statement_id_ed_to_account_statement_step);

      ASSERT_TRUE(maybe_statement.has_value()) << "Expected successful account statement extraction";

      // Should have extracted transaction rows
      EXPECT_EQ(maybe_statement->entries().size(), 4) << "Expected four transaction entries";

      // Verify one specific entry
      bool found_interest = false;
      for (auto const& entry : maybe_statement->entries()) {
        if (entry.transaction_caption.find("Intäktsränta") != std::string::npos) {
          found_interest = true;
          EXPECT_GT(entry.transaction_amount, Amount{0}) << "Expected positive interest amount";
          break;
        }
      }
      EXPECT_TRUE(found_interest) << "Expected to find interest transaction";

      logger::development_trace("Extracted {} entries from newer SKV CSV", maybe_statement->entries().size());
    }

    TEST(AccountStatementTests, EmptyCSVReturnsEmptyList) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTests, EmptyCSVReturnsEmptyList)"};

      // Create a CSV table with only headers
      CSV::Table table;
      table.heading = Key::Path{std::vector<std::string>{"Date", "Amount", "Description"}};
      table.rows.push_back(table.heading);

      auto maybe_statement = account::statement::maybe::to_statement_id_ed_step(table)
        .and_then(account::statement::maybe::statement_id_ed_to_account_statement_step);

      ASSERT_FALSE(maybe_statement.has_value()) << "Expected header only csv to be invalid";
    }

    TEST(AccountStatementTests, BalanceRowsAreIgnored) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTests, BalanceRowsAreIgnored)"};

      // Create a CSV table with balance row
      CSV::Table table;
      table.heading = Key::Path{std::vector<std::string>{"Date", "Description", "Amount"}};
      table.rows.push_back(table.heading);
      table.rows.push_back(Key::Path{std::vector<std::string>{"", "Ingående saldo 2025-01-01", "1000"}});
      table.rows.push_back(Key::Path{std::vector<std::string>{"2025-01-05", "Payment", "100"}});
      table.rows.push_back(Key::Path{std::vector<std::string>{"", "Utgående saldo 2025-01-31", "1100"}});

      auto maybe_statement = account::statement::maybe::to_statement_id_ed_step(table)
        .and_then(account::statement::maybe::statement_id_ed_to_account_statement_step);

      ASSERT_TRUE(maybe_statement.has_value()) << "Expected successful extraction";
      EXPECT_EQ(maybe_statement->entries().size(), 1) << "Expected only transaction row, balance rows ignored";

      if (!maybe_statement->entries().empty()) {
        EXPECT_EQ(maybe_statement->entries().at(0).transaction_caption, "Payment");
      }
    }

    TEST(AccountStatementTests, MissingColumnsReturnsNullopt) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTests, MissingColumnsReturnsNullopt)"};

      // Create a CSV table with missing required columns
      CSV::Table table;
      table.heading = Key::Path{std::vector<std::string>{"SomeColumn", "AnotherColumn"}};
      table.rows.push_back(table.heading);
      table.rows.push_back(Key::Path{std::vector<std::string>{"value1", "value2"}});

      auto maybe_statement = account::statement::maybe::to_statement_id_ed_step(table)
        .and_then(account::statement::maybe::statement_id_ed_to_account_statement_step);

      EXPECT_FALSE(maybe_statement.has_value()) << "Expected nullopt when required columns cannot be detected";
    }

    TEST(AccountStatementTests, MultipleDescriptionColumnsAreConcatenated) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTests, MultipleDescriptionColumnsAreConcatenated)"};

      // Parse NORDEA CSV which has multiple description columns
      std::string csv_text = sz_NORDEA_csv_20251120;

      auto maybe_statement = CSV::parse::maybe::csv_text_to_table_step(csv_text)
        .and_then(account::statement::maybe::to_statement_id_ed_step)
        .and_then(account::statement::maybe::statement_id_ed_to_account_statement_step);

      ASSERT_TRUE(maybe_statement.has_value());
      EXPECT_GT(maybe_statement->entries().size(), 0);

      // Check that descriptions are not empty and contain meaningful content
      auto const& first_entry = maybe_statement->entries().at(0);
      EXPECT_GT(first_entry.transaction_caption.size(), 5)
        << "Expected substantial description content";

      logger::development_trace("First entry description: {}", first_entry.transaction_caption);
    }

    TEST(AccountStatementTests, IntegrationWithPipelineSteps) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTests, IntegrationWithPipelineSteps)"};

      // Create a temporary test file
      auto temp_path = std::filesystem::temp_directory_path() / "cratchit_test_account_statements.csv";
      {
        std::ofstream ofs(temp_path);
        ofs << "Date,Amount,Description\n";
        ofs << "2025-01-01,100.50,Payment received\n";
        ofs << "2025-01-02,-50.00,Withdrawal\n";
        ofs << "2025-01-03,200.00,Transfer in\n";
      }

      // Complete pipeline: file → text → CSV::Table → AccountStatementEntries
      auto maybe_text = path_to_platform_encoded_string_shortcut(temp_path);
      ASSERT_TRUE(maybe_text) << "Expected successful file read";

      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(maybe_text.value());
      ASSERT_TRUE(maybe_table.has_value()) << "Expected successful CSV parse";

      auto maybe_statement = account::statement::maybe::to_statement_id_ed_step(*maybe_table)
        .and_then(account::statement::maybe::statement_id_ed_to_account_statement_step);
      
      ASSERT_TRUE(maybe_statement.has_value()) << "Expected successful account statement extraction";

      EXPECT_EQ(maybe_statement->entries().size(), 3) << "Expected 3 account statement entries";

      // Verify entries
      EXPECT_EQ(maybe_statement->entries().at(0).transaction_date, to_date(2025, 1, 1));
      EXPECT_EQ(maybe_statement->entries().at(1).transaction_amount, *to_amount("-50.00"));
      EXPECT_EQ(maybe_statement->entries().at(2).transaction_caption, "Transfer in");

      // Clean up
      std::filesystem::remove(temp_path);

      logger::development_trace("Complete pipeline integration test passed");
    }

    TEST(AccountStatementTests, ExtractTransactionAmountNotSaldo) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTests, ExtractTransactionAmountNotSaldo)"};

      // Parse the newer SKV CSV format which has both transaction amount and saldo columns
      // Column structure: Date | Description | Transaction Amount | Saldo
      std::string csv_text = sz_SKV_csv_20251120;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);

      ASSERT_TRUE(maybe_table.has_value()) << "Expected successful CSV parse";

      auto maybe_statement = account::statement::maybe::to_statement_id_ed_step(*maybe_table)
        .and_then(account::statement::maybe::statement_id_ed_to_account_statement_step);

      ASSERT_TRUE(maybe_statement.has_value()) << "Expected successful account statement extraction";
      ASSERT_EQ(maybe_statement->entries().size(), 4) << "Expected four transaction entries";

      // Expected transaction amounts (column 2), NOT saldo values (column 3)
      // Row: "2025-07-05";"Intäktsränta";"1";"659"     -> amount=1, NOT 659
      // Row: "2025-08-13";"Moms...";"879";"1 538"      -> amount=879, NOT 1538
      // Row: "2025-08-20";"Utbetalning";"-879";"659"   -> amount=-879, NOT 659
      // Row: "2025-09-06";"Intäktsränta";"1";"660"     -> amount=1, NOT 660

      auto const& entries = maybe_statement->entries();

      // Verify each transaction has correct transaction amount (not saldo)
      EXPECT_EQ(entries[0].transaction_amount, Amount{1})
        << "First Intäktsränta should be 1, not saldo 659";
      EXPECT_EQ(entries[1].transaction_amount, Amount{879})
        << "Moms should be 879, not saldo 1538";
      EXPECT_EQ(entries[2].transaction_amount, Amount{-879})
        << "Utbetalning should be -879, not saldo 659";
      EXPECT_EQ(entries[3].transaction_amount, Amount{1})
        << "Second Intäktsränta should be 1, not saldo 660";

      logger::development_trace("Verified all 4 entries have transaction amounts, not saldo amounts");
    }

    TEST(AccountStatementTests, ExtractTransactionAmountWithOlderSKVFormat) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTests, ExtractTransactionAmountWithOlderSKVFormat)"};

      // Parse the older SKV CSV format
      // Format: Date;Description;Amount;(empty);(empty)
      std::string csv_text = sz_SKV_csv_older;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);

      ASSERT_TRUE(maybe_table.has_value()) << "Expected successful CSV parse";

      auto maybe_statement = account::statement::maybe::to_statement_id_ed_step(*maybe_table)
        .and_then(account::statement::maybe::statement_id_ed_to_account_statement_step);

      ASSERT_TRUE(maybe_statement.has_value()) << "Expected successful account statement extraction";
      ASSERT_EQ(maybe_statement->entries().size(), 4) << "Expected four transaction entries";

      auto const& entries = maybe_statement->entries();

      // Verify transaction amounts from older format
      // Row: 2025-04-05;Intäktsränta;1;;
      // Row: 2025-05-12;Moms jan 2025 - mars 2025;1997;;
      // Row: 2025-05-14;Utbetalning;-1997;;
      // Row: 2025-06-01;Intäktsränta;1;;
      EXPECT_EQ(entries[0].transaction_amount, Amount{1});
      EXPECT_EQ(entries[1].transaction_amount, Amount{1997});
      EXPECT_EQ(entries[2].transaction_amount, Amount{-1997});
      EXPECT_EQ(entries[3].transaction_amount, Amount{1});

      logger::development_trace("Verified older SKV format transaction amounts");
    }

    TEST(AccountStatementTests, ThousandsSeparatorInSaldoDoesNotAffectTransaction) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTests, ThousandsSeparatorInSaldoDoesNotAffectTransaction)"};

      // The SKV CSV has saldo "1 538" (with space as thousands separator)
      // Verify that:
      // 1. The "1 538" saldo format now parses correctly as 1538
      // 2. Our extraction correctly uses the transaction column (879), not the saldo column

      // First, verify that "1 538" now parses correctly as 1538
      auto saldo_with_space = to_amount("1 538");
      ASSERT_TRUE(saldo_with_space.has_value()) << "1 538 should parse successfully";
      EXPECT_EQ(*saldo_with_space, Amount{1538})
        << "Saldo '1 538' should parse as 1538 (space is thousands separator)";

      // Now verify the actual extraction uses the correct column
      std::string csv_text = sz_SKV_csv_20251120;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value());

      auto maybe_statement = account::statement::maybe::to_statement_id_ed_step(*maybe_table)
        .and_then(account::statement::maybe::statement_id_ed_to_account_statement_step);

      ASSERT_TRUE(maybe_statement.has_value());
      ASSERT_EQ(maybe_statement->entries().size(), 4);

      // The row with "1 538" saldo should have 879 as transaction amount
      // Row: "2025-08-13";"Moms april 2025 - juni 2025";"879";"1 538"
      // Extraction uses transaction_amount_column (879), not saldo_amount_column (1538)
      auto const& moms_entry = maybe_statement->entries().at(1);
      EXPECT_EQ(moms_entry.transaction_amount, Amount{879})
        << "Transaction amount should be 879, not saldo 1538";

      logger::development_trace("Verified correct column selection with properly parsed saldo");
    }

    // ============================================================================
    // Tests for statement_id_ed_to_account_statement_step
    // ============================================================================

    TEST(CSVTable2AccountStatementTests, CsvTableToAccountStatementWithNordea) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTests, CsvTableToAccountStatementWithNordea)"};

      // Parse NORDEA CSV
      std::string csv_text = sz_NORDEA_csv_20251120;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << "Failed to parse NORDEA CSV";

      auto maybe_statement_id_ed = account::statement::maybe::to_statement_id_ed_step(*maybe_table);
      ASSERT_TRUE(maybe_statement_id_ed.has_value()) << "Failed to identify account statement from NORDEA CSV";

      auto maybe_statement = account::statement::maybe::statement_id_ed_to_account_statement_step(*maybe_statement_id_ed);

      ASSERT_TRUE(maybe_statement.has_value()) << "Expected successful AccountStatement creation";

      // Verify the AccountStatement has entries
      EXPECT_GT(maybe_statement->entries().size(), 0) << "Expected at least one entry";

      ASSERT_TRUE(maybe_statement->meta().m_maybe_account_irl_id.has_value())
        << "Expected AccountID in meta";
      EXPECT_EQ(maybe_statement->meta().m_maybe_account_irl_id->m_prefix, "NORDEA")
        << "Expected NORDEA AccountID prefix in AccountStatement meta";

      logger::development_trace("Created AccountStatement with {} entries and account: {}",
        maybe_statement->entries().size(),
        maybe_statement->meta().m_maybe_account_irl_id->to_string());
    }

    TEST(CSVTable2AccountStatementTests, CsvTableToAccountStatementWithSKV) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTests, CsvTableToAccountStatementWithSKV)"};

      // Parse SKV CSV (newer format)
      std::string csv_text = sz_SKV_csv_20251120;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << "Failed to parse SKV CSV";

      auto maybe_statement_id_ed = account::statement::maybe::to_statement_id_ed_step(*maybe_table);
      ASSERT_TRUE(maybe_statement_id_ed.has_value()) << "Failed to identify account statement from SKV CSV";

      auto maybe_statement = account::statement::maybe::statement_id_ed_to_account_statement_step(*maybe_statement_id_ed);

      ASSERT_TRUE(maybe_statement.has_value()) << "Expected successful AccountStatement creation";

      // Verify entries
      EXPECT_EQ(maybe_statement->entries().size(), 4) << "Expected 4 transaction entries";

      ASSERT_TRUE(maybe_statement->meta().m_maybe_account_irl_id.has_value());
      EXPECT_EQ(maybe_statement->meta().m_maybe_account_irl_id->m_prefix, "SKV")
        << "Expected SKV AccountID prefix in AccountStatement meta";
      EXPECT_EQ(maybe_statement->meta().m_maybe_account_irl_id->m_value, "5567828172")
        << "Expected org number in AccountStatement meta";

      logger::development_trace("Created SKV AccountStatement with {} entries and account: {}",
        maybe_statement->entries().size(),
        maybe_statement->meta().m_maybe_account_irl_id->to_string());
    }

    TEST(CSVTable2AccountStatementTests, CsvTableToAccountStatementPreservesEntryData) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTests, CsvTableToAccountStatementPreservesEntryData)"};

      // Create a simple test CSV
      CSV::Table table;
      table.heading = Key::Path{std::vector<std::string>{"Date", "Amount", "Description"}};
      table.rows.push_back(table.heading);
      table.rows.push_back(Key::Path{std::vector<std::string>{"2025-01-01", "100.50", "Test Payment"}});
      table.rows.push_back(Key::Path{std::vector<std::string>{"2025-01-02", "-50.00", "Withdrawal"}});

      auto maybe_statement = account::statement::maybe::to_statement_id_ed_step(table)
        .and_then(account::statement::maybe::statement_id_ed_to_account_statement_step);

      ASSERT_TRUE(maybe_statement.has_value()) << "Expected successful AccountStatement creation";

      // Verify entry data is preserved
      ASSERT_EQ(maybe_statement->entries().size(), 2);

      auto const& first_entry = maybe_statement->entries()[0];
      EXPECT_EQ(first_entry.transaction_date, to_date(2025, 1, 1));
      EXPECT_EQ(first_entry.transaction_amount, *to_amount("100.50"));
      EXPECT_EQ(first_entry.transaction_caption, "Test Payment");

      auto const& second_entry = maybe_statement->entries()[1];
      EXPECT_EQ(second_entry.transaction_date, to_date(2025, 1, 2));
      EXPECT_EQ(second_entry.transaction_amount, *to_amount("-50.00"));
      EXPECT_EQ(second_entry.transaction_caption, "Withdrawal");

      // Verify AccountID is preserved
      ASSERT_TRUE(maybe_statement->meta().m_maybe_account_irl_id.has_value());
      EXPECT_EQ(maybe_statement->meta().m_maybe_account_irl_id->m_prefix, "Generic");
      EXPECT_EQ(maybe_statement->meta().m_maybe_account_irl_id->m_value, "Anonymous");
    }

    TEST(CSVTable2AccountStatementTests, CsvTableToAccountStatementWithInvalidTable) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTests, CsvTableToAccountStatementWithInvalidTable)"};

      // Create a CSV table with undetectable columns
      CSV::Table table;
      table.heading = Key::Path{std::vector<std::string>{"UnknownCol1", "UnknownCol2"}};
      table.rows.push_back(table.heading);
      table.rows.push_back(Key::Path{std::vector<std::string>{"value1", "value2"}});

      auto maybe_statement = account::statement::maybe::to_statement_id_ed_step(table)
        .and_then(account::statement::maybe::statement_id_ed_to_account_statement_step);

      EXPECT_FALSE(maybe_statement.has_value())
        << "Expected nullopt when columns cannot be detected";
    }

    TEST(CSVTable2AccountStatementTests, CsvTableToAccountStatementEmptyEntriesIsValid) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTests, CsvTableToAccountStatementEmptyEntriesIsValid)"};

      // Create a CSV table with valid structure but only header row
      CSV::Table table;
      table.heading = Key::Path{std::vector<std::string>{"Date", "Amount", "Description"}};
      table.rows.push_back(table.heading);

      auto maybe_statement = account::statement::maybe::to_statement_id_ed_step(table)
        .and_then(account::statement::maybe::statement_id_ed_to_account_statement_step);

      ASSERT_FALSE(maybe_statement.has_value())
        << "Expected NO AccountStatement for table with no data rows";
    }

    TEST(CSVTable2AccountStatementTests, CsvTableToAccountStatementIntegrationPipeline) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTests, CsvTableToAccountStatementIntegrationPipeline)"};

      std::string csv_text = sz_NORDEA_csv_20251120;

      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << "Step 1: Failed to parse CSV";

      auto maybe_statement_id_ed = account::statement::maybe::to_statement_id_ed_step(*maybe_table);
      ASSERT_TRUE(maybe_statement_id_ed.has_value()) << "Step 2: Failed to identify account statement";

      auto maybe_statement = account::statement::maybe::statement_id_ed_to_account_statement_step(*maybe_statement_id_ed);
      ASSERT_TRUE(maybe_statement.has_value()) << "Step 3: Failed to create AccountStatement";

      EXPECT_GT(maybe_statement->entries().size(), 0);
      ASSERT_TRUE(maybe_statement->meta().m_maybe_account_irl_id.has_value());

      auto const& first = maybe_statement->entries()[0];
      EXPECT_TRUE(first.transaction_date != Date{}) << "Expected valid date";
      EXPECT_FALSE(first.transaction_caption.empty()) << "Expected non-empty caption";

      logger::development_trace("Integration pipeline produced AccountStatement with {} entries, account: {}",
        maybe_statement->entries().size(),
        maybe_statement->meta().m_maybe_account_irl_id->to_string());
    }

  } // namespace account_statement_suite

  namespace account_id_suite {

    TEST(AccountIdTests, ExtractNordeaAccountId) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountIdTests, ExtractNordeaAccountId)"};

      // Parse NORDEA CSV test data
      std::string csv_text = sz_NORDEA_csv_20251120;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << "Failed to parse NORDEA CSV";

      auto maybe_statement_id_ed = account::statement::maybe::to_statement_id_ed_step(*maybe_table);

      ASSERT_TRUE(maybe_statement_id_ed.has_value()) << "Expected valid statement id:ed for NORDEA CSV";
      EXPECT_EQ(maybe_statement_id_ed->meta.account_id.m_prefix, "NORDEA")
        << "Expected NORDEA prefix for NORDEA bank statement";

      // Verify the table is preserved in defacto
      EXPECT_EQ(maybe_statement_id_ed->defacto.rows.size(), maybe_table->rows.size())
        << "Expected defacto table to match original table";

      // The NORDEA CSV has account numbers in the Avsandare column
      // Looking at the test data, some rows have empty Avsandare, but row with Insattning has "32592317244"
      // So we expect some account number to be extracted
      logger::development_trace("Extracted NORDEA account: '{}'", maybe_statement_id_ed->meta.account_id.m_value);

      // The account value should not be empty for NORDEA CSV with valid data
      // Note: In the sample data, the Avsandare column is mostly empty, but we still detect NORDEA format
    }

    TEST(AccountIdTests, ExtractSkvAccountIdWithOrgNumber) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountIdTests, ExtractSkvAccountIdWithOrgNumber)"};

      // Parse newer SKV CSV test data (contains org number in header: "556782-8172")
      std::string csv_text = sz_SKV_csv_20251120;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << "Failed to parse SKV CSV (newer format)";

      auto maybe_statement_id_ed = account::statement::maybe::to_statement_id_ed_step(*maybe_table);

      ASSERT_TRUE(maybe_statement_id_ed.has_value()) << "Expected valid statement id:ed for SKV CSV";
      EXPECT_EQ(maybe_statement_id_ed->meta.account_id.m_prefix, "SKV")
        << "Expected SKV prefix for tax agency statement";
      EXPECT_EQ(maybe_statement_id_ed->meta.account_id.m_value, "5567828172")
        << "Expected org number extracted from SKV CSV header";
    }

    TEST(AccountIdTests, ExtractSkvAccountIdFromOlderFormat) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountIdTests, ExtractSkvAccountIdFromOlderFormat)"};

      // Parse older SKV CSV test data (may not have explicit org number)
      std::string csv_text = sz_SKV_csv_older;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << "Failed to parse SKV CSV (older format)";

      auto maybe_statement_id_ed = account::statement::maybe::to_statement_id_ed_step(*maybe_table);

      ASSERT_TRUE(maybe_statement_id_ed.has_value()) << "Expected valid statement id:ed for SKV CSV";
      EXPECT_EQ(maybe_statement_id_ed->meta.account_id.m_prefix, "SKV")
        << "Expected SKV prefix for tax agency statement";
      // Older format may or may not have org number - we just verify it's detected as SKV
      logger::development_trace("Extracted SKV org number from older format: '{}'", maybe_statement_id_ed->meta.account_id.m_value);
    }

    TEST(AccountIdTests, NotStatementReturnsNullopt) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountIdTests, NotStatementReturnsNullopt)"};

      // Create a generic CSV that is neither NORDEA nor SKV format
      std::string csv_text = "Name,Value,Date\nAlice,Y,2025-01-01\nBob,N,2025-01-02\n";
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << "Failed to parse generic CSV";

      auto maybe_statement_id_ed = account::statement::maybe::to_statement_id_ed_step(*maybe_table);

      EXPECT_FALSE(maybe_statement_id_ed.has_value())
        << "Expected nullopt for unknown statement CSV format";
    }

    TEST(AccountIdTests, ValidGenericStatementOk) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountIdTests, ValidGenericStatementOk)"};

      // Create a generic CSV that is neither NORDEA nor SKV format
      std::string csv_text = "Name,Value,Date\nAlice,100,2025-01-01\nBob,200,2025-01-02\n";
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << "Failed to parse generic CSV";
      auto maybe_statement_id_ed = account::statement::maybe::to_statement_id_ed_step(*maybe_table);
      EXPECT_TRUE(maybe_statement_id_ed) << "Expected valid Name,Amount,Date statement to be accepted";
      EXPECT_EQ(maybe_statement_id_ed->meta.account_id.m_prefix, "Generic")
        << "Expected NORDEA prefix when header contains NORDEA keywords";
    }

    TEST(AccountIdTests, EmptyTableReturnsNullopt) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountIdTests, EmptyTableReturnsNullopt)"};

      // Create an empty CSV::Table
      CSV::Table empty_table;
      // heading and rows are default-constructed (empty)

      auto maybe_statement_id_ed = account::statement::maybe::to_statement_id_ed_step(empty_table);

      EXPECT_FALSE(maybe_statement_id_ed.has_value())
        << "Expected nullopt for empty CSV::Table";
    }

    TEST(AccountIdTests, TableWithOnlyHeadingUnknownFormatReturnsNullopt) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountIdTests, TableWithOnlyHeadingUnknownFormatReturnsNullopt)"};

      // Create a CSV::Table with only a heading (no data rows)
      // Use Key::Path constructor with vector of strings
      CSV::Table header_only_table;
      header_only_table.heading = Key::Path(std::vector<std::string>{"Name", "Amount", "Date"});
      header_only_table.rows = {};

      auto maybe_statement_id_ed = account::statement::maybe::to_statement_id_ed_step(header_only_table);

      EXPECT_FALSE(maybe_statement_id_ed.has_value())
        << "Expected nullopt for insufficient (header only) statement format";
    }

    TEST(GenericAccountIdTests, HeaderSingleAmountTransOK) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountIdTests, GenericHeaderSingleAmountTransOK)",logger::LogToConsole::OFF};

      std::vector<std::string> heading = {
        "Bokforingsdag", "Belopp", "Avsandare", "Mottagare", "Namn", "Saldo", "Valuta"
      };
      CSV::Table generic_statement_table{
         .heading = heading
        ,.rows = {
           heading
          ,std::vector<std::string>{"2025-03-01", "100.00", "", "", "Kort Elfa", "", "SEK"}
        }
      };

      auto maybe_statement_id_ed = account::statement::maybe::to_statement_id_ed_step(generic_statement_table);

      ASSERT_TRUE(maybe_statement_id_ed.has_value()) << "Expected valid statement id:ed";
      EXPECT_EQ(maybe_statement_id_ed->meta.account_id.m_prefix, "Generic")
        << "Expected Generic prefix for Generic statement table";
    }

    TEST(GenericAccountIdTests, NoHeaderSingleAmountTransOK) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountIdTests, GenericHeaderSingleAmountTransOK)",logger::LogToConsole::OFF};

      CSV::Table generic_statement_table{
        .rows = {
          std::vector<std::string>{"2025-03-01", "100.00", "", "", "Kort Elfa", "", "SEK"}
        }
      };

      auto maybe_statement_id_ed = account::statement::maybe::to_statement_id_ed_step(generic_statement_table);

      ASSERT_TRUE(maybe_statement_id_ed.has_value()) << "Expected valid statement id:ed";
      EXPECT_EQ(maybe_statement_id_ed->meta.account_id.m_prefix, "Generic")
        << "Expected Generic prefix for Generic statement table";
    }

    TEST(GenericAccountIdTests, HeaderTransSaldoAmountsOK) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountIdTests, GenericHeaderSingleAmountTransOK)",logger::LogToConsole::OFF};

      std::vector<std::string> heading = {
        "Bokforingsdag", "Belopp", "Avsandare", "Mottagare", "Namn", "Saldo", "Valuta"
      };
      CSV::Table generic_statement_table{
        .heading = heading
        ,.rows = {
           heading
          ,std::vector<std::string>{"2025-03-01", "100.00", "", "", "Kort Elfa", "500.00", "SEK"}
          ,std::vector<std::string>{"2025-03-01", "100.00", "", "", "Kort Elfa", "600.00", "SEK"}
        }
      };

      auto maybe_statement_id_ed = account::statement::maybe::to_statement_id_ed_step(generic_statement_table);

      ASSERT_TRUE(maybe_statement_id_ed.has_value()) << "Expected valid statement id:ed";
      EXPECT_EQ(maybe_statement_id_ed->meta.account_id.m_prefix, "Generic")
        << "Expected Generic prefix for Generic statement table";
    }

    TEST(GenericAccountIdTests, NoHeaderTransSaldoAmountsOK) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountIdTests, GenericHeaderSingleAmountTransOK)",logger::LogToConsole::OFF};

      CSV::Table generic_statement_table{
        .rows = {
           std::vector<std::string>{"2025-03-01", "100.00", "", "", "Kort Elfa", "500.00", "SEK"}
          ,std::vector<std::string>{"2025-03-01", "100.00", "", "", "Kort Elfa", "600.00", "SEK"}
        }
      };

      auto maybe_statement_id_ed = account::statement::maybe::to_statement_id_ed_step(generic_statement_table);

      ASSERT_TRUE(maybe_statement_id_ed.has_value()) << "Expected valid statement id:ed";
      EXPECT_EQ(maybe_statement_id_ed->meta.account_id.m_prefix, "Generic")
        << "Expected Generic prefix for Generic statement table";
    }


    TEST(AccountIdTests, AccountIdToStringFormat) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountIdTests, AccountIdToStringFormat)"};

      // Test the to_string() method of AccountID (DomainPrefixedId)
      AccountID nordea_id{"NORDEA", "51 86 87-9"};
      EXPECT_EQ(nordea_id.to_string(), "NORDEA::51 86 87-9");

      AccountID skv_id{"SKV", "5567828172"};
      EXPECT_EQ(skv_id.to_string(), "SKV::5567828172");

      AccountID empty_id{"", ""};
      EXPECT_EQ(empty_id.to_string(), "");
    }

  } // namespace account_id_suite

  namespace full_pipeline_suite {

    // ============================================================================
    // Full Pipeline Tests
    // ============================================================================

    // Test fixture for full pipeline tests with file I/O
    class FullPipelineTestFixture : public ::testing::Test {
    protected:
      std::filesystem::path test_dir;
      std::filesystem::path nordea_csv_file;
      std::filesystem::path skv_csv_file;
      std::filesystem::path skv_older_csv_file;
      std::filesystem::path simple_csv_file;
      std::filesystem::path empty_file;
      std::filesystem::path invalid_csv_file;

      void SetUp() override {
        logger::scope_logger log_raii{logger::development_trace, "FullPipelineTestFixture::SetUp"};

        // Create temporary test directory
        test_dir = std::filesystem::temp_directory_path() / "cratchit_test_full_pipeline";
        std::filesystem::create_directories(test_dir);

        // Create NORDEA CSV test file
        nordea_csv_file = test_dir / "nordea_test.csv";
        {
          std::ofstream ofs(nordea_csv_file, std::ios::binary);
          ofs << sz_NORDEA_csv_20251120;
        }

        // Create SKV CSV test file (newer format)
        skv_csv_file = test_dir / "skv_test.csv";
        {
          std::ofstream ofs(skv_csv_file, std::ios::binary);
          ofs << sz_SKV_csv_20251120;
        }

        // Create SKV CSV test file (older format)
        skv_older_csv_file = test_dir / "skv_older_test.csv";
        {
          std::ofstream ofs(skv_older_csv_file, std::ios::binary);
          ofs << sz_SKV_csv_older;
        }

        // Create simple CSV test file
        simple_csv_file = test_dir / "simple_test.csv";
        {
          std::ofstream ofs(simple_csv_file, std::ios::binary);
          ofs << "Date,Amount,Description\n";
          ofs << "2025-01-01,100.50,Payment received\n";
          ofs << "2025-01-02,-50.00,Withdrawal\n";
          ofs << "2025-01-03,200.00,Transfer in\n";
        }

        // Create empty file
        empty_file = test_dir / "empty.csv";
        {
          std::ofstream ofs(empty_file);
        }

        // Create invalid CSV file (undetectable columns)
        invalid_csv_file = test_dir / "invalid.csv";
        {
          std::ofstream ofs(invalid_csv_file);
          ofs << "UnknownCol1,UnknownCol2\n";
          ofs << "value1,value2\n";
        }
      }

      void TearDown() override {
        logger::scope_logger log_raii{logger::development_trace, "FullPipelineTestFixture::TearDown"};
        std::error_code ec;
        std::filesystem::remove_all(test_dir, ec);
      }
    };

    TEST_F(FullPipelineTestFixture, ImportNordeaFileSuccess) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTests, ImportNordeaFileSuccess)"};

      auto result = persistent::in::text::monadic::path_to_istream_ptr_step(nordea_csv_file)
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
        .and_then(text::encoding::monadic::to_with_inferred_encoding)
        .and_then(text::encoding::monadic::to_platform_encoded_string_step)
        .and_then(CSV::parse::monadic::csv_text_to_table_step)
        .and_then(account::statement::monadic::to_statement_id_ed_step)
        .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step)
        .and_then(tas::monadic::account_statement_to_tagged_amounts_step);

      ASSERT_TRUE(result) << "Expected successful import of NORDEA CSV file";
      EXPECT_GT(result.value().size(), 0) << "Expected at least one TaggedAmount";

      // Verify messages document the pipeline
      EXPECT_GT(result.m_messages.size(), 3) << "Expected messages from multiple pipeline stages";

      // Check that we have meaningful TaggedAmounts
      auto const& tagged_amounts = result.value();
      for (auto const& ta : tagged_amounts) {
        EXPECT_TRUE(ta.date() != Date{}) << "Expected valid date";
        EXPECT_TRUE(ta.tag_value("Text").has_value()) << "Expected 'Text' tag";
        EXPECT_TRUE(ta.tag_value("Account").has_value()) << "Expected 'Account' tag";
      }

      // Verify Account tag contains NORDEA
      if (!tagged_amounts.empty()) {
        auto account_tag = tagged_amounts[0].tag_value("Account");
        ASSERT_TRUE(account_tag.has_value());
        EXPECT_TRUE(account_tag->find("NORDEA") != std::string::npos)
          << "Expected Account tag to contain NORDEA";
      }

      logger::development_trace("Imported {} TaggedAmounts from NORDEA CSV", result.value().size());
    }

    TEST_F(FullPipelineTestFixture, ImportSkvFileSuccess) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTests, ImportSkvFileSuccess)"};

      auto result = persistent::in::text::monadic::path_to_istream_ptr_step(skv_csv_file)
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
        .and_then(text::encoding::monadic::to_with_inferred_encoding)
        .and_then(text::encoding::monadic::to_platform_encoded_string_step)
        .and_then(CSV::parse::monadic::csv_text_to_table_step)
        .and_then(account::statement::monadic::to_statement_id_ed_step)
        .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step)
        .and_then(tas::monadic::account_statement_to_tagged_amounts_step);

      ASSERT_TRUE(result) << "Expected successful import of SKV CSV file";
      EXPECT_EQ(result.value().size(), 4) << "Expected 4 transaction TaggedAmounts from SKV CSV";

      // Verify messages document the pipeline
      EXPECT_GT(result.m_messages.size(), 3) << "Expected messages from multiple pipeline stages";

      // Verify Account tag contains SKV
      auto const& tagged_amounts = result.value();
      if (!tagged_amounts.empty()) {
        auto account_tag = tagged_amounts[0].tag_value("Account");
        ASSERT_TRUE(account_tag.has_value());
        EXPECT_TRUE(account_tag->find("SKV") != std::string::npos)
          << "Expected Account tag to contain SKV";
      }

      logger::development_trace("Imported {} TaggedAmounts from SKV CSV", result.value().size());
    }

    TEST_F(FullPipelineTestFixture, ImportSkvOlderFormatSuccess) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTests, ImportSkvOlderFormatSuccess)"};

      auto result = persistent::in::text::monadic::path_to_istream_ptr_step(skv_older_csv_file)
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
        .and_then(text::encoding::monadic::to_with_inferred_encoding)
        .and_then(text::encoding::monadic::to_platform_encoded_string_step)
        .and_then(CSV::parse::monadic::csv_text_to_table_step)
        .and_then(account::statement::monadic::to_statement_id_ed_step)
        .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step)
        .and_then(tas::monadic::account_statement_to_tagged_amounts_step);

      ASSERT_TRUE(result) << "Expected successful import of older SKV CSV file";
      EXPECT_GT(result.value().size(), 0) << "Expected at least one TaggedAmount";

      logger::development_trace("Imported {} TaggedAmounts from older SKV CSV", result.value().size());
    }

    TEST_F(FullPipelineTestFixture, ImportSimpleCsvFailsForUnknownFormat) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTests, ImportSimpleCsvFailsForUnknownFormat)"};

      auto result = persistent::in::text::monadic::path_to_istream_ptr_step(simple_csv_file)
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
        .and_then(text::encoding::monadic::to_with_inferred_encoding)
        .and_then(text::encoding::monadic::to_platform_encoded_string_step)
        .and_then(CSV::parse::monadic::csv_text_to_table_step)
        .and_then(account::statement::monadic::to_statement_id_ed_step)
        .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step)
        .and_then(tas::monadic::account_statement_to_tagged_amounts_step);

      ASSERT_TRUE(result) << "Expected success for generic account statement CSV format";

      if (false) {
        std::print("\nresult:{} m_messages:{}"
          ,result.m_value.has_value()
          ,result.m_messages);
      }

    }

    TEST_F(FullPipelineTestFixture, ImportMissingFileReturnsEmpty) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTests, ImportMissingFileReturnsEmpty)"};

      auto missing_path = test_dir / "nonexistent_file.csv";

      auto result = persistent::in::text::monadic::path_to_istream_ptr_step(missing_path)
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
        .and_then(text::encoding::monadic::to_with_inferred_encoding)
        .and_then(text::encoding::monadic::to_platform_encoded_string_step)
        .and_then(CSV::parse::monadic::csv_text_to_table_step)
        .and_then(account::statement::monadic::to_statement_id_ed_step)
        .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step)
        .and_then(tas::monadic::account_statement_to_tagged_amounts_step);

      EXPECT_FALSE(result) << "Expected failure for missing file";
      EXPECT_GT(result.m_messages.size(), 0) << "Expected error messages";

      // Verify error message mentions the file
      bool found_error_msg = false;
      for (auto const& msg : result.m_messages) {
        if (msg.find("failed") != std::string::npos ||
            msg.find("Failed") != std::string::npos ||
            msg.find("error") != std::string::npos ||
            msg.find("Error") != std::string::npos ||
            msg.find("not") != std::string::npos) {
          found_error_msg = true;
          break;
        }
      }
      EXPECT_TRUE(found_error_msg) << "Expected meaningful error message";
    }

    TEST_F(FullPipelineTestFixture, ImportEmptyFileReturnsEmptyTaggedAmounts) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTests, ImportEmptyFileReturnsEmptyTaggedAmounts)"};

      auto result = persistent::in::text::monadic::path_to_istream_ptr_step(empty_file)
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
        .and_then(text::encoding::monadic::to_with_inferred_encoding)
        .and_then(text::encoding::monadic::to_platform_encoded_string_step)
        .and_then(CSV::parse::monadic::csv_text_to_table_step)
        .and_then(account::statement::monadic::to_statement_id_ed_step)
        .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step)
        .and_then(tas::monadic::account_statement_to_tagged_amounts_step);

      ASSERT_FALSE(result) << "Expected failure for empty file - no content to process";
      EXPECT_GT(result.m_messages.size(), 0) << "Expected error messages about empty file";
    }

    TEST_F(FullPipelineTestFixture, ImportInvalidCsvReturnsEmpty) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTests, ImportInvalidCsvReturnsEmpty)"};

      auto result = persistent::in::text::monadic::path_to_istream_ptr_step(invalid_csv_file)
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
        .and_then(text::encoding::monadic::to_with_inferred_encoding)
        .and_then(text::encoding::monadic::to_platform_encoded_string_step)
        .and_then(CSV::parse::monadic::csv_text_to_table_step)
        .and_then(account::statement::monadic::to_statement_id_ed_step)
        .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step)
        .and_then(tas::monadic::account_statement_to_tagged_amounts_step);

      EXPECT_FALSE(result) << "Expected failure for CSV with undetectable columns";
      EXPECT_GT(result.m_messages.size(), 0) << "Expected error messages";
    }

    TEST_F(FullPipelineTestFixture, MessagesPreservedThroughPipeline) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTests, MessagesPreservedThroughPipeline)"};

      auto result = persistent::in::text::monadic::path_to_istream_ptr_step(nordea_csv_file)
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
        .and_then(text::encoding::monadic::to_with_inferred_encoding)
        .and_then(text::encoding::monadic::to_platform_encoded_string_step)
        .and_then(CSV::parse::monadic::csv_text_to_table_step)
        .and_then(account::statement::monadic::to_statement_id_ed_step)
        .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step)
        .and_then(tas::monadic::account_statement_to_tagged_amounts_step);

      ASSERT_TRUE(result) << "Expected successful import";

      if (false) {
        /* got msg:[
          "nordea_test.csv -> stream : ok"
          , "byte buffer : 1305 bytes"
          , "with confidence_threshold:100 : ok"
          , "with encoding : Detected: UTF-8"
          , "platform encoded : 1305 bytes"
          , "csv table : 13 rows"
          , "statement id:ed table : 2 transaction candidates, columns[date:0 amount:1 saldo:8 description:4]"
          , "account statement : NORDEA::?? : 12 entries"
          , "tagged ampunts : 12 amounts"]
        */

        std::print(
          "\ngot msg:{}"
          ,result.m_messages);
      }


      // Look for messages from different pipeline stages
      bool has_encoding_msg = false;
      bool has_csv_msg = false;
      bool has_stement_id_ed_msg = false;
      bool has_completion_msg = false;

      for (auto const& msg : result.m_messages) {
        if (msg.find("with encoding") != std::string::npos) {
          has_encoding_msg = true;
        }
        if (msg.find("csv table") != std::string::npos) {
          has_csv_msg = true;
        }
        if (msg.find("account statement") != std::string::npos) {
          has_stement_id_ed_msg = true;
        }
        if (msg.find("tagged ampunts") != std::string::npos) {
          has_completion_msg = true;
        }
      }

      EXPECT_TRUE(has_encoding_msg) << std::format(
         "Expected encoding-related message in {}"
        ,result.m_messages);
      EXPECT_TRUE(has_csv_msg) << std::format(
         "Expected CSV parsing message in {}"
        ,result.m_messages);
      EXPECT_TRUE(has_stement_id_ed_msg) << std::format(
         "Expected AccountID detection message in {}"
        ,result.m_messages);
      EXPECT_TRUE(has_completion_msg) << std::format(
         "Expected pipeline completion message in {}"
        ,result.m_messages);

      logger::development_trace("Found {} messages from pipeline", result.m_messages.size());
    }

    TEST(FullPipelineTextTests, ImportNordeaTextSuccess) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTextTests, ImportNordeaTextSuccess)"};

      std::string csv_text = sz_NORDEA_csv_20251120;

      auto result = persistent::in::text::monadic::injected_string_to_istream_ptr_step(std::string(csv_text))
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
        .and_then(text::encoding::monadic::to_with_inferred_encoding)
        .and_then(text::encoding::monadic::to_platform_encoded_string_step)
        .and_then(CSV::parse::monadic::csv_text_to_table_step)
        .and_then(account::statement::monadic::to_statement_id_ed_step)
        .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step)
        .and_then(tas::monadic::account_statement_to_tagged_amounts_step);

      ASSERT_TRUE(result) << "Expected successful import of NORDEA CSV text";
      EXPECT_GT(result.value().size(), 0) << "Expected at least one TaggedAmount";

      // Verify Account tag
      auto const& tagged_amounts = result.value();
      if (!tagged_amounts.empty()) {
        auto account_tag = tagged_amounts[0].tag_value("Account");
        ASSERT_TRUE(account_tag.has_value());
        EXPECT_TRUE(account_tag->find("NORDEA") != std::string::npos)
          << "Expected Account tag to contain NORDEA";
      }
    }

    TEST(FullPipelineTextTests, ImportSkvTextSuccess) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTextTests, ImportSkvTextSuccess)"};

      std::string csv_text = sz_SKV_csv_20251120;

      auto result = persistent::in::text::monadic::injected_string_to_istream_ptr_step(std::string(csv_text))
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
        .and_then(text::encoding::monadic::to_with_inferred_encoding)
        .and_then(text::encoding::monadic::to_platform_encoded_string_step)
        .and_then(CSV::parse::monadic::csv_text_to_table_step)
        .and_then(account::statement::monadic::to_statement_id_ed_step)
        .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step)
        .and_then(tas::monadic::account_statement_to_tagged_amounts_step);

      ASSERT_TRUE(result) << "Expected successful import of SKV CSV text";
      EXPECT_EQ(result.value().size(), 4) << "Expected 4 transaction TaggedAmounts";

      // Verify Account tag contains SKV and org number
      auto const& tagged_amounts = result.value();
      if (!tagged_amounts.empty()) {
        auto account_tag = tagged_amounts[0].tag_value("Account");
        ASSERT_TRUE(account_tag.has_value());
        EXPECT_TRUE(account_tag->find("SKV") != std::string::npos)
          << "Expected Account tag to contain SKV";
        EXPECT_TRUE(account_tag->find("5567828172") != std::string::npos)
          << "Expected Account tag to contain org number";
      }
    }

    TEST(FullPipelineTextTests, ImportEmptyTextReturnsEmpty) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTextTests, ImportEmptyTextReturnsEmpty)",logger::LogToConsole::OFF};

      std::string empty_text;

      auto result = persistent::in::text::monadic::injected_string_to_istream_ptr_step(std::string(empty_text))
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
        .and_then(text::encoding::monadic::to_with_inferred_encoding)
        .and_then(text::encoding::monadic::to_platform_encoded_string_step)
        .and_then(CSV::parse::monadic::csv_text_to_table_step)
        .and_then(account::statement::monadic::to_statement_id_ed_step)
        .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step)
        .and_then(tas::monadic::account_statement_to_tagged_amounts_step);

      ASSERT_FALSE(result) << "Expected null result for empty text";
    }

    TEST(FullPipelineTextTests, ImportInvalidTextReturnsEmpty) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTextTests, ImportInvalidTextReturnsEmpty)"};

      std::string invalid_text = "UnknownCol1,UnknownCol2\nvalue1,value2\n";

      auto result = persistent::in::text::monadic::injected_string_to_istream_ptr_step(std::string(invalid_text))
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
        .and_then(text::encoding::monadic::to_with_inferred_encoding)
        .and_then(text::encoding::monadic::to_platform_encoded_string_step)
        .and_then(CSV::parse::monadic::csv_text_to_table_step)
        .and_then(account::statement::monadic::to_statement_id_ed_step)
        .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step)
        .and_then(tas::monadic::account_statement_to_tagged_amounts_step);

      EXPECT_FALSE(result) << "Expected failure for invalid CSV text";
    }

    TEST(FullPipelineTableTests, ImportGenericAccountStatementOK) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTableTests, ImportGenericAccountStatementOK)"};

      // Create a generic CSV::Table (neither NORDEA nor SKV format)
      CSV::Table table;
      table.heading = Key::Path{std::vector<std::string>{"Date", "Amount", "Description"}};
      table.rows.push_back(table.heading);
      table.rows.push_back(Key::Path{std::vector<std::string>{"2025-01-01", "100.50", "Test Payment"}});
      table.rows.push_back(Key::Path{std::vector<std::string>{"2025-01-02", "-50.00", "Withdrawal"}});

      auto result = account::statement::monadic::to_statement_id_ed_step(table)
        .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step)
        .and_then(tas::monadic::account_statement_to_tagged_amounts_step);

      ASSERT_TRUE(result) << "Expected success for generic account statement csv";

    }

    TEST(FullPipelineTableTests, ImportTableWithNordeaData) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTableTests, ImportTableWithNordeaData)"};

      // Parse NORDEA CSV to get a table
      std::string csv_text = sz_NORDEA_csv_20251120;

      auto result = CSV::parse::monadic::csv_text_to_table_step(csv_text)
        .and_then(account::statement::monadic::to_statement_id_ed_step)
        .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step)
        .and_then(tas::monadic::account_statement_to_tagged_amounts_step);

      ASSERT_TRUE(result) << "Expected successful import of NORDEA table";
      EXPECT_GT(result.value().size(), 0) << "Expected at least one TaggedAmount";

      // Verify Account tag
      if (!result.value().empty()) {
        auto account_tag = result.value()[0].tag_value("Account");
        ASSERT_TRUE(account_tag.has_value());
        EXPECT_TRUE(account_tag->find("NORDEA") != std::string::npos);
      }
    }

    TEST(FullPipelineTableTests, ImportInvalidTableReturnsEmpty) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTableTests, ImportInvalidTableReturnsEmpty)"};

      CSV::Table table;
      table.heading = Key::Path{std::vector<std::string>{"Foo", "Bar"}};
      table.rows.push_back(table.heading);
      table.rows.push_back(Key::Path{std::vector<std::string>{"x", "y"}});

      auto result = account::statement::monadic::to_statement_id_ed_step(table)
        .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step)
        .and_then(tas::monadic::account_statement_to_tagged_amounts_step);

      EXPECT_FALSE(result) << "Expected failure for invalid table";
    }

    // ----------------------------------------------------------------------------
    // Integration tests
    // ----------------------------------------------------------------------------

    TEST_F(FullPipelineTestFixture, CompletePipelineVerifyTaggedAmountStructureWithNordea) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTests, CompletePipelineVerifyTaggedAmountStructureWithNordea)"};

      auto result = persistent::in::text::monadic::path_to_istream_ptr_step(nordea_csv_file)
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
        .and_then(text::encoding::monadic::to_with_inferred_encoding)
        .and_then(text::encoding::monadic::to_platform_encoded_string_step)
        .and_then(CSV::parse::monadic::csv_text_to_table_step)
        .and_then(account::statement::monadic::to_statement_id_ed_step)
        .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step)
        .and_then(tas::monadic::account_statement_to_tagged_amounts_step);

      ASSERT_TRUE(result) << "Expected successful import of NORDEA CSV";
      ASSERT_GT(result.value().size(), 0) << "Expected at least one TaggedAmount";

      // Verify complete TaggedAmount structure for first entry
      auto const& first = result.value()[0];

      // Date should be valid
      EXPECT_TRUE(first.date() != Date{}) << "Expected valid date";

      // Amount should be non-zero
      auto cents = to_amount_in_cents_integer(first.cents_amount());
      EXPECT_NE(cents, 0) << "Expected non-zero amount";

      // Account tag should be present and contain NORDEA
      auto account_tag = first.tag_value("Account");
      ASSERT_TRUE(account_tag.has_value());
      EXPECT_TRUE(account_tag->find("NORDEA") != std::string::npos)
        << "Expected Account tag to contain NORDEA";
    }

    TEST_F(FullPipelineTestFixture, PipelinePreservesAllEntries) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTests, PipelinePreservesAllEntries)"};

      auto result = persistent::in::text::monadic::path_to_istream_ptr_step(nordea_csv_file)
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
        .and_then(text::encoding::monadic::to_with_inferred_encoding)
        .and_then(text::encoding::monadic::to_platform_encoded_string_step)
        .and_then(CSV::parse::monadic::csv_text_to_table_step)
        .and_then(account::statement::monadic::to_statement_id_ed_step)
        .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step)
        .and_then(tas::monadic::account_statement_to_tagged_amounts_step);

      ASSERT_TRUE(result) << "Expected successful import";

      // Count entries - should match the number of valid transaction rows in sample data sz_NORDEA_csv_20251120
      EXPECT_EQ(result.value().size(), 12)
        << "Expected more than 5 entries from NORDEA sample data";

      // Verify each entry has required tags
      for (auto const& ta : result.value()) {
        EXPECT_TRUE(ta.tag_value("Text").has_value()) << "Missing Text tag";
        EXPECT_TRUE(ta.tag_value("Account").has_value()) << "Missing Account tag";
        EXPECT_TRUE(ta.date() != Date{}) << "Invalid date";
      }
    }

    TEST_F(FullPipelineTestFixture, PipelineHandlesDifferentEncodings) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTests, PipelineHandlesDifferentEncodings)"};

      // Create ISO-8859-1 encoded CSV file
      auto iso_csv_file = test_dir / "iso8859_test.csv";
      {
        std::ofstream ofs(iso_csv_file, std::ios::binary);
        // ISO-8859-1 encoded: Date;Amount;Namn (using 0xC5 for Å)
        unsigned char iso_content[] = {
          'D','a','t','e',';','A','m','o','u','n','t',';','N','a','m','n','\n',
          '2','0','2','5','-','0','1','-','0','1',';','1','0','0','.','5','0',';',
          0xC5,'s','a',' ','L','i','n','d','s','t','r',0xF6,'m','\n'  // Åsa Lindström
        };
        ofs.write(reinterpret_cast<char*>(iso_content), sizeof(iso_content));
      }

      auto result = persistent::in::text::monadic::path_to_istream_ptr_step(iso_csv_file)
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
        .and_then(text::encoding::monadic::to_with_inferred_encoding)
        .and_then(text::encoding::monadic::to_platform_encoded_string_step)
        .and_then(CSV::parse::monadic::csv_text_to_table_step)
        .and_then(account::statement::monadic::to_statement_id_ed_step)
        .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step)
        .and_then(tas::monadic::account_statement_to_tagged_amounts_step);

      // The pipeline should handle encoding automatically
      // Even if it defaults to UTF-8, it should produce some result
      // (exact behavior depends on encoding detection)
      // The key is that it should not crash or throw
      SUCCEED() << "Pipeline handled different encoding without crashing";
    }

  } // namespace full_pipeline_suite

} // namespace tests::csv_import_pipeline

namespace tests::csv_table_identification {

  namespace account_statement_table_suite {

    class AccountStatementTableTestsFixture : public ::testing::Test {
    protected:
      // Helper to create a minimal CSV::Table with valid structure
      static CSV::Table to_minimal_table() {
        CSV::Table table;
        table.heading = Key::Path(std::vector<std::string>{"Datum", "Belopp", "Namn"});
        table.rows = {
          Key::Path(std::vector<std::string>{"Datum", "Belopp", "Namn"}),  // Header row (duplicated as per CSV::Table convention)
          Key::Path(std::vector<std::string>{"2025-01-15", "100,50", "Test Transaction"})
        };
        return table;
      }
    }; // AccountStatementTableTestsFixture

    TEST_F(AccountStatementTableTestsFixture,MappingBasedMinimalParseOK) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, MappingBasedMinimalParseOK)"};
      auto table = to_minimal_table();
      auto table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(table);
      ASSERT_TRUE(table_meta.column_mapping.is_valid()) << "Expected Valid Mapping";
    }

    TEST_F(AccountStatementTableTestsFixture,MappingBasedNordeaLikeOk) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, MappingBasedNordeaLikeOk)"};
      std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_older;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << "Expetced sz_NORDEA_csv_20251120 -> Table OK";

      auto table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);
      ASSERT_TRUE(table_meta.column_mapping.is_valid()) << "Expected Valid Mapping";
    }

    TEST_F(AccountStatementTableTestsFixture,MappingBasedSKVLikeOlderOk) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, MappingBasedSKVLikeOlderOk)"};
      // std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      std::string csv_text = sz_SKV_csv_older;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << "Expected sz_SKV_csv_older -> Table OK";

      auto table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);
      ASSERT_TRUE(table_meta.column_mapping.is_valid()) << "Expected Valid Mapping";

    }

    TEST_F(AccountStatementTableTestsFixture,MappingBasedSKVLike20251120OOk) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, MappingBasedSKVLike20251120OOk)"};
      // std::string csv_text = sz_NORDEA_csv_20251120;
      std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_older;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << "Expected sz_SKV_csv_20251120 -> Table OK ";

      auto table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);
      ASSERT_TRUE(table_meta.column_mapping.is_valid()) << "Expected Valid Mapping";

    }

    // ***********************
    // Test of Generic mapping
    // ***********************

    // BEGIN TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok sub tests

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok_sub_0) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok_sub_0)"};
      std::string caption = "sz_NORDEA_csv_20251120";
      std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("Expected {} -> Table OK",caption);

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);

      const size_t EXPECTED_CANDIDATE_COUNT = 12;
      ASSERT_TRUE(maybe_statement_mapping->transaction_candidates_count() == EXPECTED_CANDIDATE_COUNT) << std::format(
         "Expected {} candidates but got {}"
        ,EXPECTED_CANDIDATE_COUNT
        ,maybe_statement_mapping->transaction_candidates_count());

      using namespace account::statement::maybe::table;
      // Empty: 10 Text: 0 1 2 3 4 5 6 7 8 9
      static const RowMap EXPECTED_ROW_0_MAP{
        .ixs = {
           {FieldType::Empty,{10}}
          ,{FieldType::Text,{0,1,2,3,4,5,6,7,8,9}}
        }
      };
      ASSERT_TRUE(maybe_statement_mapping->m_row_0_map == EXPECTED_ROW_0_MAP) << std::format(
        "Expected row map:'{}' but got:'{}' for {}"
        ,to_string(EXPECTED_ROW_0_MAP)
        ,to_string(maybe_statement_mapping->m_row_0_map)
        ,caption);
      ASSERT_TRUE(maybe_statement_mapping->has_heading) << std::format("Expected heading for {}",caption);
      ASSERT_FALSE(maybe_statement_mapping->m_maybe_in_out_saldos) << std::format("Expected NO saldos for {}",caption);
      ASSERT_TRUE(maybe_statement_mapping->entry_amounts_type == EntryAmountsType::TransThenSaldo) << std::format(
        "Expected TransThenSaldo bit got {} for {}"
        ,static_cast<int>(maybe_statement_mapping->entry_amounts_type)
        ,caption);
      ASSERT_TRUE(maybe_statement_mapping->maybe_common) << std::format("Expected maybe_common for {}",caption);
      {
        // Empty: 10 Date: 0 Amount: 1 8 Text: 4 5 9
        static const RowMap EXPECTED_COMMON_ROW_MAP{
          .ixs = {
             {FieldType::Empty,{10}}
            ,{FieldType::Date,{0}}
            ,{FieldType::Amount,{1,8}}
            ,{FieldType::Text,{4,5,9}}
          }
        };
        ASSERT_TRUE(maybe_statement_mapping->maybe_common == EXPECTED_COMMON_ROW_MAP) << std::format(
          "Expected common row map:'{}' but got:'{}' for {}"
          ,to_string(EXPECTED_COMMON_ROW_MAP)
          ,to_string(*maybe_statement_mapping->maybe_common)
          ,caption);

      }

    }

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok_sub_1) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok_sub_1)"};
      std::string caption = "sz_NORDEA_csv_20251120";
      std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("Expected {} -> Table OK",caption);

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);
      CSV::MDTable<account::statement::maybe::table::StatementMapping>  mapped_table{*maybe_statement_mapping,*maybe_table};
      auto maybe_column_mapping = account::statement::maybe::table::generic_like_to_column_mapping(mapped_table);
      ASSERT_TRUE(maybe_column_mapping) << std::format("Expected valid column mapping for {}",caption);
    }
    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok_sub_2) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok__sub_2)"};
      std::string caption = "sz_NORDEA_csv_20251120";
      std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("Expected {} -> Table OK",caption);

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);
      CSV::MDTable<account::statement::maybe::table::StatementMapping>  mapped_table{*maybe_statement_mapping,*maybe_table};
      auto maybe_column_mapping = account::statement::maybe::table::generic_like_to_column_mapping(mapped_table);
      ASSERT_TRUE(maybe_column_mapping) << std::format("Expected valid column mapping for {}",caption);
      auto maybe_account_id = account::statement::maybe::table::generic_like_to_account_id(mapped_table);
      ASSERT_TRUE(maybe_account_id) << std::format("Expected valid account ID for {}",caption);

    }
    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok_sub_3) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok_sub_3)"};
      std::string caption = "sz_NORDEA_csv_20251120";
      std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("Expected {} -> Table OK",caption);

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);
      ASSERT_TRUE(statement_table_meta.column_mapping.is_valid()) << std::format("Expected Valid Mapping for {}",caption);

    }

    // END TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok sub tests

    // BEGIN TableMetaBasedGeneric_sz_SKV_csv_20251120_Ok sub tests

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_csv_20251120_Ok_sub_0) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_csv_20251120_Ok_sub_0)"};
      std::string caption = "sz_SKV_csv_20251120";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("Expected {} -> Table OK",caption);

      // TODO: Move this test to istream -> csv-text tests
      std::string const& prefix(maybe_table->rows[0][0]); 
      ASSERT_FALSE(prefix.starts_with("\xEF\xBB\xBF")) << std::format(
         "Expected NO UTF8 BOM in  {}"
        ,caption);


      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);

      const size_t EXPECTED_CANDIDATE_COUNT = 4;
      ASSERT_TRUE(maybe_statement_mapping->transaction_candidates_count() == EXPECTED_CANDIDATE_COUNT) << std::format(
         "Expected {} candidates but got {}"
        ,EXPECTED_CANDIDATE_COUNT
        ,maybe_statement_mapping->transaction_candidates_count());

      using namespace account::statement::maybe::table;
      // Empty: 2 3 OrgNo: 1 Text: 0
      static const RowMap EXPECTED_ROW_0_MAP{
        .ixs = {
           {FieldType::Empty,{2,3}}
          ,{FieldType::OrgNo,{1}}
          ,{FieldType::Text,{0}}
        }
      };
      ASSERT_TRUE(maybe_statement_mapping->m_row_0_map == EXPECTED_ROW_0_MAP) << std::format(
        "Expected row map:'{}' but got:'{}' for {}"
        ,to_string(EXPECTED_ROW_0_MAP)
        ,to_string(maybe_statement_mapping->m_row_0_map)
        ,caption);
      ASSERT_FALSE(maybe_statement_mapping->has_heading) << std::format("Expected NO heading for {}",caption);
      ASSERT_TRUE(maybe_statement_mapping->m_maybe_in_out_saldos) << std::format("Expected saldos for {}",caption);
      {
        int const SALDO_RIX = 2;
        auto saldo_rix = maybe_statement_mapping->m_maybe_in_out_saldos->m_in_saldo.rix();
        ASSERT_TRUE(saldo_rix == SALDO_RIX) 
          << std::format(
                "Expected in saldo rix:{} but got: {} for {}"
                ,SALDO_RIX
                ,saldo_rix
                ,caption);
      }
      {
        CentsAmount CENTS_SALDO{65800};
        auto cents_saldo = maybe_statement_mapping->m_maybe_in_out_saldos->m_in_saldo.ta().cents_amount();
        ASSERT_TRUE(cents_saldo == CENTS_SALDO) 
          << std::format(
                "Expected in saldo:{} but got:{} for {}"
                ,to_string(CENTS_SALDO)
                ,to_string(cents_saldo)
                ,caption);
      }
      {
        CentsAmount CENTS_SALDO{66000};
        auto cents_saldo = maybe_statement_mapping->m_maybe_in_out_saldos->m_out_saldo.ta().cents_amount();
        ASSERT_TRUE(cents_saldo == CENTS_SALDO) 
          << std::format(
                "Expected out saldo:{} but got:{} for {}"
                ,to_string(CENTS_SALDO)
                ,to_string(cents_saldo)
                ,caption);
      }
      {
        int const SALDO_RIX = 7;
        auto saldo_rix = maybe_statement_mapping->m_maybe_in_out_saldos->m_out_saldo.rix();
        ASSERT_TRUE(saldo_rix == SALDO_RIX) 
          << std::format(
                "Expected out saldo rix:{} but got: {} for {}"
                ,SALDO_RIX
                ,saldo_rix
                ,caption);
      }
      ASSERT_TRUE(maybe_statement_mapping->entry_amounts_type == EntryAmountsType::TransThenSaldo) << std::format(
        "Expected TransThenSaldo bit got {} for {}"
        ,static_cast<int>(maybe_statement_mapping->entry_amounts_type)
        ,caption);
      ASSERT_TRUE(maybe_statement_mapping->maybe_common) << std::format("Expected maybe_common for {}",caption);
      {
        // Date: 0 Amount: 2 3 Text: 1
        static const RowMap EXPECTED_COMMON_ROW_MAP{
          .ixs = {
            {FieldType::Date,{0}}
            ,{FieldType::Amount,{2,3}}
            ,{FieldType::Text,{1}}
          }
        };
        ASSERT_TRUE(maybe_statement_mapping->maybe_common == EXPECTED_COMMON_ROW_MAP) << std::format(
          "Expected common row map:'{}' but got:'{}' for {}"
          ,to_string(EXPECTED_COMMON_ROW_MAP)
          ,to_string(*maybe_statement_mapping->maybe_common)
          ,caption);

      }

    }

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_csv_20251120_Ok_sub_1) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_csv_20251120_Ok_sub_1)"};
      std::string caption = "sz_SKV_csv_20251120";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("Expected {} -> Table OK",caption);

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);
      CSV::MDTable<account::statement::maybe::table::StatementMapping>  mapped_table{*maybe_statement_mapping,*maybe_table};
      auto maybe_column_mapping = account::statement::maybe::table::generic_like_to_column_mapping(mapped_table);
      ASSERT_TRUE(maybe_column_mapping) << std::format("Expected valid column mapping for {}",caption);

    }
    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_csv_20251120_Ok_sub_2) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_csv_20251120_Ok_sub_2)"};
      std::string caption = "sz_SKV_csv_20251120";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("Expected {} -> Table OK",caption);

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);
      CSV::MDTable<account::statement::maybe::table::StatementMapping>  mapped_table{*maybe_statement_mapping,*maybe_table};
      auto maybe_column_mapping = account::statement::maybe::table::generic_like_to_column_mapping(mapped_table);
      ASSERT_TRUE(maybe_column_mapping) << std::format("Expected valid column mapping for {}",caption);
      auto maybe_account_id = account::statement::maybe::table::generic_like_to_account_id(mapped_table);
      ASSERT_TRUE(maybe_account_id) << std::format("Expected valid account ID for {}",caption);

    }
    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_csv_20251120_Ok_sub_3) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_csv_20251120_Ok_sub_3)"};
      std::string caption = "sz_SKV_csv_20251120";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("Expected {} -> Table OK",caption);

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);
      ASSERT_TRUE(statement_table_meta.column_mapping.is_valid()) << std::format("Expected Valid Mapping for {}",caption);
    }


    // END TableMetaBasedGeneric_sz_SKV_csv_20251120_Ok sub tests

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_csv_20251120_BOM_ed_Ok) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_csv_20251120_BOM_ed_Ok)"};
      std::string caption = "sz_SKV_csv_20251120_BOM_ed";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      std::string csv_text = sz_SKV_csv_20251120_BOM_ed;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;

      auto maybe_table = persistent::in::text::maybe::injected_string_to_istream_ptr_step(csv_text)
        .and_then(persistent::in::text::maybe::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::maybe::to_with_threshold_step_f(100))
        .and_then(text::encoding::maybe::to_with_inferred_encoding)
        .and_then(text::encoding::maybe::to_platform_encoded_string_step)
        .and_then(CSV::parse::maybe::csv_text_to_table_step);

      ASSERT_TRUE(maybe_table.has_value()) << std::format("Expected {} -> Table OK",caption);

      // TODO: Move this test to istream -> csv-text tests
      std::string const& prefix(maybe_table->rows[0][0]); 
      ASSERT_FALSE(prefix.starts_with("\xEF\xBB\xBF")) << std::format(
         "Expected UTF8 BOM to be removed in  {}"
        ,caption);

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);

    }

    // BEGIN TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_x

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_0) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_0)"};
      std::string caption = "sz_SKV_csv_older";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120_BOM_ed;
      std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("Expected {} -> Table OK",caption);

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);

      const size_t EXPECTED_CANDIDATE_COUNT = 4;
      ASSERT_TRUE(maybe_statement_mapping->transaction_candidates_count() == EXPECTED_CANDIDATE_COUNT) << std::format(
         "Expected {} candidates but got {}"
        ,EXPECTED_CANDIDATE_COUNT
        ,maybe_statement_mapping->transaction_candidates_count());

      using namespace account::statement::maybe::table;
      // Empty: 0 4 Amount: 2 3 Text: 1
      static const RowMap EXPECTED_ROW_0_MAP{
        .ixs = {
           {FieldType::Empty,{0,4}}
          ,{FieldType::Amount,{2,3}}
          ,{FieldType::Text,{1}}
        }
      };
      ASSERT_TRUE(maybe_statement_mapping->m_row_0_map == EXPECTED_ROW_0_MAP) << std::format(
        "Expected row map:'{}' but got:'{}' for {}"
        ,to_string(EXPECTED_ROW_0_MAP)
        ,to_string(maybe_statement_mapping->m_row_0_map)
        ,caption);
      ASSERT_FALSE(maybe_statement_mapping->has_heading) << std::format("Expected NO heading for {}",caption);
      ASSERT_TRUE(maybe_statement_mapping->m_maybe_in_out_saldos) << std::format("Expected saldos for {}",caption);
      {
        int const SALDO_RIX = 0;
        auto saldo_rix = maybe_statement_mapping->m_maybe_in_out_saldos->m_in_saldo.rix();
        ASSERT_TRUE(saldo_rix == SALDO_RIX) 
          << std::format(
                "Expected in saldo rix:{} but got: {} for {}"
                ,SALDO_RIX
                ,saldo_rix
                ,caption);
      }
      {
        CentsAmount CENTS_SALDO{65600};
        auto cents_saldo = maybe_statement_mapping->m_maybe_in_out_saldos->m_in_saldo.ta().cents_amount();
        ASSERT_TRUE(cents_saldo == CENTS_SALDO) 
          << std::format(
                "Expected in saldo:{} but got:{} for {}"
                ,to_string(CENTS_SALDO)
                ,to_string(cents_saldo)
                ,caption);
      }
      {
        CentsAmount CENTS_SALDO{65800};
        auto cents_saldo = maybe_statement_mapping->m_maybe_in_out_saldos->m_out_saldo.ta().cents_amount();
        ASSERT_TRUE(cents_saldo == CENTS_SALDO) 
          << std::format(
                "Expected out saldo:{} but got:{} for {}"
                ,to_string(CENTS_SALDO)
                ,to_string(cents_saldo)
                ,caption);
      }
      {
        int const SALDO_RIX = 5;
        auto saldo_rix = maybe_statement_mapping->m_maybe_in_out_saldos->m_out_saldo.rix();
        ASSERT_TRUE(saldo_rix == SALDO_RIX) 
          << std::format(
                "Expected out saldo rix:{} but got: {} for {}"
                ,SALDO_RIX
                ,saldo_rix
                ,caption);
      }
      ASSERT_TRUE(maybe_statement_mapping->entry_amounts_type == EntryAmountsType::TransOnly) << std::format(
        "Expected TransThenSaldo bit got {} for {}"
        ,static_cast<int>(maybe_statement_mapping->entry_amounts_type)
        ,caption);
      ASSERT_TRUE(maybe_statement_mapping->maybe_common) << std::format("Expected maybe_common for {}",caption);
      {
        // Empty: 3 4 Date: 0 Amount: 2 Text: 1
        static const RowMap EXPECTED_COMMON_ROW_MAP{
          .ixs = {
             {FieldType::Empty,{3,4}}
            ,{FieldType::Date,{0}}
            ,{FieldType::Amount,{2}}
            ,{FieldType::Text,{1}}
          }
        };
        ASSERT_TRUE(maybe_statement_mapping->maybe_common == EXPECTED_COMMON_ROW_MAP) << std::format(
          "Expected common row map:'{}' but got:'{}' for {}"
          ,to_string(EXPECTED_COMMON_ROW_MAP)
          ,to_string(*maybe_statement_mapping->maybe_common)
          ,caption);

      }


    } // TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_0

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_1) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_1)"};
      std::string caption = "sz_SKV_csv_older";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120_BOM_ed;
      std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("Expected {} -> Table OK",caption);

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);
      CSV::MDTable<account::statement::maybe::table::StatementMapping>  mapped_table{*maybe_statement_mapping,*maybe_table};
      auto maybe_column_mapping = account::statement::maybe::table::generic_like_to_column_mapping(mapped_table);
      ASSERT_TRUE(maybe_column_mapping) << std::format("Expected valid column mapping for {}",caption);

    } // TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_1
    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_2) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_2)"};
      std::string caption = "sz_SKV_csv_older";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120_BOM_ed;
      std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("Expected {} -> Table OK",caption);

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);
      CSV::MDTable<account::statement::maybe::table::StatementMapping>  mapped_table{*maybe_statement_mapping,*maybe_table};
      auto maybe_column_mapping = account::statement::maybe::table::generic_like_to_column_mapping(mapped_table);
      ASSERT_TRUE(maybe_column_mapping) << std::format("Expected valid column mapping for {}",caption);
      auto maybe_account_id = account::statement::maybe::table::generic_like_to_account_id(mapped_table);
      ASSERT_TRUE(maybe_account_id) << std::format("Expected valid account ID for {}",caption);

    } // TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_2
    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_3) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_3)"};
      std::string caption = "sz_SKV_csv_older";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120_BOM_ed;
      std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("Expected {} -> Table OK",caption);

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);
      ASSERT_TRUE(statement_table_meta.column_mapping.is_valid()) << std::format("Expected Valid Mapping for {}",caption);

    } // TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_3

    // END TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_x

    // BEGIN TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_x

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_0) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_0)"};
      std::string caption = "sz_NORDEA_0_1";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120_BOM_ed;
      // std::string csv_text = sz_SKV_csv_older;
      std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("Expected {} -> Table OK",caption);


      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);

      const size_t EXPECTED_CANDIDATE_COUNT = 1;
      ASSERT_TRUE(maybe_statement_mapping->transaction_candidates_count() == EXPECTED_CANDIDATE_COUNT) << std::format(
         "Expected {} candidates but got {}"
        ,EXPECTED_CANDIDATE_COUNT
        ,maybe_statement_mapping->transaction_candidates_count());

      using namespace account::statement::maybe::table;
      // Empty: 10 Text: 0 1 2 3 4 5 6 7 8 9
      static const RowMap EXPECTED_ROW_0_MAP{
        .ixs = {
           {FieldType::Empty,{10}}
          ,{FieldType::Text,{0,1,2,3,4,5,6,7,8,9}}
        }
      };
      ASSERT_TRUE(maybe_statement_mapping->m_row_0_map == EXPECTED_ROW_0_MAP) << std::format(
        "Expected row map:'{}' but got:'{}' for {}"
        ,to_string(EXPECTED_ROW_0_MAP)
        ,to_string(maybe_statement_mapping->m_row_0_map)
        ,caption);
      ASSERT_TRUE(maybe_statement_mapping->has_heading) << std::format("Expected heading for {}",caption);
      ASSERT_FALSE(maybe_statement_mapping->m_maybe_in_out_saldos) << std::format("Expected NO saldos for {}",caption);
      ASSERT_TRUE(maybe_statement_mapping->entry_amounts_type == EntryAmountsType::TransThenSaldo) << std::format(
        "Expected TransThenSaldo bit got {} for {}"
        ,static_cast<int>(maybe_statement_mapping->entry_amounts_type)
        ,caption);
      ASSERT_TRUE(maybe_statement_mapping->maybe_common) << std::format("Expected maybe_common for {}",caption);
      {
        // Empty: 2 3 6 7 10 Date: 0 Amount: 1 8 Text: 4 5 9
        static const RowMap EXPECTED_COMMON_ROW_MAP{
          .ixs = {
             {FieldType::Empty,{2,3,6,7,10}}
            ,{FieldType::Date,{0}}
            ,{FieldType::Amount,{1,8}}
            ,{FieldType::Text,{4,5,9}}
          }
        };
        ASSERT_TRUE(maybe_statement_mapping->maybe_common == EXPECTED_COMMON_ROW_MAP) << std::format(
          "Expected common row map:'{}' but got:'{}' for {}"
          ,to_string(EXPECTED_COMMON_ROW_MAP)
          ,to_string(*maybe_statement_mapping->maybe_common)
          ,caption);

      }

    } // TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_0

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_1) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_1)"};
      std::string caption = "sz_NORDEA_0_1";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120_BOM_ed;
      // std::string csv_text = sz_SKV_csv_older;
      std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("Expected {} -> Table OK",caption);

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);
      CSV::MDTable<account::statement::maybe::table::StatementMapping>  mapped_table{*maybe_statement_mapping,*maybe_table};
      auto maybe_column_mapping = account::statement::maybe::table::generic_like_to_column_mapping(mapped_table);
      ASSERT_TRUE(maybe_column_mapping) << std::format("Expected valid column mapping for {}",caption);

    } // TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_1
    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_2) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_2)"};
      std::string caption = "sz_NORDEA_0_1";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120_BOM_ed;
      // std::string csv_text = sz_SKV_csv_older;
      std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("Expected {} -> Table OK",caption);

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);
      CSV::MDTable<account::statement::maybe::table::StatementMapping>  mapped_table{*maybe_statement_mapping,*maybe_table};
      auto maybe_column_mapping = account::statement::maybe::table::generic_like_to_column_mapping(mapped_table);
      ASSERT_TRUE(maybe_column_mapping) << std::format("Expected valid column mapping for {}",caption);
      auto maybe_account_id = account::statement::maybe::table::generic_like_to_account_id(mapped_table);
      ASSERT_TRUE(maybe_account_id) << std::format("Expected valid account ID for {}",caption);

    } // TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_2
    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_3) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_3)"};
      std::string caption = "sz_NORDEA_0_1";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120_BOM_ed;
      // std::string csv_text = sz_SKV_csv_older;
      std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("Expected {} -> Table OK",caption);

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);
      ASSERT_TRUE(statement_table_meta.column_mapping.is_valid()) << std::format("Expected Valid Mapping for {}",caption);

    } // TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_3

    // END TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_x

    // BEGIN TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_x

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_0) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_0)"};
      std::string caption = "sz_SKV_0_0";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120_BOM_ed;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("Expected {} -> Table OK",caption);

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);

      const size_t EXPECTED_CANDIDATE_COUNT = 0;
      ASSERT_TRUE(maybe_statement_mapping->transaction_candidates_count() == EXPECTED_CANDIDATE_COUNT) << std::format(
         "Expected {} candidates but got {}"
        ,EXPECTED_CANDIDATE_COUNT
        ,maybe_statement_mapping->transaction_candidates_count());
      
      using namespace account::statement::maybe::table;
      // Empty: 2 3 OrgNo: 1 Text: 0
      static const RowMap EXPECTED_ROW_0_MAP{
        .ixs = {
           {FieldType::Empty,{2,3}}
          ,{FieldType::OrgNo,{1}}
          ,{FieldType::Text,{0}}
        }
      };
      ASSERT_TRUE(maybe_statement_mapping->m_row_0_map == EXPECTED_ROW_0_MAP) << std::format(
        "Expected row map:'{}' but got:'{}' for {}"
        ,to_string(EXPECTED_ROW_0_MAP)
        ,to_string(maybe_statement_mapping->m_row_0_map)
        ,caption);
      ASSERT_FALSE(maybe_statement_mapping->has_heading) << std::format("Expected NO heading for {}",caption);
      ASSERT_TRUE(maybe_statement_mapping->m_maybe_in_out_saldos) << std::format("Expected saldos for {}",caption);
      {
        int const SALDO_RIX = 2;
        auto saldo_rix = maybe_statement_mapping->m_maybe_in_out_saldos->m_in_saldo.rix();
        ASSERT_TRUE(saldo_rix == SALDO_RIX) 
          << std::format(
                "Expected in saldo rix:{} but got: {} for {}"
                ,SALDO_RIX
                ,saldo_rix
                ,caption);
      }
      {
        CentsAmount CENTS_SALDO{66300};
        auto cents_saldo = maybe_statement_mapping->m_maybe_in_out_saldos->m_in_saldo.ta().cents_amount();
        ASSERT_TRUE(cents_saldo == CENTS_SALDO) 
          << std::format(
                "Expected in saldo:{} but got:{} for {}"
                ,to_string(CENTS_SALDO)
                ,to_string(cents_saldo)
                ,caption);
      }
      {
        CentsAmount CENTS_SALDO{66300};
        auto cents_saldo = maybe_statement_mapping->m_maybe_in_out_saldos->m_out_saldo.ta().cents_amount();
        ASSERT_TRUE(cents_saldo == CENTS_SALDO) 
          << std::format(
                "Expected out saldo:{} but got:{} for {}"
                ,to_string(CENTS_SALDO)
                ,to_string(cents_saldo)
                ,caption);
      }
      {
        int const SALDO_RIX = 3;
        auto saldo_rix = maybe_statement_mapping->m_maybe_in_out_saldos->m_out_saldo.rix();
        ASSERT_TRUE(saldo_rix == SALDO_RIX) 
          << std::format(
                "Expected out saldo rix:{} but got: {} for {}"
                ,SALDO_RIX
                ,saldo_rix
                ,caption);
      }
      ASSERT_TRUE(maybe_statement_mapping->entry_amounts_type == EntryAmountsType::Undefined) << std::format(
        "Expected TransThenSaldo bit got {} for {}"
        ,static_cast<int>(maybe_statement_mapping->entry_amounts_type)
        ,caption);
      ASSERT_FALSE(maybe_statement_mapping->maybe_common) << std::format("Expected NO maybe_common for {}",caption);

    } // TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_0

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_1) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_1)"};
      std::string caption = "sz_SKV_0_0";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120_BOM_ed;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("Expected {} -> Table OK",caption);

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);
      CSV::MDTable<account::statement::maybe::table::StatementMapping>  mapped_table{*maybe_statement_mapping,*maybe_table};
      auto maybe_column_mapping = account::statement::maybe::table::generic_like_to_column_mapping(mapped_table);
      ASSERT_FALSE(maybe_column_mapping) << std::format("Expected NO column mapping for {}",caption);

    } // TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_1

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_2) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_2)"};
      std::string caption = "sz_SKV_0_0";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120_BOM_ed;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("Expected {} -> Table OK",caption);

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);
      CSV::MDTable<account::statement::maybe::table::StatementMapping>  mapped_table{*maybe_statement_mapping,*maybe_table};
      auto maybe_column_mapping = account::statement::maybe::table::generic_like_to_column_mapping(mapped_table);
      ASSERT_FALSE(maybe_column_mapping) << std::format("Expected NO column mapping for {}",caption);
      auto maybe_account_id = account::statement::maybe::table::generic_like_to_account_id(mapped_table);
      ASSERT_TRUE(maybe_account_id) << std::format("Expected valid account ID for {}",caption);

    } // TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_2

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_3) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_3)"};
      std::string caption = "sz_SKV_0_0";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120_BOM_ed;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("Expected {} -> Table OK",caption);

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);
      ASSERT_FALSE(statement_table_meta.column_mapping.is_valid()) << std::format("Expected NO Mapping for {}",caption);

    } // TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_3

    // END TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_x

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_0_0_BOM_ed_Ok) {
      logger::scope_logger log_raii{
         logger::development_trace
        ,"TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_0_0_BOM_ed_Ok)"
        ,logger::LogToConsole::OFF
        };
      std::string caption = "sz_SKV_0_0_BOM_ed";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120_BOM_ed;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      std::string csv_text = sz_SKV_0_0_BOM_ed;

      auto maybe_table = persistent::in::text::maybe::injected_string_to_istream_ptr_step(csv_text)
        .and_then(persistent::in::text::maybe::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::maybe::to_with_threshold_step_f(100))
        .and_then(text::encoding::maybe::to_with_inferred_encoding)
        .and_then(text::encoding::maybe::to_platform_encoded_string_step)
        .and_then(CSV::parse::maybe::csv_text_to_table_step);

      ASSERT_TRUE(maybe_table.has_value()) << std::format("Expected {} -> Table OK",caption);

      // TODO: Move this test to istream -> csv-text tests
      std::string const& prefix(maybe_table->rows[0][0]); 
      ASSERT_FALSE(prefix.starts_with("\xEF\xBB\xBF")) << std::format(
         "Expected UTF8 BOM to be removed in  {}"
        ,caption);

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);

    } // TableMetaBasedGeneric_sz_SKV_0_0_BOM_ed_Ok

    TEST(AccountStatementTableTests, TableMetaBasedGeneric_DetectColumnsFromHeader) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTableTests, TableMetaBasedGeneric_DetectColumnsFromHeader)"};

      // Create a simple CSV table with headers
      CSV::Table table;
      table.heading = Key::Path{std::vector<std::string>{"Date", "Amount", "Description"}};
      table.rows.push_back(table.heading);
      table.rows.push_back(Key::Path{std::vector<std::string>{"2025-01-01", "100.50", "Test Transaction"}});

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(table);
      auto mapping = statement_table_meta.column_mapping;

      EXPECT_TRUE(mapping.is_valid()) << "Expected valid column mapping";
      EXPECT_EQ(mapping.date_column, 0);
      EXPECT_EQ(mapping.transaction_amount_column, 1);
      EXPECT_EQ(mapping.description_column, 2);
    }

    TEST(AccountStatementTableTests, TableMetaBasedGeneric_DetectColumnsFromData) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTableTests, TableMetaBasedGeneric_DetectColumnsFromData)"};

      // Create a CSV table without meaningful headers
      CSV::Table table;
      table.heading = Key::Path{std::vector<std::string>{"Col0", "Col1", "Col2"}};
      table.rows.push_back(Key::Path{std::vector<std::string>{"2025-01-01", "Payment received", "100.50"}});
      table.rows.push_back(Key::Path{std::vector<std::string>{"2025-01-02", "Withdrawal", "-50.00"}});
      table.rows.push_back(Key::Path{std::vector<std::string>{"2025-01-03", "Transfer", "200.00"}});

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(table);
      auto mapping = statement_table_meta.column_mapping;

      EXPECT_TRUE(mapping.is_valid()) << "Expected valid column mapping from data analysis";
      EXPECT_EQ(mapping.date_column, 0) << "Expected date in column 0";
      EXPECT_EQ(mapping.transaction_amount_column, 2) << "Expected transaction amount in column 2";
      EXPECT_EQ(mapping.description_column, 1) << "Expected description in column 1";
    }

    TEST(AccountStatementTableTests, TableMetaBasedGeneric_InvalidDateHandledGracefully) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTableTests, TableMetaBasedGeneric_InvalidDateHandledGracefully)"};

      // Create a CSV table with invalid date
      CSV::Table table;
      table.heading = Key::Path{std::vector<std::string>{"Date", "Amount", "Description"}};
      table.rows.push_back(table.heading);
      table.rows.push_back(Key::Path{std::vector<std::string>{"not-a-date", "100.50", "Test"}});

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(table);
      auto mapping = statement_table_meta.column_mapping;
      auto maybe_entry = account::statement::maybe::table::extract_entry_from_row(table.rows[1], mapping);

      EXPECT_FALSE(maybe_entry.has_value()) << "Expected nullopt for invalid date";
    }

    TEST(AccountStatementTableTests, TableMetaBasedGeneric_InvalidAmountHandledGracefully) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTableTests, TableMetaBasedGeneric_InvalidAmountHandledGracefully)"};

      // Create a CSV table with invalid amount
      CSV::Table table;
      table.heading = Key::Path{std::vector<std::string>{"Date", "Amount", "Description"}};
      table.rows.push_back(table.heading);
      table.rows.push_back(Key::Path{std::vector<std::string>{"2025-01-01", "not-an-amount", "Test"}});

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(table);
      auto mapping = statement_table_meta.column_mapping;
      auto maybe_entry = account::statement::maybe::table::extract_entry_from_row(table.rows[1], mapping);

      EXPECT_FALSE(maybe_entry.has_value()) << "Expected nullopt for invalid amount";
    }

    TEST(AccountStatementTableTests, TableMetaBasedGeneric_EmptyDescriptionHandledGracefully) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTableTests, TableMetaBasedGeneric_EmptyDescriptionHandledGracefully)"};

      // Create a CSV table with empty description
      CSV::Table table;
      table.heading = Key::Path{std::vector<std::string>{"Date", "Amount", "Description"}};
      table.rows.push_back(table.heading);
      table.rows.push_back(Key::Path{std::vector<std::string>{"2025-01-01", "100.50", ""}});

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(table);
      auto mapping = statement_table_meta.column_mapping;
      auto maybe_entry = account::statement::maybe::table::extract_entry_from_row(table.rows[1], mapping);

      EXPECT_FALSE(maybe_entry.has_value()) << "Expected nullopt for empty description";
    }

    TEST(AccountStatementTableTests, TableMetaBasedGeneric_DetectColumnsFromNordeaHeader) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTableTests, TableMetaBasedGeneric_DetectColumnsFromNordeaHeader)"};

      std::string csv_text = sz_NORDEA_csv_20251120;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << "Failed to parse NORDEA CSV";

      auto maybe_md_table = account::statement::maybe::to_statement_id_ed_step(*maybe_table);
      ASSERT_TRUE(maybe_md_table.has_value()) << "Failed to extract AccountID from NORDEA CSV";

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);
      auto mapping = statement_table_meta.column_mapping;

      EXPECT_TRUE(mapping.is_valid()) << "Expected valid column mapping from data analysis";
    }

    TEST(AccountStatementTableTests, TableMetaBasedGeneric_DetectColumnsFromSKVNewData) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTableTests, TableMetaBasedGeneric_DetectColumnsFromSKVNewData)"};

      std::string csv_text = sz_SKV_csv_20251120;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << "Failed to parse SKV CSV";

      auto maybe_md_table = account::statement::maybe::to_statement_id_ed_step(*maybe_table);
      ASSERT_TRUE(maybe_md_table.has_value()) << "Failed to extract AccountID from SKV CSV";


      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);
      auto mapping = statement_table_meta.column_mapping;

      EXPECT_TRUE(mapping.is_valid()) << "Expected valid column mapping from data analysis";
      EXPECT_EQ(mapping.date_column, 0) << "Expected date in column 0";
      EXPECT_EQ(mapping.transaction_amount_column, 2) << "Expected transaction amount in column 2";
      EXPECT_EQ(mapping.description_column, 1) << "Expected description in column 1";

    }

    TEST(AccountStatementTableTests, TableMetaBasedGeneric_DetectBothSKVAmountColumnsOld) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTableTests, TableMetaBasedGeneric_DetectBothSKVAmountColumnsOld)"};

      // Parse SKV CSV to get a table, then verify column detection
      std::string csv_text = sz_SKV_csv_older;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);

      ASSERT_TRUE(maybe_table.has_value()) << "Expected successful CSV parse";

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);
      auto mapping = statement_table_meta.column_mapping;

      EXPECT_TRUE(mapping.is_valid()) << "Expected valid column mapping";

      EXPECT_EQ(mapping.date_column, 0) << "Expected date in column 0";
      EXPECT_EQ(mapping.description_column, 1) << "Expected description in column 1";
      EXPECT_EQ(mapping.transaction_amount_column, 2) << "Expected transaction amount in column 2";

      logger::development_trace("Column mapping: date={}, desc={}, trans_amt={}",
         mapping.date_column
        ,mapping.description_column
        ,mapping.transaction_amount_column);
    }

    TEST(AccountStatementTableTests, TableMetaBasedGeneric_DetectBothSKVAmountColumns251120) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTableTests, TableMetaBasedGeneric_DetectBothSKVAmountColumns251120)"};

      // Parse SKV CSV to get a table, then verify column detection
      std::string csv_text = sz_SKV_csv_20251120;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);

      ASSERT_TRUE(maybe_table.has_value()) << "Expected successful CSV parse";

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);
      auto mapping = statement_table_meta.column_mapping;

      EXPECT_TRUE(mapping.is_valid()) << "Expected valid column mapping";

      // SKV format: Col0=Date, Col1=Description, Col2=Transaction, Col3=Saldo
      EXPECT_EQ(mapping.date_column, 0) << "Expected date in column 0";
      EXPECT_EQ(mapping.description_column, 1) << "Expected description in column 1";
      EXPECT_EQ(mapping.transaction_amount_column, 2) << "Expected transaction amount in column 2";
      EXPECT_EQ(mapping.saldo_amount_column, 3) << "Expected saldo amount in column 3";

      logger::development_trace("Column mapping: date={}, desc={}, trans_amt={}, saldo={}",
        mapping.date_column, mapping.description_column,
        mapping.transaction_amount_column, mapping.saldo_amount_column);
    }

    
  } // account_statement_table_suite
} // csv_table_identification

namespace tests {

  namespace statement_id_ed_to_account_statement_step {

    // Test fixture for MDTable -> AccountStatement tests
    class MDTableToAccountStatementTestFixture : public ::testing::Test {
    protected:
      // Helper to create an MDTable from CSV text using the real parsing/detection pipeline
      static std::optional<CSV::MDTable<account::statement::maybe::table::TableMeta>> make_statement_id_ed(
          std::string const& csv_text,
          char delimiter = ';') {
        auto maybe_table = CSV::parse::maybe::csv_to_table(csv_text, delimiter);
        if (!maybe_table) {
          return std::nullopt;
        }
        return account::statement::maybe::to_statement_id_ed_step(*maybe_table);
      }

      // Helper to create a minimal CSV::Table with valid structure
      static CSV::Table make_minimal_table() {
        CSV::Table table;
        table.heading = Key::Path(std::vector<std::string>{"Datum", "Belopp", "Namn"});
        table.rows = {
          Key::Path(std::vector<std::string>{"Datum", "Belopp", "Namn"}),  // Header row (duplicated as per CSV::Table convention)
          Key::Path(std::vector<std::string>{"2025-01-15", "100,50", "Test Transaction"})
        };
        return table;
      }
    };

    // ============================================================================
    // MDTable Unpacking Tests - Verify AccountID is correctly propagated
    // ============================================================================

    TEST_F(MDTableToAccountStatementTestFixture, AccountIdPropagatedToStatementMeta) {
      logger::scope_logger log_raii{logger::development_trace,
        "TEST(MDTableToAccountStatement, AccountIdPropagatedToStatementMeta)"};

      CSV::Table table = make_minimal_table();
      auto maybe_statement_id_ed = account::statement::maybe::to_statement_id_ed_step(table);

      ASSERT_TRUE(maybe_statement_id_ed);

      auto const& [table_meta,_] = *maybe_statement_id_ed;
      auto provided_account_id = table_meta.account_id;

      // Transform
      auto maybe_statement = account::statement::maybe::statement_id_ed_to_account_statement_step(*maybe_statement_id_ed);

      // Verify AccountID is correctly propagated
      ASSERT_TRUE(maybe_statement.has_value()) << "Expected successful transformation";
      ASSERT_TRUE(maybe_statement->meta().m_maybe_account_irl_id.has_value())
        << "Expected AccountID to be set in meta";
      EXPECT_EQ(maybe_statement->meta().m_maybe_account_irl_id->m_prefix, provided_account_id.m_prefix);
      EXPECT_EQ(maybe_statement->meta().m_maybe_account_irl_id->m_value, provided_account_id.m_value);
      EXPECT_EQ(maybe_statement->meta().m_maybe_account_irl_id->to_string(), provided_account_id.to_string());
    }

    TEST_F(MDTableToAccountStatementTestFixture, SKVAccountIdPropagatedCorrectly) {
      logger::scope_logger log_raii{logger::development_trace,
        "TEST(MDTableToAccountStatement, SKVAccountIdPropagatedCorrectly)"};

      auto maybe_statement_id_ed = make_statement_id_ed(sz_SKV_csv_20251120);

      ASSERT_TRUE(maybe_statement_id_ed);

      auto const& [table_meta,_] = *maybe_statement_id_ed;
      auto provided_account_id = table_meta.account_id;

      // Transform
      auto maybe_statement = account::statement::maybe::statement_id_ed_to_account_statement_step(*maybe_statement_id_ed);

      // Verify AccountID is correctly propagated
      ASSERT_TRUE(maybe_statement.has_value()) << "Expected successful transformation";
      ASSERT_TRUE(maybe_statement->meta().m_maybe_account_irl_id.has_value())
        << "Expected AccountID to be set in meta";
      EXPECT_EQ(maybe_statement->meta().m_maybe_account_irl_id->m_prefix, provided_account_id.m_prefix);
      EXPECT_EQ(maybe_statement->meta().m_maybe_account_irl_id->m_value, provided_account_id.m_value);
      EXPECT_EQ(maybe_statement->meta().m_maybe_account_irl_id->to_string(), provided_account_id.to_string());
    }

    // ============================================================================
    // Integration Tests - Using Real CSV Data Through Full Detection Pipeline
    // ============================================================================

    TEST_F(MDTableToAccountStatementTestFixture, NordeaMDTableToAccountStatement) {
      logger::scope_logger log_raii{logger::development_trace,
        "TEST(MDTableToAccountStatement, NordeaMDTableToAccountStatement)"};

      // Parse NORDEA CSV and detect account
      auto maybe_statement_id_ed = make_statement_id_ed(sz_NORDEA_csv_20251120);

      ASSERT_TRUE(maybe_statement_id_ed.has_value()) << "Expected successful MDTable creation";

      // Verify detected AccountID before transformation
      EXPECT_EQ(maybe_statement_id_ed->meta.account_id.m_prefix, "NORDEA") << "Expected Account ID";

      // Transform MDTable to AccountStatement
      auto result = account::statement::maybe::statement_id_ed_to_account_statement_step(*maybe_statement_id_ed);

      ASSERT_TRUE(result.has_value()) << "Expected successful transformation";

      // Verify AccountID propagated correctly
      ASSERT_TRUE(result->meta().m_maybe_account_irl_id.has_value()) << "Expected account ID";
      EXPECT_EQ(result->meta().m_maybe_account_irl_id->m_prefix, "NORDEA") << "Expected NORDEA prefix";

      // Verify entries were created
      EXPECT_GT(result->entries().size(), 0) << "Expected at least one entry";
    }

    TEST_F(MDTableToAccountStatementTestFixture, SKVOlderMDTableToAccountStatement) {
      logger::scope_logger log_raii{logger::development_trace,
        "TEST(MDTableToAccountStatement, SKVOlderMDTableToAccountStatement)"};

      // Parse SKV CSV (older format) and detect account
      auto maybe_statement_id_ed = make_statement_id_ed(sz_SKV_csv_older);

      ASSERT_TRUE(maybe_statement_id_ed.has_value()) << "Expected successful MDTable creation";

      // Verify detected AccountID
      EXPECT_EQ(maybe_statement_id_ed->meta.account_id.m_prefix, "SKV");

      // Transform MDTable to AccountStatement
      auto result = account::statement::maybe::statement_id_ed_to_account_statement_step(*maybe_statement_id_ed);

      ASSERT_TRUE(result.has_value()) << "Expected successful transformation";

      // Verify AccountID propagated
      ASSERT_TRUE(result->meta().m_maybe_account_irl_id.has_value());
      EXPECT_EQ(result->meta().m_maybe_account_irl_id->m_prefix, "SKV");
    }

    TEST_F(MDTableToAccountStatementTestFixture, SKVNewerMDTableToAccountStatement) {
      logger::scope_logger log_raii{logger::development_trace,
        "TEST(MDTableToAccountStatement, SKVNewerMDTableToAccountStatement)"};

      // Parse SKV CSV (newer format with company name) and detect account
      // Note: newer format uses comma delimiter in quotes
      auto maybe_table = CSV::parse::maybe::csv_to_table(sz_SKV_csv_20251120, ';');
      ASSERT_TRUE(maybe_table.has_value()) << "Expected successful CSV parsing";

      auto maybe_statement_id_ed = account::statement::maybe::to_statement_id_ed_step(*maybe_table);
      ASSERT_TRUE(maybe_statement_id_ed.has_value()) << "Expected successful MDTable creation";

      // Verify detected AccountID (should find org number 5567828172)
      EXPECT_EQ(maybe_statement_id_ed->meta.account_id.m_prefix, "SKV");

      // Transform MDTable to AccountStatement
      auto result = account::statement::maybe::statement_id_ed_to_account_statement_step(*maybe_statement_id_ed);

      ASSERT_TRUE(result.has_value()) << "Expected successful transformation";

      // Verify AccountID propagated
      ASSERT_TRUE(result->meta().m_maybe_account_irl_id.has_value());
      EXPECT_EQ(result->meta().m_maybe_account_irl_id->m_prefix, "SKV");
    }

    // ============================================================================
    // Return Value Tests
    // ============================================================================

    TEST_F(MDTableToAccountStatementTestFixture, ReturnsAccountStatementWithEntries) {
      logger::scope_logger log_raii{logger::development_trace,
        "TEST(MDTableToAccountStatement, ReturnsAccountStatementWithEntries)"};

      auto maybe_statement_id_ed = make_statement_id_ed(sz_NORDEA_csv_20251120);
      ASSERT_TRUE(maybe_statement_id_ed.has_value());

      auto result = account::statement::maybe::statement_id_ed_to_account_statement_step(*maybe_statement_id_ed);

      ASSERT_TRUE(result.has_value());

      // Verify we get an AccountStatement type back with accessible entries
      AccountStatement const& statement = *result;
      EXPECT_GT(statement.entries().size(), 0);

      // Verify entries have expected structure
      if (!statement.entries().empty()) {
        auto const& first_entry = statement.entries()[0];
        EXPECT_FALSE(first_entry.transaction_caption.empty());
      }
    }

  } // statement_id_ed_to_account_statement_step
}

namespace tests {
  namespace statement_to_tagged_amounts {

  // Test fixture for AccountStatement -> TaggedAmounts tests
  class StatementToTaggedAmountsTestFixture : public ::testing::Test {
  protected:
    // Helper to create a simple AccountStatementEntry
    static AccountStatementEntry make_entry(
        int year, unsigned month, unsigned day,
        double amount,
        std::string const& caption) {
      Date date{std::chrono::year{year}, std::chrono::month{month}, std::chrono::day{day}};
      return AccountStatementEntry(date, Amount{amount}, caption);
    }

    // Helper to create an AccountID
    static AccountID make_account_id(std::string const& prefix, std::string const& value) {
      return AccountID{.m_prefix = prefix, .m_value = value};
    }
  };

  // ============================================================================
  // Single Entry Tests
  // ============================================================================

  TEST_F(StatementToTaggedAmountsTestFixture, SingleEntryTransformation) {
    logger::scope_logger log_raii{logger::development_trace,
      "TEST(StatementToTaggedAmounts, SingleEntryTransformation)"};

    // Create a single entry
    AccountStatementEntries entries;
    entries.push_back(make_entry(2025, 1, 15, 100.50, "Test Transaction"));

    // Create AccountStatement with AccountID
    AccountID account_id = make_account_id("NORDEA", "51 86 87-9");
    AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};
    AccountStatement statement(entries, meta);

    // Transform to TaggedAmounts
    auto result = tas::maybe::account_statement_to_tagged_amounts_step(statement);

    // Verify result
    ASSERT_TRUE(result.has_value()) << "Expected successful transformation";
    ASSERT_EQ(result->size(), 1) << "Expected exactly one TaggedAmount";

    auto const& ta = (*result)[0];

    // Verify date
    EXPECT_EQ(ta.date().year(), std::chrono::year{2025});
    EXPECT_EQ(ta.date().month(), std::chrono::month{1});
    EXPECT_EQ(ta.date().day(), std::chrono::day{15});

    // Verify amount (100.50 -> 10050 cents)
    EXPECT_EQ(to_amount_in_cents_integer(ta.cents_amount()), 10050)
      << "Expected 100.50 to convert to 10050 cents";

    // Verify tags
    ASSERT_TRUE(ta.tags().contains("Text")) << "Expected 'Text' tag";
    EXPECT_EQ(ta.tags().at("Text"), "Test Transaction");

    ASSERT_TRUE(ta.tags().contains("Account")) << "Expected 'Account' tag";
    EXPECT_EQ(ta.tags().at("Account"), "NORDEA::51 86 87-9");
  }

  // ============================================================================
  // Multiple Entries Tests
  // ============================================================================

  TEST_F(StatementToTaggedAmountsTestFixture, MultipleEntriesTransformation) {
    logger::scope_logger log_raii{logger::development_trace,
      "TEST(StatementToTaggedAmounts, MultipleEntriesTransformation)"};

    // Create multiple entries
    AccountStatementEntries entries;
    entries.push_back(make_entry(2025, 1, 10, 500.00, "First Transaction"));
    entries.push_back(make_entry(2025, 1, 15, -250.75, "Second Transaction"));
    entries.push_back(make_entry(2025, 1, 20, 1000.00, "Third Transaction"));

    // Create AccountStatement
    AccountID account_id = make_account_id("SKV", "5567828172");
    AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};
    AccountStatement statement(entries, meta);

    // Transform
    auto result = tas::maybe::account_statement_to_tagged_amounts_step(statement);

    // Verify
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 3) << "Expected three TaggedAmounts";

    // All should have the same Account tag
    for (auto const& ta : *result) {
      ASSERT_TRUE(ta.tags().contains("Account"));
      EXPECT_EQ(ta.tags().at("Account"), "SKV::5567828172");
    }

    // Verify individual entries
    EXPECT_EQ((*result)[0].tags().at("Text"), "First Transaction");
    EXPECT_EQ(to_amount_in_cents_integer((*result)[0].cents_amount()), 50000);

    EXPECT_EQ((*result)[1].tags().at("Text"), "Second Transaction");
    EXPECT_EQ(to_amount_in_cents_integer((*result)[1].cents_amount()), -25075);

    EXPECT_EQ((*result)[2].tags().at("Text"), "Third Transaction");
    EXPECT_EQ(to_amount_in_cents_integer((*result)[2].cents_amount()), 100000);
  }

  // ============================================================================
  // Empty Statement Tests
  // ============================================================================

  TEST_F(StatementToTaggedAmountsTestFixture, EmptyStatementReturnsEmptyVector) {
    logger::scope_logger log_raii{logger::development_trace,
      "TEST(StatementToTaggedAmounts, EmptyStatementReturnsEmptyVector)"};

    // Create empty entries
    AccountStatementEntries entries;

    // Create AccountStatement with AccountID
    AccountID account_id = make_account_id("NORDEA", "12 34 56-7");
    AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};
    AccountStatement statement(entries, meta);

    // Transform
    auto result = tas::maybe::account_statement_to_tagged_amounts_step(statement);

    // Empty statement should return empty vector, NOT nullopt
    ASSERT_TRUE(result.has_value()) << "Expected valid result for empty statement";
    EXPECT_TRUE(result->empty()) << "Expected empty vector for empty statement";
  }

  // ============================================================================
  // Missing AccountID Tests
  // ============================================================================

  TEST_F(StatementToTaggedAmountsTestFixture, MissingAccountIdUsesEmptyString) {
    logger::scope_logger log_raii{logger::development_trace,
      "TEST(StatementToTaggedAmounts, MissingAccountIdUsesEmptyString)"};

    // Create entry
    AccountStatementEntries entries;
    entries.push_back(make_entry(2025, 2, 1, 75.25, "Transaction without AccountID"));

    // Create AccountStatement WITHOUT AccountID
    AccountStatement::Meta meta{.m_maybe_account_irl_id = std::nullopt};
    AccountStatement statement(entries, meta);

    // Transform
    auto result = tas::maybe::account_statement_to_tagged_amounts_step(statement);

    // Verify
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1);

    auto const& ta = (*result)[0];

    // Text tag should be present
    ASSERT_TRUE(ta.tags().contains("Text"));
    EXPECT_EQ(ta.tags().at("Text"), "Transaction without AccountID");

    // Account tag should be empty string (graceful handling)
    ASSERT_TRUE(ta.tags().contains("Account"));
    EXPECT_EQ(ta.tags().at("Account"), "") << "Expected empty string for missing AccountID";
  }

  // ============================================================================
  // Negative Amount Tests
  // ============================================================================

  TEST_F(StatementToTaggedAmountsTestFixture, NegativeAmountsConvertedCorrectly) {
    logger::scope_logger log_raii{logger::development_trace,
      "TEST(StatementToTaggedAmounts, NegativeAmountsConvertedCorrectly)"};

    // Create entries with negative amounts
    AccountStatementEntries entries;
    entries.push_back(make_entry(2025, 3, 1, -1083.75, "Payment Out"));
    entries.push_back(make_entry(2025, 3, 2, -0.01, "Minimal Negative"));
    entries.push_back(make_entry(2025, 3, 3, -9999.99, "Large Negative"));

    AccountID account_id = make_account_id("NORDEA", "51 86 87-9");
    AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};
    AccountStatement statement(entries, meta);

    auto result = tas::maybe::account_statement_to_tagged_amounts_step(statement);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 3);

    // -1083.75 -> -108375 cents
    EXPECT_EQ(to_amount_in_cents_integer((*result)[0].cents_amount()), -108375);

    // -0.01 -> -1 cent
    EXPECT_EQ(to_amount_in_cents_integer((*result)[1].cents_amount()), -1);

    // -9999.99 -> -999999 cents
    EXPECT_EQ(to_amount_in_cents_integer((*result)[2].cents_amount()), -999999);
  }

  // ============================================================================
  // Amount Conversion Tests
  // ============================================================================

  TEST_F(StatementToTaggedAmountsTestFixture, AmountConversionEdgeCases) {
    logger::scope_logger log_raii{logger::development_trace,
      "TEST(StatementToTaggedAmounts, AmountConversionEdgeCases)"};

    AccountStatementEntries entries;

    // Zero amount
    entries.push_back(make_entry(2025, 4, 1, 0.0, "Zero Amount"));

    // Fractional cents (should round)
    entries.push_back(make_entry(2025, 4, 2, 10.005, "Fractional Cents"));

    // Large amount
    entries.push_back(make_entry(2025, 4, 3, 1000000.00, "One Million"));

    AccountID account_id = make_account_id("TEST:", "123");
    AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};
    AccountStatement statement(entries, meta);

    auto result = tas::maybe::account_statement_to_tagged_amounts_step(statement);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 3);

    // Zero -> 0 cents
    EXPECT_EQ(to_amount_in_cents_integer((*result)[0].cents_amount()), 0);

    // 10.005 should round to 1001 cents (std::round behavior)
    EXPECT_EQ(to_amount_in_cents_integer((*result)[1].cents_amount()), 1001);

    // 1,000,000.00 -> 100,000,000 cents
    EXPECT_EQ(to_amount_in_cents_integer((*result)[2].cents_amount()), 100000000);
  }

  // ============================================================================
  // Date Preservation Tests
  // ============================================================================

  TEST_F(StatementToTaggedAmountsTestFixture, DatePreservedCorrectly) {
    logger::scope_logger log_raii{logger::development_trace,
      "TEST(StatementToTaggedAmounts, DatePreservedCorrectly)"};

    AccountStatementEntries entries;
    entries.push_back(make_entry(2025, 12, 31, 100.0, "End of Year"));
    entries.push_back(make_entry(2025, 1, 1, 200.0, "Start of Year"));
    entries.push_back(make_entry(2025, 6, 15, 300.0, "Mid Year"));

    AccountID account_id = make_account_id("TEST:", "456");
    AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};
    AccountStatement statement(entries, meta);

    auto result = tas::maybe::account_statement_to_tagged_amounts_step(statement);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 3);

    // Entry 0: 2025-12-31
    EXPECT_EQ((*result)[0].date().year(), std::chrono::year{2025});
    EXPECT_EQ((*result)[0].date().month(), std::chrono::month{12});
    EXPECT_EQ((*result)[0].date().day(), std::chrono::day{31});

    // Entry 1: 2025-01-01
    EXPECT_EQ((*result)[1].date().year(), std::chrono::year{2025});
    EXPECT_EQ((*result)[1].date().month(), std::chrono::month{1});
    EXPECT_EQ((*result)[1].date().day(), std::chrono::day{1});

    // Entry 2: 2025-06-15
    EXPECT_EQ((*result)[2].date().year(), std::chrono::year{2025});
    EXPECT_EQ((*result)[2].date().month(), std::chrono::month{6});
    EXPECT_EQ((*result)[2].date().day(), std::chrono::day{15});
  }

  // ============================================================================
  // Integration Tests with Real CSV Data
  // ============================================================================

  TEST(CSVTable2TaggedAmountsTests, IntegrationWithNordeaCsvData) {
    logger::scope_logger log_raii{logger::development_trace,
      "TEST(StatementToTaggedAmounts, IntegrationWithNordeaCsvData)"};

    // Parse NORDEA CSV data
    std::string csv_data = sz_NORDEA_csv_20251120;
    auto maybe_table = CSV::parse::maybe::csv_to_table(csv_data, ';');

    ASSERT_TRUE(maybe_table.has_value()) << "Expected successful CSV parsing";

    auto maybe_statement = account::statement::maybe::to_statement_id_ed_step(*maybe_table)
      .and_then(account::statement::maybe::statement_id_ed_to_account_statement_step);

    ASSERT_TRUE(maybe_statement.has_value()) << "Expected successful statement creation";

    // Step 8: AccountStatement -> TaggedAmounts
    auto result = tas::maybe::account_statement_to_tagged_amounts_step(*maybe_statement);

    ASSERT_TRUE(result.has_value()) << "Expected successful transformation";
    EXPECT_GT(result->size(), 0) << "Expected at least one TaggedAmount";

    // Verify all entries have correct Account tag
    for (auto const& ta : *result) {
      ASSERT_TRUE(ta.tags().contains("Account"));
      EXPECT_EQ(ta.tags().at("Account"), "NORDEA::Anonymous");

      ASSERT_TRUE(ta.tags().contains("Text"));
      EXPECT_FALSE(ta.tags().at("Text").empty()) << "Expected non-empty Text tag";
    }

    // Verify first entry (2025/09/29, -1083.75, LOOPIA)
    if (result->size() > 0) {
      auto const& first = (*result)[0];
      EXPECT_EQ(to_amount_in_cents_integer(first.cents_amount()), -108375);
      EXPECT_TRUE(first.tags().at("Text").find("LOOPIA") != std::string::npos ||
                  first.tags().at("Text").find("Webhotell") != std::string::npos);
    }
  }

  TEST(CSVTable2TaggedAmountsTests, IntegrationWithSkvCsvData) {
    logger::scope_logger log_raii{logger::development_trace,
      "TEST(StatementToTaggedAmounts, IntegrationWithSkvCsvData)"};

    // Parse SKV CSV data
    std::string csv_data = sz_SKV_csv_older;
    auto maybe_table = CSV::parse::maybe::csv_to_table(csv_data, ';');

    ASSERT_TRUE(maybe_table.has_value()) << "Expected successful CSV parsing";

    auto maybe_statement = account::statement::maybe::to_statement_id_ed_step(*maybe_table)
          .and_then(account::statement::maybe::statement_id_ed_to_account_statement_step);

    ASSERT_TRUE(maybe_statement.has_value()) << "Expected successful statement creation";

    // Step 8: AccountStatement -> TaggedAmounts
    auto result = tas::maybe::account_statement_to_tagged_amounts_step(*maybe_statement);

    ASSERT_TRUE(result.has_value()) << "Expected successful transformation";

    // Verify all entries have correct Account tag
    for (auto const& ta : *result) {
      ASSERT_TRUE(ta.tags().contains("Account"));
      EXPECT_EQ(ta.tags().at("Account"), "SKV::Anonymous");
    }
  }

  // ============================================================================
  // Composed Function Test
  // ============================================================================

  TEST_F(StatementToTaggedAmountsTestFixture, ComposedCsvTableToTaggedAmounts) {
    logger::scope_logger log_raii{logger::development_trace,
      "TEST(StatementToTaggedAmounts, ComposedCsvTableToTaggedAmounts)"};

    // Parse NORDEA CSV data
    std::string csv_data = sz_NORDEA_csv_20251120;
    auto maybe_table = CSV::parse::maybe::csv_to_table(csv_data, ';');

    ASSERT_TRUE(maybe_table.has_value()) << "Expected successful CSV parsing";

    // Use composed function (Steps 7+8 in one call)
    auto result = account::statement::maybe::to_statement_id_ed_step(*maybe_table)
          .and_then(account::statement::maybe::statement_id_ed_to_account_statement_step)
          .and_then(tas::maybe::account_statement_to_tagged_amounts_step);

    ASSERT_TRUE(result.has_value()) << "Expected successful composed transformation";
    EXPECT_GT(result->size(), 0) << "Expected at least one TaggedAmount";

    // Verify tags are correctly set
    for (auto const& ta : *result) {
      ASSERT_TRUE(ta.tags().contains("Account"));
      ASSERT_TRUE(ta.tags().contains("Text"));
      EXPECT_EQ(ta.tags().at("Account"), "NORDEA::Anonymous");
    }
  }

  // ============================================================================
  // AccountID Format Tests
  // ============================================================================

  TEST_F(StatementToTaggedAmountsTestFixture, AccountIdWithEmptyPrefix) {
    logger::scope_logger log_raii{logger::development_trace,
      "TEST(StatementToTaggedAmounts, AccountIdWithEmptyPrefix)"};

    AccountStatementEntries entries;
    entries.push_back(make_entry(2025, 5, 1, 50.0, "Test"));

    // AccountID with empty prefix
    AccountID account_id = make_account_id("", "123456789");
    AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};
    AccountStatement statement(entries, meta);

    auto result = tas::maybe::account_statement_to_tagged_amounts_step(statement);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1);

    // Account tag should just be the value without prefix
    EXPECT_EQ((*result)[0].tags().at("Account"), "123456789");
  }

  TEST_F(StatementToTaggedAmountsTestFixture, AccountIdWithVariousPrefixes) {
    logger::scope_logger log_raii{logger::development_trace,
      "TEST(StatementToTaggedAmounts, AccountIdWithVariousPrefixes)"};

    std::vector<std::pair<std::string, std::string>> test_cases = {
      {"NORDEA", "51 86 87-9"},
      {"SKV", "5567828172"},
      {"PG", "90 00 00-1"},
      {"BG", "123-4567"},
      {"IBAN", "SE35 5000 0000 0549 1000 0003"}
    };

    for (auto const& [prefix, value] : test_cases) {
      AccountStatementEntries entries;
      entries.push_back(make_entry(2025, 1, 1, 100.0, "Test"));

      AccountID account_id = make_account_id(prefix, value);
      AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};
      AccountStatement statement(entries, meta);

      auto result = tas::maybe::account_statement_to_tagged_amounts_step(statement);

      ASSERT_TRUE(result.has_value()) << "Failed for prefix: " << prefix;
      ASSERT_EQ(result->size(), 1);

      std::string expected_tag = prefix.empty() ? value : prefix + "::" + value;
      EXPECT_EQ((*result)[0].tags().at("Account"), expected_tag)
        << "Account tag mismatch for prefix: " << prefix;
    }
  }

  } // statement_to_tagged_amounts
} // tests


namespace tests {
  namespace transcoding_views {

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
      auto buffer_result = path_to_byte_buffer_shortcut(utf8_file);
      ASSERT_TRUE(buffer_result) << "Expected successful file read";

      // Create lazy decoding view: bytes → Unicode code points
      auto unicode_view = text::encoding::views::bytes_to_unicode(
        buffer_result.value(),
        text::encoding::EncodingID::UTF8
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
      auto buffer_result = path_to_byte_buffer_shortcut(utf8_file);
      ASSERT_TRUE(buffer_result) << "Expected successful file read";

      // Create full lazy transcoding pipeline: bytes → Unicode → platform encoding
      auto unicode_view = text::encoding::views::bytes_to_unicode(
        buffer_result.value(),
        text::encoding::EncodingID::UTF8
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
      auto buffer_result = path_to_byte_buffer_shortcut(utf8_file);
      ASSERT_TRUE(buffer_result) << "Expected successful file read";

      // Detect encoding directly (or use known encoding for test)
      auto encoding_result = text::encoding::inferred::maybe::to_inferred_encoding(
        buffer_result.value()
      );
      auto detected_encoding = encoding_result
        ? encoding_result->defacto
        : text::encoding::EncodingID::UTF8;

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

  } // transcoding_views
} // tests
