#pragma once
#ifndef imPrint_Test
#define imPrint_Test
#include "imPrint_File.h"
#include <iostream>

namespace imPrint {

	namespace Test {

		void test_file_operations() {
			std::wstring wstr(L"sometime naive");
			DWORD read_size = 0;

			void *read_file = imPrint::File::read_file(L"write_test.txt", malloc, free, &read_size);
			if (read_file != NULL)
				std::cout << "读文件-不存在-失败\n";
			else
				std::cout << "读文件-不存在-成功\n";

			if (!imPrint::File::write_file(L"write_test.txt", reinterpret_cast<const void*>(wstr.data()), wstr.size() * sizeof(wchar_t)))
				std::cout << "写文件-新建-失败\n";
			else
				std::cout << "写文件-新建-成功\n";

			read_file = imPrint::File::read_file(L"write_test.txt", malloc, free, &read_size);
			if (read_file == NULL)
				std::cout << "读文件-存在-失败\n";
			wstr.clear();
			wstr.assign(reinterpret_cast<const wchar_t*>(read_file), read_size / sizeof(wchar_t));
			free(read_file);
			if (wstr != L"sometime naive")
				std::cout << "读文件-存在-失败\n";
			else
				std::cout << "读文件-存在-成功\n";

			wstr = L"too young, too simple, " + wstr;
			if (imPrint::File::write_file(L"write_test.txt", reinterpret_cast<const void*>(wstr.data()), wstr.size() * sizeof(wchar_t), false))
				std::cout << "写文件-不覆盖-失败\n";
			else
				std::cout << "写文件-不覆盖-成功\n";

			if (!imPrint::File::write_file(L"write_test.txt", reinterpret_cast<const void*>(wstr.data()), wstr.size() * sizeof(wchar_t), true))
				std::cout << "写文件-覆盖-失败\n";
			else
				std::cout << "写文件-覆盖-成功\n";

			read_file = imPrint::File::read_file(L"write_test.txt", malloc, free, &read_size);
			if (read_file == NULL)
				std::cout << "读文件-存在-失败\n";
			wstr.clear();
			wstr.assign(reinterpret_cast<const wchar_t*>(read_file), read_size / sizeof(wchar_t));
			free(read_file);
			if (wstr != L"too young, too simple, sometime naive")
				std::cout << "读文件-存在-失败\n";
			else
				std::cout << "读文件-存在-成功\n";

			if (imPrint::File::copy_small_file(L"write_test_fake.txt", L"write_test_copy.txt", malloc, free))
				std::cout << "复制文件-src不存在-覆盖-失败\n";
			else
				std::cout << "复制文件-src不存在-覆盖-成功\n";

			if (!imPrint::File::copy_small_file(L"write_test.txt", L"write_test_copy.txt", malloc, free))
				std::cout << "复制文件-src存在-dst不存在-覆盖-失败\n";
			else
				std::cout << "复制文件-src存在-dst不存在-覆盖-成功\n";

			if (!imPrint::File::copy_small_file(L"write_test.txt", L"write_test_copy.txt", malloc, free))
				std::cout << "复制文件-src存在-dst存在-覆盖-失败\n";
			else
				std::cout << "复制文件-src存在-dst存在-覆盖-成功\n";

			DeleteFile(L"write_test.txt");
			DeleteFile(L"write_test_copy.txt");
		}

	}
	
}

#endif
