/**
 * @file persistence.h
 * @brief 文档 JSON 持久化接口。
 */
#ifndef GLDRAW_DOCUMENT_PERSISTENCE_H
#define GLDRAW_DOCUMENT_PERSISTENCE_H

#include <document/document.h>

/**
 * @brief 将文档保存为 JSON 文件。
 * @param document 待保存文档。
 * @param path 输出文件路径。
 * @return 保存成功返回非零，失败返回 0。
 */
int document_save_json(const Document* document, const char* path);

/**
 * @brief 从 JSON 文件加载文档。
 * @param document 目标文档（加载前会重置/覆盖）。
 * @param path 输入文件路径。
 * @return 加载成功返回非零，失败返回 0。
 */
int document_load_json(Document* document, const char* path);

#endif /* GLDRAW_DOCUMENT_PERSISTENCE_H */
