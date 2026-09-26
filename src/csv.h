// 剪贴板文本的解析：把多行内容切成一行行数据（每行是单元格列表）。
#pragma once

#include <string>
#include <vector>

namespace csv {

using Row = std::vector<std::string>;

// 按 Python csv.reader 的默认规则解析：
//   * 字段分隔符为逗号
//   * 字段可以用双引号包裹，双引号里的 "" 表示一个字面双引号
//   * 引号内的逗号 / 换行属于字段内容
//   * 记录结束符支持 \r\n、\n、\r
std::vector<Row> parseRows(const std::string& text);

// 该行是否所有单元格都是空白（等价于 Python 的 any(cell.strip() for cell in row) == False）
bool isBlankRow(const Row& row);

}  // namespace csv
