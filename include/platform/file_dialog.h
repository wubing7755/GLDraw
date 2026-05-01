/**
 * @file file_dialog.h
 * @brief Native file dialog helpers.
 */
#ifndef GLDRAW_PLATFORM_FILE_DIALOG_H
#define GLDRAW_PLATFORM_FILE_DIALOG_H

#include <stddef.h>

typedef enum PlatformFileDialogResult {
    PLATFORM_FILE_DIALOG_ERROR = -1,
    PLATFORM_FILE_DIALOG_CANCELLED = 0,
    PLATFORM_FILE_DIALOG_SELECTED = 1
} PlatformFileDialogResult;

/**
 * @brief Open a native file picker for GLDraw JSON documents.
 * @param out_path Destination buffer for the selected path.
 * @param out_path_size Destination buffer size.
 * @return Selection, cancellation, or error result.
 */
PlatformFileDialogResult platform_file_dialog_open_document(char* out_path,
                                                            size_t out_path_size);
/**
 * @brief Open a native save picker for PNG exports.
 * @param out_path Destination buffer for the selected path.
 * @param out_path_size Destination buffer size.
 * @param suggested_filename Initial filename to show when supported.
 * @return Selection, cancellation, or error result.
 */
PlatformFileDialogResult platform_file_dialog_save_png(char* out_path,
                                                       size_t out_path_size,
                                                       const char* suggested_filename);

#endif /* GLDRAW_PLATFORM_FILE_DIALOG_H */
