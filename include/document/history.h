/**
 * @file history.h
 * @brief 文档撤销/重做（快照 + 轻量编辑）接口。
 */
#ifndef GLDRAW_DOCUMENT_HISTORY_H
#define GLDRAW_DOCUMENT_HISTORY_H

#include <document/document.h>

#define DOCUMENT_HISTORY_MAX_ENTRIES 128

/**
 * @struct DocumentSnapshot
 * @brief 文档深拷贝快照。
 *
 * @member objects 深拷贝对象数组。
 * @member capacity `objects` 分配容量。
 * @member count 对象数量。
 * @member revision 快照时文档修订号。
 * @member next_id 快照时下一个可分配 ID。
 * @member selection 快照时选择集。
 */
typedef struct {
    GraphicObject** objects;
    int capacity;
    int count;
    unsigned int revision;
    ObjectId next_id;
    SelectionSet selection;
} DocumentSnapshot;

/**
 * @struct DocumentHistoryEntry
 * @brief 一条基于完整快照的历史记录（before -> after）。
 *
 * @member before 编辑前快照。
 * @member after 编辑后快照。
 */
typedef struct {
    DocumentSnapshot before;
    DocumentSnapshot after;
} DocumentHistoryEntry;

/**
 * @struct DocumentHistory
 * @brief 撤销/重做历史栈。
 *
 * @member undo_stack 撤销栈。
 * @member redo_stack 重做栈。
 * @member capacity 每个栈最大条目数。
 * @member undo_count 当前撤销栈条目数。
 * @member redo_count 当前重做栈条目数。
 */
typedef struct DocumentHistory {
    DocumentHistoryEntry* undo_stack;
    DocumentHistoryEntry* redo_stack;
    int capacity;
    int undo_count;
    int redo_count;
} DocumentHistory;

/**
 * @brief 初始化文档快照为空状态。
 * @param snapshot 待初始化快照。
 * @return 无。
 */
void document_snapshot_init(DocumentSnapshot* snapshot);

/**
 * @brief 释放快照拥有的对象拷贝与内部缓冲。
 * @param snapshot 目标快照。
 * @return 无。
 */
void document_snapshot_free(DocumentSnapshot* snapshot);

/**
 * @brief 捕获文档深拷贝到快照。
 * @param snapshot 目标快照（输出）。
 * @param document 源文档。
 * @return 成功返回非零；内存不足或克隆失败返回 0。
 */
int document_snapshot_capture(DocumentSnapshot* snapshot, const Document* document);

/**
 * @brief 初始化撤销/重做历史栈。
 * @param history 目标历史对象。
 * @return 成功返回非零；内存分配失败返回 0。
 */
int document_history_init(DocumentHistory* history);

/**
 * @brief 释放历史栈中所有条目。
 * @param history 目标历史对象。
 * @return 无。
 */
void document_history_shutdown(DocumentHistory* history);

/**
 * @brief 推入一次完整快照编辑记录。
 *
 * 算法说明：
 * 1. 复制 `before` 快照作为撤销起点；
 * 2. 从 `after_document` 采集快照作为撤销终点；
 * 3. 写入撤销栈（必要时淘汰最旧项）；
 * 4. 清空重做栈，保证线性历史分支。
 *
 * @param history 历史对象。
 * @param before 编辑前快照（调用后由历史接管内容）。
 * @param after_document 编辑后文档。
 * @return 成功返回非零，失败返回 0。
 */
int document_history_push(DocumentHistory* history, DocumentSnapshot* before, const Document* after_document);

/**
 * @brief 推入一次标量属性修改历史。
 *
 * 该接口用于记录对象单字段变化，避免完整快照开销。
 *
 * @param history 历史对象。
 * @param document 当前文档。
 * @param object_id 目标对象 ID。
 * @param key 标量字段名。
 * @param before_value 修改前值。
 * @param after_value 修改后值。
 * @param revision_before 修改前文档修订号。
 * @param revision_after 修改后文档修订号。
 * @return 成功返回非零，失败返回 0。
 */
int document_history_push_scalar_edit(DocumentHistory* history, const Document* document, ObjectId object_id, const char* key, float before_value, float after_value, unsigned int revision_before, unsigned int revision_after);

/**
 * @brief 推入一次批量平移历史。
 *
 * 该接口记录对象 ID 集合与位移向量，撤销/重做时按相反方向应用。
 *
 * @param history 历史对象。
 * @param document 当前文档。
 * @param object_ids 对象 ID 数组。
 * @param object_count 对象数量。
 * @param delta 平移增量（世界坐标）。
 * @param revision_before 修改前修订号。
 * @param revision_after 修改后修订号。
 * @return 成功返回非零，失败返回 0。
 */
int document_history_push_translate_edit(DocumentHistory* history, const Document* document, const ObjectId* object_ids, int object_count, Vec2 delta, unsigned int revision_before, unsigned int revision_after);

/**
 * @brief 执行撤销。
 *
 * 算法说明：
 * 1. 取撤销栈顶记录；
 * 2. 将记录应用到文档（完整快照或轻量编辑）；
 * 3. 该记录转移到重做栈顶；
 * 4. 返回应用结果。
 *
 * @param history 历史对象。
 * @param document 目标文档。
 * @return 有可撤销项且应用成功返回非零，否则返回 0。
 */
int document_history_undo(DocumentHistory* history, Document* document);

/**
 * @brief 执行重做。
 *
 * 算法说明与撤销对称：从重做栈取顶，应用后回推至撤销栈。
 *
 * @param history 历史对象。
 * @param document 目标文档。
 * @return 有可重做项且应用成功返回非零，否则返回 0。
 */
int document_history_redo(DocumentHistory* history, Document* document);

/**
 * @brief 判断是否可撤销。
 * @param history 历史对象。
 * @return 可撤销返回非零，否则返回 0。
 */
int document_history_can_undo(const DocumentHistory* history);

/**
 * @brief 判断是否可重做。
 * @param history 历史对象。
 * @return 可重做返回非零，否则返回 0。
 */
int document_history_can_redo(const DocumentHistory* history);

#endif /* GLDRAW_DOCUMENT_HISTORY_H */
