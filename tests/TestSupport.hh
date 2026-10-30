#pragma once
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <kcore/io/FileSystem.hh>
namespace kc::test {
class TemporaryDirectory : public testing::Test {
   protected:
    void SetUp() override {
        auto pattern =
            (std::filesystem::temp_directory_path() / "kcore-test-XXXXXX")
                .string();
        auto* created = mkdtemp(pattern.data());
        ASSERT_NE(created, nullptr);
        root = created;
    }
    void TearDown() override {
        std::error_code error;
        std::filesystem::remove_all(root.str(), error);
        EXPECT_FALSE(error) << error.message();
    }
    Path path(const Str& name) const { return Path::join(root, name); }
    Path root;
    FileSystem fs;
};

class ScopedEnv {
   public:
    ScopedEnv(const Str& name, Opt<Str> value = {}) : name(name) {
        if (const auto* previous = std::getenv(name.c_str())) {
            old = previous;
        }
        if (value) {
            EXPECT_EQ(setenv(name.c_str(), value->c_str(), 1), 0);
        } else {
            EXPECT_EQ(unsetenv(name.c_str()), 0);
        }
    }
    ScopedEnv(const ScopedEnv&) = delete;
    ScopedEnv& operator=(const ScopedEnv&) = delete;
    ~ScopedEnv() {
        if (old) {
            EXPECT_EQ(setenv(name.c_str(), old->c_str(), 1), 0);
        } else {
            EXPECT_EQ(unsetenv(name.c_str()), 0);
        }
    }

   private:
    Str name;
    Opt<Str> old;
};

class MockFile : public File {
   public:
    MOCK_METHOD(Result<size_t>, read, (std::span<Byte>), (override));
    MOCK_METHOD(Result<void>, write, (std::span<const Byte>), (override));
    MOCK_METHOD(Result<void>, close, (), (override));
};

class MockFileSystem : public FileSystem {
   public:
    MOCK_METHOD(bool, exists, (const Path&), (const, override));
    MOCK_METHOD(bool, isFile, (const Path&), (const, override));
    MOCK_METHOD(Result<FileInfo>, info, (const Path&), (const, override));
    MOCK_METHOD(
        Result<bool>, equivalent, (const Path&, const Path&), (const, override)
    );
    MOCK_METHOD(
        Result<std::unique_ptr<File>>, openFile, (const Path&, FileMode),
        (const, override)
    );
    MOCK_METHOD(
        Result<std::vector<Path>>, listEntries, (const Path&), (const, override)
    );
    MOCK_METHOD(std::vector<Path>, glob, (const Str&), (const, override));
    MOCK_METHOD(Result<Str>, readFile, (const Path&), (const, override));
    MOCK_METHOD(bool, isDirectory, (const Path&), (const, override));
    MOCK_METHOD(
        Result<std::vector<Path>>, listFiles, (const Path&), (const, override)
    );
    MOCK_METHOD(
        Result<void>, createDirectory, (const Path&), (const, override)
    );
    MOCK_METHOD(
        Result<void>, copyRecursive, (const Path&, const Path&),
        (const, override)
    );
    MOCK_METHOD(
        Result<void>, copyFile, (const Path&, const Path&), (const, override)
    );
    MOCK_METHOD(
        Result<void>, copyDirectoryRecursive, (const Path&, const Path&),
        (const, override)
    );
    MOCK_METHOD(Result<void>, removeAll, (const Path&), (const, override));
};

}  // namespace kc::test
