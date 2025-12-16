#include "gd-file-picker.h"
#include "proxy.hpp"
#include "winepath.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace {

using Path = std::filesystem::path;
using PickMode = geode::utils::file::PickMode;
using FilePickOptions = geode::utils::file::FilePickOptions;


template <class T>
void resolveOk(auto& resolve, T&& value) {
    resolve(geode::Result<std::decay_t<T>, std::string>(geode::Ok(std::forward<T>(value))));
}

void resolveErr(auto& resolve, const std::exception& ex) {
    resolve(geode::Result<void, std::string>(geode::Err(ex.what())));
}

Path unixToPath(const std::string& unixPath) {
    return std::filesystem::path{ std::u8string(unixPath.begin(), unixPath.end()) };
}

}

bool linuxOpenFolder(const Path& path) {
    const std::string unixPath = dosToUnixPath(path.string());
    g_filePicker->openLocation({ unixPath });
    return true;
}

geode::Task<geode::Result<Path>> linuxFilePick(
    PickMode mode, const FilePickOptions& options
) {
    return geode::Task<geode::Result<Path>>::runWithCallback(
        [mode, options](auto resolve, auto /*progress*/, auto /*cancelled*/) {
            try {
                if (mode == PickMode::SaveFile) {
                    SaveFileParams params{};
                    if (options.defaultPath) {
                        params.filename = options.defaultPath->filename().string();
                    }

                    const auto result = g_filePicker->saveFile(params).at(0);
                    Path filePath = unixToPath(result);
                    log::info("Picked {}", filePath.string());
                    resolveOk(resolve, filePath);
                    return;
                }

                if (mode == PickMode::OpenFile) {
                    OpenFileParams params{};
                    const auto result = g_filePicker->openFile(params).at(0);
                    Path filePath = unixToPath(result);
                    log::info("Picked {}", filePath.string());
                    resolveOk(resolve, filePath);
                    return;
                }
            } catch (const std::exception& ex) {
                resolve(geode::Result<Path, std::string>(geode::Err(ex.what())));
            }
        }
    );
}

geode::Task<geode::Result<std::vector<Path>>> linuxPickMany(
    const FilePickOptions&
) {
    return geode::Task<geode::Result<std::vector<Path>>>::runWithCallback(
        [](auto resolve, auto /*progress*/, auto /*cancelled*/) {
            try {
                OpenFileParams params{};
                params.multiple = true;

                const auto results = g_filePicker->openFile(params);
                std::vector<Path> paths;
                paths.reserve(results.size());

                for (const auto& unixPath : results) {
                    Path filePath = unixToPath(unixPath);
                    paths.emplace_back(filePath);
                }

                log::info("Multi-Picker result: {}", paths);
                resolve(geode::Result<std::vector<Path>, std::string>(geode::Ok(paths)));
                
            } catch (const std::exception& ex) {
                log::info("Multi-Picker error {}", ex.what());
                resolve(geode::Result<std::vector<Path>, std::string>(geode::Err(ex.what())));
            }
        }
    );
}

$execute {
    using namespace geode;
    using namespace utils;

    Mod::get()->hook(
        reinterpret_cast<void*>(
            addresser::getNonVirtual(
                modifier::Resolve<const std::filesystem::path&>::func(
                    &file::openFolder
                )
            )
        ),
        &linuxOpenFolder,
        "utils::file::openFolder",
        tulip::hook::TulipConvention::Stdcall
    ).unwrap();

    Mod::get()->hook(
        reinterpret_cast<void*>(
            addresser::getNonVirtual(
                modifier::Resolve<
                    file::PickMode,
                    const file::FilePickOptions&
                >::func(&file::pick)
            )
        ),
        &linuxFilePick,
        "utils::file::pick",
        tulip::hook::TulipConvention::Stdcall
    ).unwrap();

    Mod::get()->hook(
        reinterpret_cast<void*>(
            addresser::getNonVirtual(
                modifier::Resolve<
                    const file::FilePickOptions&
                >::func(&file::pickMany)
            )
        ),
        &linuxPickMany,
        "utils::file::pickMany",
        tulip::hook::TulipConvention::Stdcall
    ).unwrap();
}
