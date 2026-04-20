/**
 * @file document.h
 * @brief 文档对象容器与选择集操作接口。
 */
#ifndef GLDRAW_DOCUMENT_DOCUMENT_H
#define GLDRAW_DOCUMENT_DOCUMENT_H

#include <document/object.h>

#define DOCUMENT_MAX_OBJECTS 1024
#define DOCUMENT_MAX_SELECTION 128

/**
 * @struct SelectionSet
 * @brief 文档选择集（基于对象 ID）。
 *
 * @member ids 选中对象 ID 列表。
 * @member count 当前选中数量。
 */
typedef struct {
  ObjectId ids[DOCUMENT_MAX_SELECTION];
  int count;
} SelectionSet;

/**
 * @struct Document
 * @brief 文档主数据结构。
 *
 * @member objects 对象指针数组（由文档拥有生命周期）。
 * @member count 当前对象数量。
 * @member revision 文档修订号（编辑后递增）。
 * @member next_id 新对象自动分配 ID。
 * @member selection 当前选择集。
 */
typedef struct Document {
  GraphicObject *objects[DOCUMENT_MAX_OBJECTS];
  int count;
  unsigned int revision;
  ObjectId next_id;
  SelectionSet selection;
} Document;

/**
 * @brief 初始化文档为全新空状态。
 * @param document 待初始化文档。
 * @return 无。
 */
void document_init(Document *document);

/**
 * @brief 释放文档拥有的全部对象并清空运行时状态。
 * @param document 目标文档。
 * @return 无。
 */
void document_shutdown(Document *document);

/**
 * @brief 重置文档为“新建文档”状态。
 * @param document 目标文档。
 * @return 无。
 * @note 与 `document_shutdown` 不同，本函数会将 `revision` 与 `next_id` 重置到初始值。
 */
void document_reset(Document *document);

/**
 * @brief 向文档追加对象并自动分配 ID。
 * @param document 目标文档。
 * @param object 待插入对象。
 * @return 成功返回非零，容量不足或参数非法返回 0。
 */
int document_add_object(Document *document, GraphicObject *object);

/**
 * @brief 以指定 ID 向文档追加对象。
 * @param document 目标文档。
 * @param object 待插入对象。
 * @param id 指定对象 ID。
 * @return 成功返回非零；ID 冲突、参数非法或容量不足返回 0。
 */
int document_append_object_with_id(Document *document, GraphicObject *object,
                                   ObjectId id);

/**
 * @brief 按对象 ID 查找对象。
 * @param document 文档。
 * @param id 对象 ID。
 * @return 找到时返回对象指针，否则返回 `NULL`。
 */
GraphicObject *document_find_object(const Document *document, ObjectId id);

/**
 * @brief 按数组索引获取对象。
 * @param document 文档。
 * @param index 对象索引。
 * @return 索引有效时返回对象指针，否则返回 `NULL`。
 */
GraphicObject *document_get_object_at(const Document *document, int index);

/**
 * @brief 按 ID 删除对象并压缩对象数组。
 * @param document 目标文档。
 * @param id 待删除对象 ID。
 * @return 删除成功返回非零，否则返回 0。
 */
int document_remove_object(Document *document, ObjectId id);

/**
 * @brief 删除当前选择集中的全部对象。
 * @param document 目标文档。
 * @return 无。
 */
void document_delete_selection(Document *document);

/**
 * @brief 标记一次非结构性修改（修订号递增）。
 * @param document 目标文档。
 * @return 无。
 */
void document_touch(Document *document);

/**
 * @brief 计算文档中当前最大对象 ID。
 * @param document 文档。
 * @return 最大 ID；文档为空或参数非法返回 `0`。
 */
ObjectId document_max_id(const Document *document);

/**
 * @brief 清空选择集。
 * @param document 目标文档。
 * @return 无。
 */
void document_clear_selection(Document *document);

/**
 * @brief 判断对象 ID 是否在选择集中。
 * @param document 文档。
 * @param id 对象 ID。
 * @return 在选择集中返回非零，否则返回 0。
 */
int document_selection_contains(const Document *document, ObjectId id);

/**
 * @brief 向选择集添加对象 ID。
 * @param document 目标文档。
 * @param id 对象 ID。
 * @return 添加成功或已存在返回非零；参数非法或容量不足返回 0。
 */
int document_selection_add(Document *document, ObjectId id);

/**
 * @brief 从选择集中移除对象 ID。
 * @param document 目标文档。
 * @param id 对象 ID。
 * @return 无。
 */
void document_selection_remove(Document *document, ObjectId id);

/**
 * @brief 切换对象 ID 的选中状态。
 * @param document 目标文档。
 * @param id 对象 ID。
 * @return 无。
 */
void document_selection_toggle(Document *document, ObjectId id);

/**
 * @brief 获取“主选中对象”（选择集首元素对应对象）。
 * @param document 文档。
 * @return 主选中对象；无选择时返回 `NULL`。
 */
GraphicObject *document_primary_selection(const Document *document);

#endif /* GLDRAW_DOCUMENT_DOCUMENT_H */
