#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <functional>
#include <optional>
#include <vector>
#include <regex>
#include <algorithm>
#include <spdlog/spdlog.h>

struct PruningPolicy {
    std::optional<size_t> max_count = 10;           // Keep max 10 backups
    std::optional<std::chrono::hours> max_age = std::chrono::hours{24 * 7}; // Keep max 7 days
    
    // Factory methods
    static PruningPolicy count_only(size_t count) {
        return {count, std::nullopt};
    }
    
    static PruningPolicy age_only(std::chrono::hours age) {
        return {std::nullopt, age};
    }
    
    static PruningPolicy hybrid(size_t count, std::chrono::hours age) {
        return {count, age};
    }
    
    static PruningPolicy no_pruning() {
        return {std::nullopt, std::nullopt};
    }
};

template <typename E>
class PersistentFile {
public:
    using LoadFunc = std::function<E(const std::filesystem::path&)>;
    using SaveFunc = std::function<void(const E&, const std::filesystem::path&)>;

    PersistentFile(std::filesystem::path path, LoadFunc loader, SaveFunc saver, PruningPolicy pruning_policy = {})
        : path_(std::move(path)), load_func_(std::move(loader)), save_func_(std::move(saver)), pruning_policy_(pruning_policy) {}

    ~PersistentFile() {
        try {
            prune_backups();
        } catch (const std::exception& e) {
            spdlog::error("PersistentFile destructor: Failed to prune backups for {}: {}", path_.string(), e.what());
        } catch (...) {
            spdlog::error("PersistentFile destructor: Unknown error pruning backups for {}", path_.string());
        }
    }

    void init() {
        if (!cached_ && std::filesystem::exists(path_)) {
            cached_ = load_func_(path_);
        }
    }

    void update(const E& current) {
        if (!cached_ || *cached_ != current) {
            if (std::filesystem::exists(path_)) {
                std::filesystem::rename(path_, make_backup_filename(path_));
            }
            save_func_(current, path_);
            cached_ = current; // Store the new value in the cache
        }
    }

    const std::optional<E>& cached() const {
        return cached_;
    }

    std::filesystem::path path() const {
        return path_;
    }

    std::filesystem::path root_path() const {
        return path_.parent_path();
    }

private:
    std::filesystem::path path_;
    LoadFunc load_func_;
    SaveFunc save_func_;
    std::optional<E> cached_;
    PruningPolicy pruning_policy_;

    static std::filesystem::path make_backup_filename(const std::filesystem::path& original) {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);

        std::ostringstream ss;
        ss << std::put_time(std::localtime(&t), "_%Y-%m-%d_%H-%M-%S");

        return original.parent_path() /
               (original.stem().string() + ss.str() + original.extension().string());
    }

    struct BackupFileInfo {
        std::filesystem::path path;
        std::chrono::system_clock::time_point timestamp;
        std::filesystem::file_time_type file_time;
    };

    std::vector<BackupFileInfo> find_backup_files() const {
        std::vector<BackupFileInfo> backups;
        
        if (!std::filesystem::exists(path_.parent_path())) {
            return backups;
        }

        // Create regex pattern for backup files
        // Example: cratchit_2025-07-20_15-24-29.env
        std::string base_name = path_.stem().string();
        std::string extension = path_.extension().string();
        std::string pattern = base_name + R"(_(\d{4})-(\d{2})-(\d{2})_(\d{2})-(\d{2})-(\d{2}))" + 
                             (extension.empty() ? "" : "\\" + extension) + "$";
        std::regex backup_regex(pattern);

        for (const auto& entry : std::filesystem::directory_iterator(path_.parent_path())) {
            if (!entry.is_regular_file()) continue;
            
            std::string filename = entry.path().filename().string();
            std::smatch matches;
            
            if (std::regex_search(filename, matches, backup_regex) && matches.size() == 7) {
                try {
                    // Parse timestamp from filename
                    int year = std::stoi(matches[1]);
                    int month = std::stoi(matches[2]);
                    int day = std::stoi(matches[3]);
                    int hour = std::stoi(matches[4]);
                    int minute = std::stoi(matches[5]);
                    int second = std::stoi(matches[6]);
                    
                    std::tm tm_time = {};
                    tm_time.tm_year = year - 1900;
                    tm_time.tm_mon = month - 1;
                    tm_time.tm_mday = day;
                    tm_time.tm_hour = hour;
                    tm_time.tm_min = minute;
                    tm_time.tm_sec = second;
                    
                    auto timestamp = std::chrono::system_clock::from_time_t(std::mktime(&tm_time));
                    auto file_time = entry.last_write_time();
                    
                    backups.push_back({entry.path(), timestamp, file_time});
                } catch (const std::exception&) {
                    // Skip malformed backup files
                }
            }
        }
        
        // Sort by timestamp (newest first)
        std::sort(backups.begin(), backups.end(), 
                  [](const BackupFileInfo& a, const BackupFileInfo& b) {
                      return a.timestamp > b.timestamp;
                  });
        
        return backups;
    }

    void prune_backups() {
        if (!pruning_policy_.max_count && !pruning_policy_.max_age) {
            return; // No pruning configured
        }

        auto backups = find_backup_files();
        auto now = std::chrono::system_clock::now();
        std::vector<std::filesystem::path> files_to_delete;

        // Apply age-based pruning first
        if (pruning_policy_.max_age) {
            auto cutoff_time = now - *pruning_policy_.max_age;
            for (const auto& backup : backups) {
                if (backup.timestamp < cutoff_time) {
                    files_to_delete.push_back(backup.path);
                }
            }
        }

        // Apply count-based pruning
        if (pruning_policy_.max_count && backups.size() > *pruning_policy_.max_count) {
            // Keep only the newest max_count files
            for (size_t i = *pruning_policy_.max_count; i < backups.size(); ++i) {
                // Only add if not already marked for age-based deletion
                if (std::find(files_to_delete.begin(), files_to_delete.end(), backups[i].path) == files_to_delete.end()) {
                    files_to_delete.push_back(backups[i].path);
                }
            }
        }

        // Delete the files
        if (!files_to_delete.empty()) {
            spdlog::info("PersistentFile: Pruning {} backup files for {}", files_to_delete.size(), path_.string());
            for (const auto& file_path : files_to_delete) {
                try {
                    std::filesystem::remove(file_path);
                    spdlog::info("PersistentFile: Pruned backup file: {}", file_path.string());
                } catch (const std::exception& e) {
                    spdlog::error("PersistentFile: Failed to delete backup file {}: {}", file_path.string(), e.what());
                    // Continue with other files - don't fail the whole operation
                }
            }
        }
    }
};
