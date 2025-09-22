#ifndef METHODS
#define METHODS

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fs.h> // BLKGETSIZE64
#endif
#include <string>
#include <cstring>
#include <vector>
#include <stdint.h>
#include <cctype>
#include <sstream>

uint64_t getFileOrDeviceSize(const std::string &path)
{
#if defined(_WIN32)
	HANDLE hFile = CreateFileA(
		path.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		0,
		nullptr);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		throw std::runtime_error("Cannot open: " + path);
	}

	LARGE_INTEGER size;
	if (GetFileSizeEx(hFile, &size))
	{
		CloseHandle(hFile);
		return static_cast<uint64_t>(size.QuadPart);
	}

	// Try as block device
	GET_LENGTH_INFORMATION info;
	DWORD bytesReturned;
	if (DeviceIoControl(
			hFile,
			IOCTL_DISK_GET_LENGTH_INFO,
			nullptr,
			0,
			&info,
			sizeof(info),
			&bytesReturned,
			nullptr))
	{
		CloseHandle(hFile);
		return static_cast<uint64_t>(info.Length.QuadPart);
	}

	CloseHandle(hFile);
	throw std::runtime_error("Failed to get size: " + path);

#else // POSIX
	struct stat st;
	if (stat(path.c_str(), &st) == 0)
	{
		if (S_ISREG(st.st_mode))
		{
			// Normal file
			return static_cast<uint64_t>(st.st_size);
		}
	}

	// Try as block device
	int fd = open(path.c_str(), O_RDONLY);
	if (fd < 0)
	{
		throw std::runtime_error("Cannot open: " + path);
	}

	uint64_t size = 0;
	if (ioctl(fd, BLKGETSIZE64, &size) == -1)
	{
		close(fd);
		throw std::runtime_error("BLKGETSIZE64 failed: " + path);
	}

	close(fd);
	return size;
#endif
}

inline bool exists(const std::string &name)
{
#ifdef _WIN32
	WIN32_FILE_ATTRIBUTE_DATA fileInfo;
	if (GetFileAttributesExA(name.c_str(), GetFileExInfoStandard, &fileInfo))
	{
		return true; // File exists
	}
	else
	{
		return false; // File does not exist
	}
#else
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
#endif
}

bool startsWith(const char *str, const char *prefix)
{
	return std::strncmp(str, prefix, std::strlen(prefix)) == 0;
}

std::vector<char *> split_cstr(char *str, char delimiter)
{
	char *token = strtok(str, &delimiter);
	std::vector<char *> tokens;

	while (token != nullptr)
	{
		tokens.push_back(token);
		token = strtok(nullptr, &delimiter);
	}

	return tokens;
}

std::vector<std::string> split(const std::string &str, char delimiter)
{
	std::vector<std::string> parts;
	std::stringstream ss(str);
	std::string item;

	while (std::getline(ss, item, delimiter))
	{
		parts.push_back(item);
	}

	return parts;
}

bool containsLetter(const std::string &s)
{
	for (char c : s)
	{
		if (std::isalpha(static_cast<unsigned char>(c)))
		{
			return true;
		}
	}
	return false;
}

#endif