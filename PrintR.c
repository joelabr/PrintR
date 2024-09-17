/*
MIT License

Copyright (c) 2024 Joel Abrahamsson

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
*/
#define UNICODE
#define _UNICODE
#define __STDC_WANT_LIB_EXT1__ 1
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <windows.h>

#define NAME_LENGTH 51
#define BUFFER_SIZE 100000

typedef enum PrintError {
  OK = 0,
  OPEN_PRINTER_ERROR,
  START_PRINT_JOB_ERROR,
  START_PRINT_PAGE_ERROR,
  WRITE_DATA_ERROR,
  PARTIAL_WRITE_ERROR,
  FILE_READ_ERROR
} PrintError;

PrintError print_file(wchar_t* printer_name, FILE* file) {
  char buffer[BUFFER_SIZE];
  HANDLE printer_handle = NULL;
  DOC_INFO_1 document_info = {
    .pDocName = L"PrintR RAW Print",
    .pOutputFile = NULL,
    .pDatatype = L"RAW"
  };

  PrintError status = OK;

  if (!OpenPrinter((LPWSTR)printer_name, &printer_handle, 0)) {
    status = OPEN_PRINTER_ERROR;
    goto err_open_printer;
  }

  if (!StartDocPrinter(printer_handle, (DWORD)1, (LPBYTE)&document_info)) {
    status = START_PRINT_JOB_ERROR;
    goto err_start_print_job;
  }

  if (!StartPagePrinter(printer_handle)) {
    status = START_PRINT_PAGE_ERROR;
    goto err_start_page_error;
  }

  size_t bytes_read = fread(buffer, sizeof buffer[0], BUFFER_SIZE, file);
  DWORD bytes_written = 0L;
  while (!ferror(file) && bytes_read > 0) {
    if (!WritePrinter(printer_handle, (LPBYTE)buffer, (DWORD)bytes_read, &bytes_written)) {
      status = WRITE_DATA_ERROR;
      goto err_write_data;
    }

    if ((size_t)bytes_written != bytes_read) {
      status = PARTIAL_WRITE_ERROR;
      goto err_write_data;
    }

    if (!feof(file)) {
       bytes_read = fread(buffer, sizeof buffer[0], BUFFER_SIZE, file);
    } else {
      break;
    }
  }

  if (ferror(file)) {
    status = FILE_READ_ERROR;
  }

  err_write_data:
  EndPagePrinter(printer_handle);

  err_start_page_error:
  EndDocPrinter(printer_handle);

  err_start_print_job:
  ClosePrinter(printer_handle);

  err_open_printer:
  
  return status;
}

void show_usage(void) {
  puts("Usage: printr PRINTER [FILES]\n");
  puts("Print RAW data to the given printer. The data is supplied via standard input or files.\n");
  puts("PRINTER\tThe printer to send the print job to.");
  puts("FILES\tOptional argument. Prints the given files. If omitted the input will be read from Standard Input.\n");
}

int main(int argc, char* argv[]) {
  wchar_t printer_name[NAME_LENGTH] = {0};

  PrintError result = OK;

  if (argc > 1) {
    FILE* file = stdin;
    size_t number_of_files = argc - 2;

    size_t printer_name_length = strlen(argv[1]);
    printer_name_length = printer_name_length < NAME_LENGTH - 1 ? printer_name_length : NAME_LENGTH - 1;

    mbstowcs(printer_name, argv[1], printer_name_length);
    printer_name[NAME_LENGTH-1] = 0;

    if (number_of_files > 0) {
      for (size_t i = 0; i < number_of_files && result == OK; i++) {
        char* file_name = argv[2 + i];
        file = fopen(file_name, "r");

        if (!file) {
          fprintf(stderr, "Failed to open file: %s!\n", file_name);
          exit(EXIT_FAILURE);
        }

        result = print_file(printer_name, file);
        fclose(file);
      }
    } else {
      result = print_file(printer_name, file);
    }
  } else {
    show_usage();
  }

  switch (result) {
    case OPEN_PRINTER_ERROR:
      fprintf(stderr, "Failed to open printer!");
      break;
    case START_PRINT_JOB_ERROR:
      fprintf(stderr, "Failed to start print job!");
      break;
    case START_PRINT_PAGE_ERROR:
      fprintf(stderr, "Failed to start printing to page!");
      break;
    case WRITE_DATA_ERROR:
      fprintf(stderr, "Failed to write page data!");
      break;
    case PARTIAL_WRITE_ERROR:
      fprintf(stderr, "Failed to write all data to page!");
      break;
    case FILE_READ_ERROR:
      fprintf(stderr, "Failed to read file!");
      break;
    default:
      break;
  }

  return result == OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
