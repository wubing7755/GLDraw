/**
 * @file file_dialog.c
 * @brief Native file dialog implementation.
 */
#include <platform/file_dialog.h>

#include <string.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commdlg.h>
#endif

PlatformFileDialogResult platform_file_dialog_open_document(char* out_path,
                                                            size_t out_path_size)
{
    if (!out_path || out_path_size == 0u) {
        return PLATFORM_FILE_DIALOG_ERROR;
    }

    out_path[0] = '\0';

#ifdef _WIN32
    {
        OPENFILENAMEA dialog;
        char path_buffer[260];

        memset(&dialog, 0, sizeof(dialog));
        memset(path_buffer, 0, sizeof(path_buffer));

        dialog.lStructSize = sizeof(dialog);
        dialog.hwndOwner = NULL;
        dialog.lpstrFilter = "GLDraw Documents (*.json)\0*.json\0All Files (*.*)\0*.*\0";
        dialog.lpstrFile = path_buffer;
        dialog.nMaxFile = sizeof(path_buffer);
        dialog.lpstrDefExt = "json";
        dialog.Flags = OFN_FILEMUSTEXIST |
                       OFN_PATHMUSTEXIST |
                       OFN_NOCHANGEDIR |
                       OFN_HIDEREADONLY;

        if (GetOpenFileNameA(&dialog)) {
            if (strlen(path_buffer) + 1u > out_path_size) {
                return PLATFORM_FILE_DIALOG_ERROR;
            }
            memcpy(out_path, path_buffer, strlen(path_buffer) + 1u);
            return PLATFORM_FILE_DIALOG_SELECTED;
        }

        if (CommDlgExtendedError() == 0) {
            return PLATFORM_FILE_DIALOG_CANCELLED;
        }
        return PLATFORM_FILE_DIALOG_ERROR;
    }
#else
    (void)out_path;
    (void)out_path_size;
    return PLATFORM_FILE_DIALOG_ERROR;
#endif
}

PlatformFileDialogResult platform_file_dialog_save_png(char* out_path,
                                                       size_t out_path_size,
                                                       const char* suggested_filename)
{
    if (!out_path || out_path_size == 0u) {
        return PLATFORM_FILE_DIALOG_ERROR;
    }

    out_path[0] = '\0';

#ifdef _WIN32
    {
        OPENFILENAMEA dialog;
        char path_buffer[260];

        memset(&dialog, 0, sizeof(dialog));
        memset(path_buffer, 0, sizeof(path_buffer));
        if (suggested_filename && suggested_filename[0] != '\0') {
            if (strlen(suggested_filename) + 1u >= sizeof(path_buffer)) {
                return PLATFORM_FILE_DIALOG_ERROR;
            }
            memcpy(path_buffer, suggested_filename, strlen(suggested_filename) + 1u);
        }

        dialog.lStructSize = sizeof(dialog);
        dialog.hwndOwner = NULL;
        dialog.lpstrFilter = "PNG Images (*.png)\0*.png\0All Files (*.*)\0*.*\0";
        dialog.lpstrFile = path_buffer;
        dialog.nMaxFile = sizeof(path_buffer);
        dialog.lpstrDefExt = "png";
        dialog.Flags = OFN_PATHMUSTEXIST |
                       OFN_NOCHANGEDIR |
                       OFN_OVERWRITEPROMPT |
                       OFN_HIDEREADONLY;

        if (GetSaveFileNameA(&dialog)) {
            if (strlen(path_buffer) + 1u > out_path_size) {
                return PLATFORM_FILE_DIALOG_ERROR;
            }
            memcpy(out_path, path_buffer, strlen(path_buffer) + 1u);
            return PLATFORM_FILE_DIALOG_SELECTED;
        }

        if (CommDlgExtendedError() == 0) {
            return PLATFORM_FILE_DIALOG_CANCELLED;
        }
        return PLATFORM_FILE_DIALOG_ERROR;
    }
#else
    (void)out_path;
    (void)out_path_size;
    (void)suggested_filename;
    return PLATFORM_FILE_DIALOG_ERROR;
#endif
}
