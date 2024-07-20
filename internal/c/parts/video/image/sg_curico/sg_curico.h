//-----------------------------------------------------------------------------------------------------
// Windows Cursor & Icon Loader for QB64-PE by a740g
//
// Bibliography:
// https://en.wikipedia.org/wiki/ICO_(file_format)
// https://learn.microsoft.com/en-us/previous-versions/ms997538(v=msdn.10)
// https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader
// https://devblogs.microsoft.com/oldnewthing/20101018-00/?p=12513
// https://devblogs.microsoft.com/oldnewthing/20101019-00/?p=12503
// https://devblogs.microsoft.com/oldnewthing/20101021-00/?p=12483
// https://devblogs.microsoft.com/oldnewthing/20101022-00/?p=12473
// https://books.google.co.in/books?id=LpkFEO2FG8sC&pg=PA318&redir_esc=y#v=onepage&q&f=false
// https://books.google.co.in/books?id=qR6ngUchllsC&pg=PA349&redir_esc=y#v=onepage&q&f=false
// https://www.informit.com/articles/article.aspx?p=1186882
//-----------------------------------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <cstdlib>

uint32_t *curico_load_memory(const void *data, size_t dataSize, int *x, int *y, int *components);
uint32_t *curico_load_file(const char *filename, int *x, int *y, int *components);
bool curico_save_file(const char *filename, int x, int y, int components, const void *data);
