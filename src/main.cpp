#include "gd-file-picker.h"
#include "proxy.hpp"
#include "winepath.hpp"
#include <codecvt>
#include <locale>

static std::filesystem::path pathFromUtf8(const std::string& utf8Path) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wide = converter.from_bytes(utf8Path);
    return std::filesystem::path(wide);
}

bool linuxOpenFolder(std::filesystem::path const& path){
    std::string unixPath = dosToUnixPath(path.string());

    g_filePicker->openLocation({unixPath});

    return true;
}

geode::Task<geode::Result<std::filesystem::path>> linuxFilePick(
    geode::utils::file::PickMode mode, const geode::utils::file::FilePickOptions &options
) {
    return geode::Task<geode::Result<std::filesystem::path>>::runWithCallback(
        [mode, options] (auto resolve, auto progress, auto cancelled) {
            if (mode == geode::utils::file::PickMode::SaveFile) {
                SaveFileParams par;
                par.filename = options.defaultPath.has_value() ? 
                    options.defaultPath->filename().string() : "";

                try {
                    auto result = g_filePicker->saveFile(par)[0];
                    std::string unixPath = result;
                    std::string dosPath = unixToDosPath(unixPath);

                    // Используем u8path для корректной обработки UTF-8
                    std::filesystem::path filePath = std::filesystem::u8path(dosPath);

                    resolve(geode::Result<std::filesystem::path, std::string>(geode::Ok(filePath)));
                } catch (const std::exception& ex) {
                    resolve(geode::Result<std::filesystem::path, std::string>(geode::Err(ex.what())));
                }
            }
            else if (mode == geode::utils::file::PickMode::OpenFile) {
                OpenFileParams par;

                try {
                    auto result = g_filePicker->openFile(par)[0];
                    std::string unixPath = result;
                    std::string dosPath = unixToDosPath(unixPath);

                    std::filesystem::path filePath = std::filesystem::u8path(dosPath);

                    resolve(geode::Result<std::filesystem::path, std::string>(geode::Ok(filePath)));
                } catch (const std::exception& ex) {
                    resolve(geode::Result<std::filesystem::path, std::string>(geode::Err(ex.what())));
                }
            }
        }
    );
}

geode::Task<geode::Result<std::vector<std::filesystem::path>>> linuxPickMany(
    const geode::utils::file::FilePickOptions &options
) {
    return geode::Task<geode::Result<std::vector<std::filesystem::path>>>::runWithCallback(
        [] (auto resolve, auto progress, auto cancelled) 
        {
            OpenFileParams par;
            par.multiple = true;

            try {
                auto result = g_filePicker->openFile(par);

                std::vector<std::filesystem::path> paths = {};

                for (const auto& unixPath : result) {
                    std::string dosPath = unixToDosPath(unixPath);
                    std::filesystem::path filePath = std::filesystem::u8path(dosPath);

                    paths.push_back(filePath);

                    log::info("Picked {}", filePath.string());
                }

                resolve(geode::Result<std::vector<std::filesystem::path>, std::string>(geode::Ok(paths)));

            } catch (const std::exception& ex) {
                resolve(geode::Result<std::vector<std::filesystem::path>, std::string>(geode::Err(ex.what())));
            }
        }
    );
}

$execute {
    Mod::get()->hook(
		reinterpret_cast<void*>(
			geode::addresser::getNonVirtual(
				geode::modifier::Resolve<std::filesystem::path const&>::func(
					&utils::file::openFolder
				)
			)
		),
		&linuxOpenFolder,
		"utils::file::openFolder",
		tulip::hook::TulipConvention::Stdcall		
	).unwrap();

    Mod::get()->hook(
		reinterpret_cast<void*>(
			geode::addresser::getNonVirtual(
				geode::modifier::Resolve<
					file::PickMode, 
					const file::FilePickOptions &
				>::func(&utils::file::pick)
			)
		),
		&linuxFilePick,
		"utils::file::pick",
		tulip::hook::TulipConvention::Stdcall		
	).unwrap();

    Mod::get()->hook(
		reinterpret_cast<void*>(
			geode::addresser::getNonVirtual(
				geode::modifier::Resolve<
					const file::FilePickOptions &
				>::func(&utils::file::pickMany)
			)
		),
		&linuxPickMany,
		"utils::file::pickMany",
		tulip::hook::TulipConvention::Stdcall		
	).unwrap();
}