#include "csv.h"

namespace csv {
namespace {

constexpr char kDelimiter = ',';
constexpr char kQuote = '"';

bool isAsciiSpace(unsigned char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

}  // namespace

std::vector<Row> parseRows(const std::string& text) {
    std::vector<Row> rows;
    Row record;
    std::string field;

    bool in_quotes = false;       // 正在读一个被双引号包裹的字段
    bool field_started = false;   // 当前字段已经写过字符（含起始引号），决定引号是否生效
    bool record_touched = false;  // 当前记录里出现过内容（纯空行不会生成记录）

    auto endField = [&]() {
        record.push_back(field);
        field.clear();
        field_started = false;
    };

    auto endRecord = [&]() {
        if (record_touched || !record.empty() || !field.empty()) {
            record.push_back(field);
            rows.push_back(record);
        }
        record.clear();
        field.clear();
        field_started = false;
        in_quotes = false;
        record_touched = false;
    };

    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];

        if (in_quotes) {
            if (c == kQuote) {
                if (i + 1 < text.size() && text[i + 1] == kQuote) {
                    field.push_back(kQuote);  // "" -> 一个字面双引号
                    ++i;
                } else {
                    in_quotes = false;  // 字段结束
                }
            } else {
                field.push_back(c);  // 引号内的逗号 / 换行都算内容
            }
            continue;
        }

        if (c == kQuote && !field_started) {
            in_quotes = true;
            field_started = true;
            record_touched = true;
            continue;
        }

        if (c == kDelimiter) {
            endField();
            record_touched = true;
            continue;
        }

        if (c == '\r' || c == '\n') {
            if (c == '\r' && i + 1 < text.size() && text[i + 1] == '\n') ++i;  // \r\n 算一个结束符
            endRecord();
            continue;
        }

        field.push_back(c);
        field_started = true;
        record_touched = true;
    }

    endRecord();  // 收尾：最后一条可能没有换行符
    return rows;
}

bool isBlankRow(const Row& row) {
    for (const std::string& cell : row) {
        for (char ch : cell) {
            if (!isAsciiSpace(static_cast<unsigned char>(ch))) return false;
        }
    }
    return true;
}

}  // namespace csv
