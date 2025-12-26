/* ============================================================================
MIT License

Copyright (c) 2025 jhlayt

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

============================================================================ */

#include <stdint.h>
#include <windows.h>
#include <stdio.h>

#define TRUE 1
#define FALSE 0
#define internal static
#define CACHE_FILE_NAME "compile_timer_cache"

#define PATH_BUF_SIZE 128

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;
typedef u8 bool8;
typedef double f64;

internal inline void
win32_log_last_err()
{
  char buf[256];
  DWORD err_msg = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
      NULL,
      GetLastError(),
      0,
      buf,
      255,
      NULL);
  fprintf(stdout, "%s\n", buf);
}

typedef struct cache_file_info cache_file_info;
struct cache_file_info
{
  f64 _stamp;
};

internal inline u32
str_len(const char *a)
{
  u32 i = 0;
  for (;;)
  {
    if (a[i++] == '\0')
      return (i-1);
  }
  return (0);
}

internal inline bool8 
str_cmp(const char *a, const char *b)
{
  u32 i = 0;
  for (;;)
  {
    char v0 = a[i];
    char v1 = b[i];
    if (v0 + v1 != 0)
    {
      if (v0 == v1)
        i++;
      else
        break;
    }
    else if (v0 + v1 == 0)
    {
      return (TRUE);
    }
  }
  return (FALSE);
}

void 
get_full_file_path(const char *dir, char *buf)
{
  u32 index = 0, dir_len, file_len;
  for (;;)
  {
    if (dir[index] == '\0')
    {
      dir_len = index;
      break;
    }
    index++;
  }
  memcpy(buf, dir, dir_len);
  if (dir[index-1] != '\\')
  {
    buf[dir_len++] = '\\';
  }

  index = 0;
  const char *file_name = CACHE_FILE_NAME;
  for (;;)
  {
    if (file_name[index] == '\0')
    {
      file_len = index;
      break;
    }
    index++;
  }
  memcpy(buf + dir_len, file_name, file_len);
}

bool8
read_cache_file_info(const char *file_path, cache_file_info *out_info)
{
  HANDLE file_handle = CreateFileA(
      file_path,
      GENERIC_READ,
      0,
      NULL,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL,
      NULL
      );
  if (file_handle != INVALID_HANDLE_VALUE)
  {
    u64 bytes_read = 0;
    DWORD file_size = GetFileSize(file_handle, NULL);
    if (file_size == INVALID_FILE_SIZE)
    {
      win32_log_last_err();
      CloseHandle(file_handle);
      return (FALSE);
    }
    if (!ReadFile(file_handle, out_info, file_size,
        (LPDWORD)(&bytes_read), NULL))
    {
      win32_log_last_err();
      CloseHandle(file_handle);
      return (FALSE);
    }
    CloseHandle(file_handle);
    return (TRUE);
  }
  else
  {
    win32_log_last_err();
    fprintf(stdout, "The directory doesn't contain a cache file -\
 Make sure you run the program with 'start' before using 'stop', and that the\
 directory exists; this program will not create it.\n");
    CloseHandle(file_handle);
    return (FALSE);
  }
  return (FALSE);
}

bool8
write_cache_file_info(const char *file_path, cache_file_info cfi)
{
  HANDLE file_handle = CreateFileA(
      file_path,
      GENERIC_WRITE,
      0,
      NULL,
      CREATE_ALWAYS,
      FILE_ATTRIBUTE_NORMAL,
      NULL
      );
  if (file_handle != INVALID_HANDLE_VALUE)
  {
    u32 written_bytes = 0;
    WriteFile(file_handle, (void*)(&cfi), sizeof(cache_file_info),
        (LPDWORD)(&written_bytes), NULL);
    CloseHandle(file_handle);
    fprintf(stdout, "Cache file written: %s\n", file_path);
    return (TRUE);
  }
  else
  {
    win32_log_last_err();
  }
  CloseHandle(file_handle);
  return (FALSE);
}

int
main (int argc, const char **argv)
{
  if (argc == 3)
  {
    LARGE_INTEGER freq = {0}, stamp = {0};
    timeBeginPeriod(1);
    QueryPerformanceFrequency(&freq);

    const char *mode = argv[1];
    const char *file_dir = argv[2];

    if (str_len(file_dir) + str_len(CACHE_FILE_NAME) >= PATH_BUF_SIZE-1)
    {
      fprintf(stdout, "Directory path provided is too long, only supporting\
 128 - CACHE_FILE_NAME length characters.\n");
      return (0);
    }
    char full_file_path[PATH_BUF_SIZE];
    memset(full_file_path, 0, PATH_BUF_SIZE);
    get_full_file_path(file_dir, full_file_path);

    if (str_cmp(mode,  "start"))
    {
      QueryPerformanceCounter(&stamp);
      f64 d = stamp.QuadPart / (f64)freq.QuadPart;
      cache_file_info cfi = {0};
      cfi._stamp = d;
      write_cache_file_info(full_file_path, cfi);
    }
    else if (str_cmp(mode,  "stop"))
    {
      cache_file_info cfi = {0};
      if (read_cache_file_info(full_file_path, &cfi))
      {
        QueryPerformanceCounter(&stamp);
        f64 result_ms = (stamp.QuadPart / (f64)freq.QuadPart) - cfi._stamp;
        fprintf(stdout, "%.4fs\n", result_ms);
      }
      else
      {
        return (0);
      }
    }
    else
    {
      fprintf(stdout, "Mode is invalid.\n");
    }
  }
  else
  {
    fprintf(stdout, "Args expected: [mode (start/stop)] [dir_for_cache_file]\n");
  }
  return (0);
}
