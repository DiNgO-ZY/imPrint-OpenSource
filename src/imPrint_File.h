#pragma once
#ifndef imPrint_File
#define imPrint_File
#include <Windows.h>
#include <vector>
#include <memory>
#include <string>
#include <stack>
#include <tuple>
#include <functional>

namespace imPrint {

	template <typename char_t>
	long long strlength(char_t* str) {
		long long length = 0;
		while (*(str++) != 0)
			++length;
		return length;
	}

	bool isFile(DWORD Attributes) {
		if ((Attributes & FILE_ATTRIBUTE_ARCHIVE) != 0)
			return true;
		else
			return false;
	}

	std::wstring mkdirname(const wchar_t* directory_name, const wchar_t* add_directory_name) {
		std::wstring filename(directory_name, strlength(directory_name) - 3);
		filename.reserve(filename.size() + strlength(add_directory_name) + 4);
		filename += add_directory_name;
		filename += L"\\*.*";
		return filename;
	}

	struct FileInfo {
		FileInfo() = default;

		FileInfo(const WIN32_FIND_DATA *find_data) :Attributes(find_data->dwFileAttributes), Name(find_data->cFileName) {}

		FileInfo(const FileInfo&) = default;

		FileInfo(FileInfo&&) = default;

		DWORD Attributes;
		std::wstring Name;
		std::vector<FileInfo> Directory;
	};

	std::vector<FileInfo> directory_info(const wchar_t* directory_name) {
		HANDLE hFind;
		WIN32_FIND_DATA data;
		std::vector<FileInfo> rv;

		hFind = FindFirstFile(directory_name, &data);
		if (hFind != INVALID_HANDLE_VALUE) {
			do
			{
				if (strlength(data.cFileName) == 1)
					if (std::memcmp(L".", data.cFileName, 1) == 0)
						continue;
				if (strlength(data.cFileName) == 2)
					if (std::memcmp(L"..", data.cFileName, 2) == 0)
						continue;

				rv.emplace_back(&data);
			} while (FindNextFile(hFind, &data))
				;
			FindClose(hFind);
		}
		return rv;
	}

	void fill_full_path(std::vector<FileInfo>& data, const wchar_t* directory_name) {
		for (auto &p : data)
			p.Name = mkdirname(directory_name, p.Name.c_str());
	}

	void do_search(FileInfo& root, std::vector<std::wstring>* filenames = NULL, bool should_move = true) {
		using pack_t = std::tuple<FileInfo*, std::vector<FileInfo>::iterator>;
		if (isFile(root.Attributes))
			return;

		std::stack<pack_t> SearchStack;

		auto deeper = [&SearchStack](FileInfo* c, const std::vector<FileInfo>::iterator& it) {
			SearchStack.push(pack_t(c, it));
		};
		auto shallower = [&SearchStack] {
			SearchStack.pop();
		};

		deeper(&root, root.Directory.end());

		while (!SearchStack.empty()) {
			if (std::get<0>(SearchStack.top())->Directory.empty() &&
				!isFile(std::get<0>(SearchStack.top())->Attributes)) {
				std::get<0>(SearchStack.top())->Directory = directory_info(std::get<0>(SearchStack.top())->Name.c_str());
				fill_full_path(std::get<0>(SearchStack.top())->Directory, std::get<0>(SearchStack.top())->Name.c_str());
				std::get<1>(SearchStack.top()) = std::get<0>(SearchStack.top())->Directory.begin();
			}

			if (std::get<1>(SearchStack.top()) == std::get<0>(SearchStack.top())->Directory.end()) {
				shallower();
				continue;
			}

			auto &iter = std::get<1>(SearchStack.top());

			if (!isFile(iter->Attributes))
				deeper(&(*iter), iter->Directory.end());//如果是文件夹，则进去遍历
			else {
				iter->Name.resize(iter->Name.size() - 4);//如果是文件，把Name结尾的通配符删掉，得到正确的文件名
				if (filenames != NULL) {
					if (should_move)
						filenames->push_back(std::move(iter->Name));
					else
						filenames->push_back(iter->Name);
				}
			}

			++iter;
		}
	}

	using HANDLE_uptr = std::unique_ptr<HANDLE, std::function<void(HANDLE*)>>;

	namespace File {

		//获取目录及其下所有子目录结构
		//directory_name指定目录名，必须是xx\xx\xx\*.*的形式
		//返回值是目录结构
		//不应抛出异常
		FileInfo search_directory_info(const wchar_t* directory_name) {
			auto rv = directory_info(directory_name);
			fill_full_path(rv, directory_name);

			for (auto rvit = rv.begin(); rvit != rv.end(); rvit++)
				do_search(*rvit);

			FileInfo return_this;
			return_this.Attributes = FILE_ATTRIBUTE_DIRECTORY;
			return_this.Name = directory_name;
			return_this.Directory = std::move(rv);

			return return_this;
		}

		//获取目录及其下所有子目录中的文件名
		//directory_name指定目录名，必须是xx\xx\xx\*.*的形式
		//返回值是文件名集合
		//不应抛出异常
		std::vector<std::wstring> search_directory_files(const wchar_t* directory_name) {
			std::vector<std::wstring> return_this;
			auto rv = directory_info(directory_name);
			fill_full_path(rv, directory_name);

			for (auto rvit = rv.begin(); rvit != rv.end(); rvit++)
				do_search(*rvit, &return_this);

			return return_this;
		}

		//获取目录及其下所有子目录中的结构和文件名
		//directory_name指定目录名，必须是xx\xx\xx\*.*的形式
		//返回值是目录结构和文件名集合
		//不应抛出异常
		std::tuple<FileInfo, std::vector<std::wstring>> search_directory_info_files(const wchar_t* directory_name) {
			std::vector<std::wstring> return_this;
			auto rv = directory_info(directory_name);
			fill_full_path(rv, directory_name);

			for (auto rvit = rv.begin(); rvit != rv.end(); rvit++)
				do_search(*rvit, &return_this, false);

			FileInfo fileinfo;
			fileinfo.Attributes = FILE_ATTRIBUTE_DIRECTORY;
			fileinfo.Name = directory_name;
			fileinfo.Directory = std::move(rv);

			return std::tuple<FileInfo, std::vector<std::wstring>>(std::move(fileinfo), std::move(return_this));
		}

		//读取文件
		//src是要读取的文件名，alloc_func是用于分配的函数，alloc_size是分配内存的大小（这个参数可以不填如果你不需要的话），dealloc_func是用于回收的函数（将在抛出异常前被调用），should_throw_if_count_mismatch指示如果读取字节与预期不符是否应该抛出异常
		//返回值是读取的文件内容（如果函数失败则返回NULL）
		//可能抛出异常，如果读取的字节数与预期不同
		void* read_file(const wchar_t* src, const std::function<void*(std::size_t)>& alloc_func, const std::function<void(void*)>& dealloc_func, DWORD *alloc_size = NULL, bool should_throw_if_count_mismatch = true) {
			HANDLE hFile = CreateFile(src, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			HANDLE_uptr hFileRAII(&hFile, [](HANDLE* hd) {
				CloseHandle(*hd);
			});

			if (hFile == INVALID_HANDLE_VALUE)
				return NULL;

			DWORD fileSize = GetFileSize(hFile, NULL), readSize;
			void *buffer = alloc_func(fileSize);
			if (alloc_size != NULL)
				*alloc_size = fileSize;

			if (ReadFile(hFile, buffer, fileSize, &readSize, NULL) != TRUE)
				return NULL;

			if (readSize != fileSize && should_throw_if_count_mismatch) {
				dealloc_func(buffer);
				throw std::runtime_error("imPrint::File::read_file ---- readSize != fileSize");
			}

			return buffer;
		}

		//写入文件
		//dst是要写入的文件名，src是要写入的内容，length指示src的长度，should_throw_if_count_mismatch指示如果写入字节与预期不符是否应该抛出异常
		//返回值指示是否成功
		//不应抛出异常
		bool write_file(const wchar_t* dst, const void* src, DWORD length, bool should_overwrite = true, bool should_throw_if_count_mismatch = true) {
			HANDLE hFile = INVALID_HANDLE_VALUE;
			if (should_overwrite)
				hFile = CreateFile(dst, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			else
				hFile = CreateFile(dst, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

			HANDLE_uptr hFileRAII(&hFile, [](HANDLE* hd) {
				CloseHandle(*hd);
			});

			if (hFile == INVALID_HANDLE_VALUE)
				return false;

			DWORD writtenBytesCount = 0;
			if (WriteFile(hFile, src, length, &writtenBytesCount, NULL) != TRUE)
				return false;

			if (writtenBytesCount != length && should_throw_if_count_mismatch)
				throw std::runtime_error("imPrint::File::write_file ---- writtenSize != sourceSize");

			return true;
		}

		//复制小文件，将文件全部读取到内存中，然后再写入到新的文件
		//src是要复制的文件，dst是新文件，should_overwrite指示当新文件存在时是否应该覆盖，alloc_func是用于分配的函数，dealloc_func是用于回收的函数
		//返回值是操作是否成功
		//不应抛出异常
		bool copy_small_file(const wchar_t* src, const wchar_t* dst, const std::function<void*(std::size_t)>& alloc_func, const std::function<void(void*)>& dealloc_func, bool should_overwrite = true, bool should_throw_if_count_mismatch = true) {
			DWORD file_size;
			std::unique_ptr<void, std::function<void(void*)>> file_content(read_file(src, alloc_func, dealloc_func, &file_size, should_throw_if_count_mismatch), [&dealloc_func](void* ptr) {
				dealloc_func(ptr);
			});

			if (file_content == NULL)
				return false;

			return write_file(dst, file_content.get(), file_size, should_overwrite, should_throw_if_count_mismatch);
		}

		//复制大文件，将文件部分轮流读取到内存中，然后写入到新文件
		//
		//返回值是操作是否成功
		//不应抛出异常
		bool copy_large_file(const wchar_t* src, const wchar_t* dst, std::size_t block_size) {
			return false;
		}

		//复制文件，自动判断应该调用copy_large_file还是copy_small_file
		//
		//返回值是操作是否成功
		//不应抛出异常
		bool copy_file(const wchar_t* src, const wchar_t* dst, std::size_t limit_size, std::size_t block_size) {
			return false;
		}

		//获得小文件hash
		//
		//返回值指示操作是否成功
		//不应抛出异常
		bool hash_small_file() {
			return false;
		}

		//获得大文件hash
		//
		//返回值指示操作是否成功
		//不应抛出异常
		bool hash_large_file() {
			return false;
		}

		//获得文件hash，自动判断应该调用hash_large_file还是hash_small_file
		//
		//返回值指示操作是否成功
		//不应抛出异常
		bool hash_file() {
			return false;
		}

	}
}

#endif
