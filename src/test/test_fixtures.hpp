#pragma once

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>

namespace tests::fixtures {
    
    // Global test environment to communicate settings across all tests
    class TestEnvironment : public ::testing::Environment {
    public:
        static TestEnvironment* GetInstance();
        void SetKeepTestFiles(bool keep) { keep_test_files_ = keep; }
        bool ShouldKeepTestFiles() const { return keep_test_files_; }
        
    private:
        bool keep_test_files_ = false;
    };
    
    // Base fixture for meta-transform tests that need meh library support
    class MetaTransformFixture : public ::testing::Test {
    protected:
        void SetUp() override;
        void TearDown() override;
        
        // Copy meh library files to the specified build directory
        void copy_meh_library_to(const std::filesystem::path& build_dir);
        
        // Get the path to the meh source directory
        std::filesystem::path get_meh_source_path() const;
        
        std::filesystem::path test_base_dir;  // cpptha_test directory
        std::filesystem::path temp_dir;      // test-specific subdirectory
        
    private:
        void copy_meh_library();
    };
    
    // Utility function to copy meh library to any meta_transform build directory
    void copy_meh_library_to_build_dir(const std::filesystem::path& build_dir);
    
}