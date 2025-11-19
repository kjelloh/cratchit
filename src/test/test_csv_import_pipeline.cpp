#include "test_atomics.hpp"
#include "test_fixtures.hpp"
#include "logger/log.hpp"
#include "io/file_reader.hpp"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>

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
} // namespace tests::csv_import_pipeline
