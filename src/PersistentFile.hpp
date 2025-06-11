#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <functional>
#include <optional>

template <typename E>
class PersistentFile {
public:
    using LoadFunc = std::function<E(const std::filesystem::path&)>;
    using SaveFunc = std::function<void(const E&, const std::filesystem::path&)>;

    PersistentFile(std::filesystem::path path, LoadFunc loader, SaveFunc saver)
        : path_(std::move(path)), load_func_(std::move(loader)), save_func_(std::move(saver)) {}

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

private:
    std::filesystem::path path_;
    LoadFunc load_func_;
    SaveFunc save_func_;
    std::optional<E> cached_;

    static std::filesystem::path make_backup_filename(const std::filesystem::path& original) {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);

        std::ostringstream ss;
        ss << std::put_time(std::localtime(&t), "_%Y-%m-%d_%H-%M-%S");

        return original.parent_path() /
               (original.stem().string() + ss.str() + original.extension().string());
    }
};
